#include "src/common/logging.h"

#include <dlfcn.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <google/protobuf/message.h>

#include <tr1/unordered_map>

#include "src/common/assert.h"
#include "src/common/flag.h"
#include "src/common/scoped_ptr.h"
#include "src/common/self_thread.h"
#include "src/sync/atomic.h"
#include "src/sync/lock.h"

#include "include/routine_thread_pool.h"

DECLARE_FLAG_BOOL(common_EnableApsaraLoggingSystem);
DECLARE_FLAG_BOOL(common_AutoInitLoggingSystem);
DECLARE_FLAG_BOOL(common_EnableAsyncLogging);

// GLOBAL_NOLINT

struct GZlib
{
    typedef void* (*GzOpen)(const char* path, const char* mode);
    typedef int   (*GzWrite)(void* file, void* buf, unsigned int len);
    typedef int   (*GzClose)(void* file);

    GzOpen mGzOpen;
    GzWrite mGzWrite;
    GzClose mGzClose;

    GZlib()
    {
        // test if zlib is statically linked
        mGzOpen = (GzOpen)dlsym(0, "gzopen");
        mGzWrite = (GzWrite)dlsym(0, "gzwrite");
        mGzClose = (GzClose)dlsym(0, "gzclose");

        if (mGzOpen == NULL) {
            void* handle = dlopen("libz.so", RTLD_NOW);
            if (handle != NULL) {
                mGzOpen = (GzOpen)dlsym(handle, "gzopen");
                mGzWrite = (GzWrite)dlsym(handle, "gzwrite");
                mGzClose = (GzClose)dlsym(handle, "gzclose");
            }
        }
    }
};

static bool sLoggingSystemInit = false;
static MicroRWLock sRWLock;

static void*          sTLSLogHelperListHeader = NULL;
static void*          sTLSLogHelperFreeListHeader = NULL;
static __thread void* sTLSLogHelper = NULL;
static pthread_key_t  sTLSLogHelperSessionKey;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

// Cache 0-59 minute
static uint16_t sInt2StrMap[60] __attribute__ ((aligned (64)));

struct LogLevelDict
{
    LogLevelIndex index;
    LogLevel level;
    char* symbol;
    char* easySymbol;
};

static LogLevelDict sLogLevelDict[] = {
    {LOG_LEVEL_INDEX_ALL,     LOG_LEVEL_ALL,      "ALL",     "SPAM"    },
    {LOG_LEVEL_INDEX_PROFILE, LOG_LEVEL_PROFILE,  "PROFILE", "XXXXXXXX"},
    {LOG_LEVEL_INDEX_DEBUG,   LOG_LEVEL_DEBUG,    "DEBUG",   "DEBUG"   },
    {LOG_LEVEL_INDEX_INFO,    LOG_LEVEL_INFO,     "INFO",    "INFO"    },
    {LOG_LEVEL_INDEX_WARNING, LOG_LEVEL_WARNING,  "WARNING", "WARN"    },
    {LOG_LEVEL_INDEX_ERROR,   LOG_LEVEL_ERROR,    "ERROR",   "ERROR"   },
    {LOG_LEVEL_INDEX_FATAL,   LOG_LEVEL_FATAL,    "FATAL",   "FATAL"   },
};

static LogLevelIndex sEasyLevelDict[] = {
    LOG_LEVEL_INDEX_FATAL,
    LOG_LEVEL_INDEX_FATAL,
    LOG_LEVEL_INDEX_ERROR,
    LOG_LEVEL_INDEX_WARNING,
    LOG_LEVEL_INDEX_INFO,
    LOG_LEVEL_INDEX_DEBUG,
    LOG_LEVEL_INDEX_PROFILE
};

// do not use global static to remove dependency of init order
static std::map<std::string, Logger*>* GetKeyToLoggerMap()
{
    static std::map<std::string, Logger*> sKeyToLoggerMap;
    return &sKeyToLoggerMap;
}

static std::map<std::string, ILoggingSystem*>* GetNameToLoggingSystemMap()
{
    static std::map<std::string, ILoggingSystem*> sNameToLoggingSystemMap;
    return &sNameToLoggingSystemMap;
}

static ILoggingSystem* GetApsaraLoggingSystem()
{
    std::map<std::string, ILoggingSystem*>* logsystemMap
        = GetNameToLoggingSystemMap();
    ScopedMicroRWLock lock(sRWLock, 'r');
    typeof(logsystemMap->end()) it = logsystemMap->find("apsara");
    if (it != logsystemMap->end())
    {
        return it->second;
    }
    return NULL;
}

static LogLevelIndex ConvertLogLevelToLogLevelIndex(LogLevel level)
{
    int quot = static_cast<uint32_t>(level) / 100;
    if (LIKELY(quot < LOG_LEVEL_INDEX_NONE
                && sLogLevelDict[quot].level == level))
    {
        return (LogLevelIndex)quot;
    }
    fprintf(stderr, "Oops, %s:%u %s logging bugs happend, level:%u,"
            " report as INFO!!!!\n",
            __FILE__, __LINE__, __FUNCTION__, static_cast<int>(level));
    ASSERT_DEBUG(false);
    return LOG_LEVEL_INDEX_INFO; // cannot happens;
}

static LogLevelIndex FindLogLevelIndexBySymbol(const std::string& symbol)
{
    size_t size = sizeof(sLogLevelDict) / sizeof(sLogLevelDict[0]);
    for (size_t idx = 0; idx < size; ++idx)
    {
        if (sLogLevelDict[idx].symbol == symbol)
        {
            return sLogLevelDict[idx].index;
        }
    }
    fprintf(stderr, "Oops, %s:%u %s logging bugs happend, level:%s,"
            " report as INFO!!!!\n",
            __FILE__, __LINE__, __FUNCTION__, symbol.c_str());
    ASSERT_DEBUG(false);
    return LOG_LEVEL_INDEX_INFO; // cannot happens;
}

static LogLevelIndex ConvertEasyLogLevelToLogLevelIndex(int level)
{
    if (level >= sizeof(sEasyLevelDict) / sizeof(sEasyLevelDict[0]))
    {
        level = sizeof(sEasyLevelDict) / sizeof(sEasyLevelDict[0]) - 1;
    }
    return sEasyLevelDict[level];
}

static LogLevelIndex FindLogLevelIndexByEasySymbol(const char* symbol)
{
    size_t size = sizeof(sLogLevelDict) / sizeof(sLogLevelDict[0]);
    for (size_t idx = 0; idx < size; ++idx)
    {
        if (strcasecmp(sLogLevelDict[idx].easySymbol, symbol) == 0)
        {
            return sLogLevelDict[idx].index;
        }
    }
    fprintf(stderr, "Oops, %s:%u %s logging bugs happend, level:%s,"
            " report as INFO!!!!\n",
            __FILE__, __LINE__, __FUNCTION__, symbol);
    ASSERT_DEBUG(false);
    return LOG_LEVEL_INDEX_INFO; // cannot happens;
}

static std::string FormatUserKey(const std::string& key)
{
    return key;
}

struct AppendLogTaskClosure : public ::google::protobuf::Closure
{
    AppendLogTaskClosure(const char* data_, uint32_t size_,
            ILoggerAdaptor** adaptorList_, const ConstLoggingHeader& header_,
            size_t headerLen_, size_t tid_)
        : refData(data_, size_, MAX_SUPPORT_LOGGING_SYSTEM),
          adaptorList(adaptorList_), task(this)
    {
        header.filename = header_.filename;
        header.function = header_.function;
        header.line = header_.line;
        header.level = header_.level;
        header.headerLen = headerLen_;
        header.tid = tid_;
    }
    /* override */ void Run()
    {
        size_t appendCnt = 0;
        while (*adaptorList != NULL)
        {
            ILoggerAdaptor* adaptor = *adaptorList;
            if (LIKELY(adaptor->IsLevelEnabled(header.level)))
            {
                adaptor->AppendLog(header, &refData);
                ++appendCnt;
            }
            ++adaptorList;
        }
        refData.SubRef(MAX_SUPPORT_LOGGING_SYSTEM - appendCnt);
    }
    RefCountedLoggingData refData;
    ILoggerAdaptor** adaptorList;
    LoggingHeader header;
    EasyTask task;
};

class TLSLogHelper
{
public:
    TLSLogHelper()
    {
        mTid = common::ThisThread::GetId();
        mLoggingEnabled = true;
        char buf[32];
        mTidLen = snprintf(buf, sizeof(buf), "%u", mTid);
        size_t size = sizeof(sLogLevelDict) / sizeof(sLogLevelDict[0]);
        for (size_t idx = 0; idx < size; ++idx)
        {
            ASSERT_DEBUG(sLogLevelDict[idx].index == idx);

            LogLevelIndex index = sLogLevelDict[idx].index;
            mHeaderDataLen[index] = snprintf(mHeaderData[index],
                    sizeof(mHeaderData[index]), "[%s]\t[%s]\t[%u]\t[",
                    "1970-01-01 00:00:00.000000",
                    sLogLevelDict[idx].symbol, mTid);
        }
        memset(mLastLogTimeInHour, 0, sizeof(mLastLogTimeInHour));
        memset(mLogCounter, 0, sizeof(mLogCounter));
        mNextFree = NULL;
        mNext = static_cast<TLSLogHelper*>(sTLSLogHelperListHeader);
        sTLSLogHelperListHeader = this;
    }

    void EnableLogging()    { mLoggingEnabled = true;  }
    void DisableLogging()   { mLoggingEnabled = false; }
    bool IsLoggingEnabled() { return mLoggingEnabled;  }

    size_t GetTidLen()      { return mTidLen;          }
    size_t GetTid()         { return mTid;             }

    TLSLogHelper* GetNext()     { return mNext;     }
    TLSLogHelper* GetNextFree() { return mNextFree; }
    void SetAsFree()
    {
        mNextFree = static_cast<TLSLogHelper*>(sTLSLogHelperFreeListHeader);
        sTLSLogHelperFreeListHeader = this;
    }

    // counter related
    void AddCounter(LogLevelIndex index)   { mLogCounter[index]++;      }
    size_t GetCounter(LogLevelIndex index) { return mLogCounter[index]; }

    const uint64_t* GetHeaderFragment(LogLevelIndex index, size_t* sizeptr)
    {
        char* result = mHeaderData[index];
        __builtin_prefetch(result, 0, 1);
        __builtin_prefetch(sInt2StrMap, 0, 1);
        __builtin_prefetch(reinterpret_cast<char*>(sInt2StrMap) + 64, 0, 1);
        mLastLogTimeInHour[index] = updateTimeStamp(result + 1,
                mLastLogTimeInHour[index]);
        *sizeptr = mHeaderDataLen[index];
        return reinterpret_cast<uint64_t*>(result);
    }

    // return result with format: 1970-01-01 00:00:00.000000
    // total length = 26
    const char* GetCurrentTimeString(size_t *sizeptr)
    {
        int index = LOG_LEVEL_INDEX_WARNING;
        char* result = mHeaderData[index];
        mLastLogTimeInHour[index] = updateTimeStamp(result + 1,
                mLastLogTimeInHour[index]);

        // enasure LOGGING_TIMESTAMP_FRAGMENT_LEN <= 32, because
        // GetCurrentTimeInString depends on it
        STATIC_ASSERT(LOGGING_TIMESTAMP_FRAGMENT_LEN <= 32);
        *sizeptr = LOGGING_TIMESTAMP_FRAGMENT_LEN;
        return result + 1;
    }

private:
    uint64_t updateTimeStamp(char* data, uint64_t lasthour)
    {
        uint64_t now = common::GetCurrentTimeInUs();
        uint32_t usecond = now % 1000000ULL;
        time_t   second = now / 1000000ULL;
        uint32_t thishour = second / 3600;
        uint32_t thissecond = second % 3600;
        if (UNLIKELY(thishour != lasthour))
        {
            struct tm t;
            localtime_r(&second, &t);
            sprintf(data, "%d-%02d-%02d %02d:", t.tm_year + 1900, t.tm_mon + 1,
                    t.tm_mday, t.tm_hour);
        }
        uint32_t minute = thissecond / 60;
        uint32_t onlysecond = thissecond % 60;
        data += LOGGING_HEADER_FRAGMENT_LEN_BEFORE_MINUTE;
        *reinterpret_cast<uint16_t*>(data) = sInt2StrMap[minute];
        data += 2; *data++ = ':';
        *reinterpret_cast<uint16_t*>(data) = sInt2StrMap[onlysecond];
        data += 2;
        IntegerFormatUtil::FormatInteger(1000000ULL + usecond, data);
        *data = '.'; // replace the leading '1' with '.'
        return thishour;
    }

    enum
    {
        LOGGING_TIMESTAMP_FRAGMENT_LEN = 26,
        LOGGING_MAX_HEADER_FRAGMENT_LEN = 64,
        LOGGING_HEADER_FRAGMENT_LEN_BEFORE_MINUTE = 14,
    };

    char mHeaderData[LOG_LEVEL_INDEX_NONE][LOGGING_MAX_HEADER_FRAGMENT_LEN];
    TLSLogHelper* mNext;
    TLSLogHelper* mNextFree;
    uint64_t mLastLogTimeInHour[LOG_LEVEL_INDEX_NONE];
    uint32_t mHeaderDataLen[LOG_LEVEL_INDEX_NONE];
    uint32_t mLogCounter[LOG_LEVEL_INDEX_NONE];
    uint32_t mTid;
    uint16_t mTidLen;
    uint8_t mPadding;
    bool mLoggingEnabled;
};

static void DestroyTLSLogHelper(void* param)
{
    ScopedMicroRWLock lock(sRWLock, 'w');
    TLSLogHelper* helper = static_cast<TLSLogHelper*>(param);
    helper->SetAsFree();
}

static void InitTLSLogHelperKey()
{
    int ret = pthread_key_create(&sTLSLogHelperSessionKey, &DestroyTLSLogHelper);
    if (ret != 0)
    {
        fprintf(stderr, "Create thread key error in InitTLSLogHelperKey");
        abort();
    }
}

static TLSLogHelper* GetTLSLogHelper()
{
    TLSLogHelper* helper = static_cast<TLSLogHelper*>(sTLSLogHelper);
    if (UNLIKELY(helper == NULL))
    {
        pthread_once(&once_control, InitTLSLogHelperKey);
        {
            ScopedMicroRWLock lock(sRWLock, 'w');
            TLSLogHelper* newhelper = static_cast<TLSLogHelper*>(sTLSLogHelperFreeListHeader);
            if (newhelper == NULL)
            {
                sTLSLogHelper = helper = new TLSLogHelper();
            }
            else
            {
                sTLSLogHelper = helper = newhelper;
                sTLSLogHelperFreeListHeader = newhelper->GetNextFree();
            }
        }
        int ret = pthread_setspecific(sTLSLogHelperSessionKey, sTLSLogHelper);
        if (ret != 0)
        {
            fprintf(stderr, "pthread_setspecific fail in GetTLSLogHelper");
            abort();
        }
    }
    return helper;
}

ILoggingSystem::ILoggingSystem(const std::string& name)
{
    mName = name;
}

ILoggingSystem::~ILoggingSystem()
{
}

Logger::Logger()
{
    memset(mAdaptorList, 0, sizeof(mAdaptorList));
    mMinLevel = LOG_LEVEL_NONE;
}

Logger* Logger::GetLogger(const std::string& key)
{
    if (!sLoggingSystemInit && BOOL_FLAG(common_AutoInitLoggingSystem))
    {
        InitLoggingSystem();
    }
    std::string fmtkey = FormatUserKey(key);
    std::map<std::string, Logger*>* loggerMap = GetKeyToLoggerMap();
    {
        ScopedMicroRWLock lock(sRWLock, 'r');
        typeof (loggerMap->end()) it = loggerMap->find(fmtkey);
        if (it != loggerMap->end())
        {
            return it->second;
        }
    }
    {
        std::map<std::string, ILoggingSystem*>* logsystemMap
            = GetNameToLoggingSystemMap();

        ScopedMicroRWLock lock(sRWLock, 'w');
        Logger** loggerPtr = &(*loggerMap)[fmtkey];
        if (*loggerPtr == NULL)
        {
            Logger* logger = new Logger;
            for (typeof (logsystemMap->end()) it = logsystemMap->begin();
                    it != logsystemMap->end(); ++it)
            {
                ILoggingSystem* system = it->second;
                ILoggerAdaptor* adaptor = system->GetLogger(fmtkey);
                logger->AddAdaptor(adaptor);
            }
            *loggerPtr = logger;
        }
        return *loggerPtr;
    }
}

void Logger::AppendLog(const ConstLoggingHeader& header, size_t headerLen, size_t tid,
        const char* data, uint32_t size, uint32_t capacity)
{
    if (UNLIKELY(size + sizeof(AppendLogTaskClosure) > capacity))
    {
        capacity = (size & ~0x0FULL) + 16 + sizeof(AppendLogTaskClosure);
        data = static_cast<char*>(realloc(const_cast<char*>(data), capacity));
    }
    AppendLogTaskClosure* taskClosure = reinterpret_cast<AppendLogTaskClosure*>(
            const_cast<char*>(data) + capacity - sizeof(AppendLogTaskClosure));
    new (taskClosure) AppendLogTaskClosure(data, size, mAdaptorList, header,
            headerLen, tid);
    if (false && LIKELY(BOOL_FLAG(common_EnableAsyncLogging)))
    {
        PushTaskToNonCriticalTaskThreadPool(&taskClosure->task, 0);
    }
    else
    {
        taskClosure->Run();
    }
}

void Logger::AddAdaptor(ILoggerAdaptor* adaptor)
{
    // protected by sRWLock
    mMinLevel = std::min(mMinLevel, adaptor->GetLogLevel());
    ILoggerAdaptor** adaptorList = mAdaptorList;
    while (*adaptorList != NULL)
    {
        ++adaptorList;
    }
    ILoggerAdaptor** nextList = adaptorList + 1;
    if (*nextList != NULL)
    {
        while (*nextList != NULL)
        {
            ++nextList;
        }
        *nextList = *(adaptorList + 1);
    }
    *adaptorList = adaptor;
}

void Logger::DisableAdaptor(ILoggerAdaptor* adaptor)
{
    ILoggerAdaptor** nilptr = NULL;
    for (size_t idx = 0; idx <= MAX_SUPPORT_LOGGING_SYSTEM; ++idx)
    {
        if (mAdaptorList[idx] == NULL)
        {
            nilptr = &mAdaptorList[idx];
        }
        if (mAdaptorList[idx] == adaptor)
        {
            if (nilptr != NULL)
            {
                // after nilptr, it has been disabled
                return;
            }
            // find it, remove it to last, any some logging may output twice
            for (size_t idy = idx + 1; idy <= MAX_SUPPORT_LOGGING_SYSTEM; ++idy)
            {
                mAdaptorList[idy - 1] = mAdaptorList[idy];
            }
            mAdaptorList[MAX_SUPPORT_LOGGING_SYSTEM] = adaptor;
            return;
        }
    }
    // Oops, disable a unknown adaptor
    ASSERT_DEBUG(false);
}

void Logger::EnableAdaptor(ILoggerAdaptor* adaptor)
{
    ILoggerAdaptor** nilptr = NULL;
    for (size_t idx = 0; idx <= MAX_SUPPORT_LOGGING_SYSTEM; ++idx)
    {
        if (mAdaptorList[idx] == NULL)
        {
            if (nilptr == NULL)
            {
                nilptr = &mAdaptorList[idx];
            }
        }
        else if (mAdaptorList[idx] == adaptor)
        {
            if (nilptr == NULL)
            {
                // still enable adaptor, skip
                return;
            }
            ILoggerAdaptor* nilnext = *(nilptr + 1);
            // set next to NULL, mark iterator end
            *(nilptr + 1) = NULL;
            *nilptr = adaptor;
            mAdaptorList[idx] = nilnext;
            return;
        }
    }
    // Oops, enable a unknown adaptor
    ASSERT_DEBUG(false);
}

void Logger::ReloadLevel()
{
    LogLevel level = LOG_LEVEL_NONE;
    ILoggerAdaptor** adaptorList = mAdaptorList;
    while (*adaptorList != NULL)
    {
        ILoggerAdaptor* adaptor = *adaptorList;
        level = std::min(level, adaptor->GetLogLevel());
        ++adaptorList;
    }
    mMinLevel = level;
}

uint64_t Logger::GetGlobalCounter(LogLevel level)
{
    uint64_t count = 0;
    ScopedMicroRWLock lock(sRWLock, 'r');
    TLSLogHelper* helper = static_cast<TLSLogHelper*>(sTLSLogHelperListHeader);
    while (helper != NULL)
    {
        count += helper->GetCounter(ConvertLogLevelToLogLevelIndex(level));
        helper = helper->GetNext();
    }
    return count;
}

LogMaker::LogMaker(Logger* logger, const char* filename, int line,
        const char* function, LogLevel level)
{
    mLogger = logger;
    mLogHeader.filename = filename;
    mLogHeader.function = function;
    mLogHeader.line = line;
    mLogLevelIndex = ConvertLogLevelToLogLevelIndex(level);
    mLogHeader.level = sLogLevelDict[mLogLevelIndex].level;
    mFileNameLen = strlen(mLogHeader.filename);
    char data[128];
    mLineCodeSize = snprintf(data, sizeof(data), ":%d]", mLogHeader.line);
    mLineCode = *reinterpret_cast<uint64_t*>(data);

    mHeaderNoTidLen = mFileNameLen +
        sprintf(data, "[%d-%02d-%02d %02d:%02d:%02d.%06d]\t[%s]\t[%s]\t[:%d]\t",
                1970, 1, 1, 0, 0, 0, 0, sLogLevelDict[mLogLevelIndex].symbol,
                "", mLogHeader.line);
}

LogMaker::~LogMaker()
{
}

TLSLogHelper* LogMaker::MakeLogHeader(StringPieceFormatter& formatter)
{
    TLSLogHelper* helper = GetTLSLogHelper();
    if (UNLIKELY(!helper->IsLoggingEnabled()))
    {
        return NULL;
    }
    helper->AddCounter(mLogLevelIndex);
    size_t size = 0;
    const uint64_t* data = helper->GetHeaderFragment(mLogLevelIndex, &size);
    // data = "[1970-01-01 00:00:00.000000]\t[INFO|WARNING|FATAL]\t[tid]\t["
    // max(strlen(data)) = 56, so....
    formatter.append(*(data + 0), *(data + 1), sizeof(*data) * 2);
    formatter.append(*(data + 2), *(data + 3), sizeof(*data) * 2);
    if (LIKELY(size <= sizeof(*data) * 6))
    {
        formatter.append(*(data + 4), *(data + 5), size - sizeof(*data) * 4);
    }
    else
    {
        formatter.append(*(data + 4), *(data + 5), sizeof(*data) * 2);
        formatter.append(*(data + 6), size - sizeof(*data) * 6);
    }
    // mFileName
    formatter.append(mLogHeader.filename, mFileNameLen);
    // data = ":300]"; line number
    formatter.append(mLineCode, mLineCodeSize);
    return helper;
}

void LogMaker::WriteLogToSink(StringPieceFormatter& formatter, TLSLogHelper* helper)
{
    formatter << "\n";
    size_t capacity = formatter.capacity();
    size_t size = 0;
    const char* data = formatter.release(&size);
    mLogger->AppendLog(mLogHeader, mHeaderNoTidLen + helper->GetTidLen(),
            helper->GetTid(), data, size, capacity);
}

void LogCallTracer::recordLogCallLeave()
{
    mLogMaker->AppendLog("Leave", mLogMaker->mLogHeader.function);
}

void InitLoggingSystem()
{
    if (!sLoggingSystemInit)
    {
        ScopedMicroRWLock lock(sRWLock, 'w');
        if (!sLoggingSystemInit)
        {
            char buf[8];
            size_t cnt = sizeof(sInt2StrMap) / sizeof(sInt2StrMap[0]);
            for (size_t idx = 0; idx < cnt; ++idx)
            {
                sprintf(buf, "%02lu", idx);
                sInt2StrMap[idx] = *reinterpret_cast<uint16_t*>(buf);
            }
            sLoggingSystemInit = true;
        }
    }

    if (BOOL_FLAG(common_EnableApsaraLoggingSystem))
    {
        RegisterLoggingSystem(CreateApsaraLoggingSystem());
    }
}

void UninitLoggingSystem()
{
    if (sLoggingSystemInit)
    {
        std::map<std::string, ILoggingSystem*>* logsystemMap
            = GetNameToLoggingSystemMap();
        ScopedMicroRWLock lock(sRWLock, 'w');
        if (!sLoggingSystemInit)
        {
            return;
        }
        for (typeof (logsystemMap->end()) it = logsystemMap->begin();
                it != logsystemMap->end(); ++it)
        {
            ILoggingSystem* system = it->second;
            system->TearDown();
        }
        sLoggingSystemInit = false;
    }
}

void FlushLog()
{
    if (sLoggingSystemInit)
    {
        std::map<std::string, ILoggingSystem*>* logsystemMap
            = GetNameToLoggingSystemMap();
        ScopedMicroRWLock lock(sRWLock, 'w');
        for (typeof (logsystemMap->end()) it = logsystemMap->begin();
                it != logsystemMap->end(); ++it)
        {
            ILoggingSystem* system = it->second;
            system->FlushLog();
        }
    }
}

static void RotateLogFile(
        const std::string& src,
        const std::string& dest,
        int maxDay)
{
    if (access(src.c_str(), F_OK) != 0)
    {
        return;
    }
    const int secPerDay = 3600 * 24;
    time_t now = time(NULL);
    struct stat buffer;
    stat(src.c_str(), &buffer);
    time_t fileModifyTime = time_t(buffer.st_mtime);

    if (((now - fileModifyTime) / secPerDay) >= maxDay)
    {
        if (remove(src.c_str())!= 0 && errno != ENOENT)
            std::cerr << "Can't remove " << src << ", errno= "
                << strerror(errno) << std::endl;
    }
    else if (rename(src.c_str(), dest.c_str()) != 0 && errno != ENOENT)
    {
        std::cerr << "Can't rename " << src << " to " << dest
            << ", err=" << strerror(errno) << std::endl;
    }
}

static std::string CompressLogFile(const std::string& src)
{
    static GZlib gzlib;
    if (gzlib.mGzOpen == NULL)
    {
        string cmd = string("gzip -f ") + src;
        int res = system(cmd.c_str());
        if (res != 0)
        {
            remove(src.c_str());
            return "";
        }
        return src + ".gz";
    }
    string gzip = src + ".gz";
    FILE* srcFile = fopen(src.c_str(), "rb");
    if (srcFile == NULL)
    {
        remove(src.c_str());
        return "";
    }
    void* gzipFile = gzlib.mGzOpen(gzip.c_str(), "wb");
    if (gzipFile == NULL)
    {
        remove(src.c_str());
        fclose(srcFile);
        return "";
    }
    static const uint32_t kBufferSize = 64 * 1024;
    scoped_array<char> buffer(new char[kBufferSize]);
    bool success = true;
    while (!feof(srcFile))
    {
        const int32_t bytesRead = fread(buffer.get(), 1, kBufferSize, srcFile);
        if (bytesRead <= 0 || gzlib.mGzWrite(gzipFile, buffer.get(), bytesRead) < 0)
        {
            success = false;
            break;
        }
        sched_yield();
    }
    fclose(srcFile);
    gzlib.mGzClose(gzipFile);
    remove(src.c_str());
    if (!success)
    {
        remove(gzip.c_str());
        return "";
    }
    return gzip;
}

void RotateLogFiles(const std::string& path,
                    bool bCompress,
                    int maxFileNum,
                    int maxDay,
                    std::string* tmpFile)
{
    if (access(path.c_str(), F_OK) != 0)
    {
        return; // path doesn't exist
    }
    string lastFile = path + "." + ToString(maxFileNum - 1);
    if (bCompress)
    {
        lastFile += ".gz";
    }
    if (access(lastFile.c_str(), F_OK) == 0)
    {
        if (tmpFile != NULL)
        { 
            // Use micro seconds as file name to avoid duplication.
            struct timeval now;
            gettimeofday(&now, NULL);
            *tmpFile = lastFile + "." + ToString(now.tv_usec);
            if (rename(lastFile.c_str(), tmpFile->c_str()) != 0
                    && errno != ENOENT)
            {
                std::cerr << "Can't rename " << lastFile << ", errno="
                    << strerror(errno) << std::endl;
            }
        }
        else
        {
            if (remove(lastFile.c_str()) != 0 && errno != ENOENT)
            {
                std::cerr << "Can't remove " << lastFile << ", errno="
                    << strerror(errno) << std::endl;
            }
        }
    }

    if (!bCompress)
    {
        for (int n = maxFileNum - 2; n >= 0; --n)
        {
            string src = path;
            if (n > 0)
            {
                src += "." + ToString(n);
            }
            string dest = path + "." + ToString(n + 1);
            RotateLogFile(src, dest, maxDay);
        }
        return;
    }

    for (int n = maxFileNum - 2; n >= 0; --n)
    {
        string src;
        if (n > 0)
        {
            src.reserve(path.size() + 10);
            src.append(path).append(".").append(ToString(n)).append(".gz");
        }
        else
        {
            src = CompressLogFile(src);
            if (src.empty())
            {
                continue;
            }
        }
        string dest = path + "." + ToString(n + 1) + ".gz";
        RotateLogFile(src, dest, maxDay);
    }
}

//
// LoadConfig and reconfiguration logging system
void ReloadLogLevel()
{
    std::map<std::string, Logger*>* loggerMap = GetKeyToLoggerMap();
    ScopedMicroRWLock lock(sRWLock, 'w');
    for (typeof(loggerMap->end()) it = loggerMap->begin();
            it != loggerMap->end(); ++it)
    {
        it->second->ReloadLevel();
    }
}

bool LoadConfig(const std::string& jsonContent)
{
    std::map<std::string, ILoggingSystem*>* logsystemMap
        = GetNameToLoggingSystemMap();
    try
    {
        Any logConfigJson = json::ParseJson(jsonContent);
        std::map<std::string, Any> items =
            AnyCast<std::map<std::string, Any> >(logConfigJson);
        bool found   = false;
        bool success = true;
        ILoggingSystem* apsaraSystem = GetApsaraLoggingSystem();
        std::map<std::string, Logger*>* loggerMap = GetKeyToLoggerMap();
        ScopedMicroRWLock lock(sRWLock, 'r');
        for (typeof (logsystemMap->end()) it = logsystemMap->begin();
                it != logsystemMap->end(); ++it)
        {
            typeof (items.end()) cit = items.find(it->first);
            if (cit == items.end())
            {
                continue;
            }
            found = true;
            if (!it->second->LoadConfig(json::ToString(cit->second)))
            {
                success = false;
            }
        }
        if (!found)
        {
            // not found config of each system, use as apsara logging config
            if (items.find("Loggers") != items.end())
            {
                success = apsaraSystem->LoadConfig(jsonContent);
            }
        }
        for (typeof(loggerMap->end()) it = loggerMap->begin();
                it != loggerMap->end(); ++it)
        {
            it->second->ReloadLevel();
        }
        return success;
    }
    catch(const ExceptionBase & e)
    {
        fprintf(stderr, "The wrong format LoadConfig parameter %s, reason: %s",
                jsonContent.c_str(), e.what());
        return false;
    }
}

bool LoadConfigFile(const std::string& filePath)
{
    std::ifstream ifs(filePath.c_str());
    if (!ifs)
    {
        return false;
    }
    std::ostringstream os;
    os << ifs.rdbuf();
    std::string content = os.str();
    ifs.close();

    return LoadConfig(content);
}

bool RegisterLoggingSystem(ILoggingSystem* system)
{
    std::map<std::string, ILoggingSystem*>* logsystemMap =
        GetNameToLoggingSystemMap();
    const std::string& name = system->GetName();
    {
        ScopedMicroRWLock lock(sRWLock, 'w');
        if (logsystemMap->size() == MAX_SUPPORT_LOGGING_SYSTEM)
        {
            return false;
        }
        ILoggingSystem** systemPtr = &(*logsystemMap)[name];
        ASSERT_DEBUG(*systemPtr == NULL || *systemPtr == system);
        if (*systemPtr == NULL)
        {
            system->Setup();
            *systemPtr = system;
            std::map<std::string, Logger*>* loggerMap =
                GetKeyToLoggerMap();
            for (typeof(loggerMap->end()) it = loggerMap->begin();
                    it != loggerMap->end(); ++it)
            {
                const std::string& fmtkey = it->first;
                Logger* logger = it->second;
                ILoggerAdaptor* adaptor = system->GetLogger(fmtkey);
                LogLevel level = std::min(logger->GetLevel(),
                        adaptor->GetLogLevel());
                logger->AddAdaptor(adaptor);
            }
        }
        else if (*systemPtr != system)
        {
            fprintf(stderr, ">>> %s duplicated logging system: %p vs %p\n",
                    __FUNCTION__, *systemPtr, system);
        }
    }
    return true;
}

static void DisableOrEnableLoggingSystem(const std::string& name, bool enable)
{
    std::map<std::string, ILoggingSystem*>* logsystemMap
        = GetNameToLoggingSystemMap();
    {
        ScopedMicroRWLock lock(sRWLock, 'w');
        typeof (logsystemMap->end()) it = logsystemMap->find(name);
        if (it == logsystemMap->end())
        {
            return;
        }
        ILoggingSystem* system = it->second;
        std::map<std::string, Logger*>* loggerMap = GetKeyToLoggerMap();
        for (typeof(loggerMap->end()) lit = loggerMap->begin();
                lit != loggerMap->end(); ++lit)
        {
            const std::string& fmtkey = lit->first;
            Logger* logger = lit->second;
            ILoggerAdaptor* adaptor = system->GetLogger(fmtkey);
            if (enable)
            {
                logger->EnableAdaptor(adaptor);
            }
            else
            {
                logger->DisableAdaptor(adaptor);
            }
        }
    }
}

void DisableLoggingSystem(const std::string& name)
{
    DisableOrEnableLoggingSystem(name, false);
}

void EnableLoggingSystem(const std::string& name)
{
    DisableOrEnableLoggingSystem(name, true);
}

std::string GetCurrentTimeInString()
{
    TLSLogHelper* helper = GetTLSLogHelper();
    size_t size = 0;
    const uint64_t* source = reinterpret_cast<const uint64_t*>(
            helper->GetCurrentTimeString(&size));
    // 1970-01-01 00:00:00.000000
    // strlen(result) = 26
    ASSERT_DEBUG(size <= 32 && size >= 24);
    std::string result(32, 0);
    uint64_t* target = reinterpret_cast<uint64_t*>(
            const_cast<char*>(result.data()));
    *target++ = *source++;
    *target++ = *source++;
    *target++ = *source++;
    *target   = *source;
    result.resize(size);
    return result;
}
