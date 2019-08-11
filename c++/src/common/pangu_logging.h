#ifndef APSARA_LOGGING_H
#define APSARA_LOGGING_H

#include <string>
#include <tr1/type_traits>
#include <stdint.h>

#include "anet/ilogger.h"

#include "include/macros.h"
#include "include/string_formater.h"

#ifndef MAKE_APSARA_LOGGING_HAPPY
// disable load common logging, we implement pangu's logging system here
#define APSARA_LOGGING_H
#endif

#define DECLARE_SLOGGER(logger, key) \
    static ::apsara::pangu::Logger* logger = ::apsara::pangu::Logger::GetLogger(key)

#define GET_SLOGGER(key) \
    ::apsara::pangu::Logger::GetLogger(key)

// to disable pangu logging by loglevel, set DISABLE_LOGGING_XXX to 1
#ifndef NDEBUG // debug mode
    #define DISABLE_LOGGING_DEBUG     0
    #define DISABLE_LOGGING_INFO      0
    #define DISABLE_LOGGING_WARNING   0
    #define DISABLE_LOGGING_ERROR     0
    #define DISABLE_LOGGING_FATAL     0
    #define DISABLE_LOGGING_PROFILE   0
#else // release mode
    #define DISABLE_LOGGING_DEBUG     1
    #define DISABLE_LOGGING_INFO      0
    #define DISABLE_LOGGING_WARNING   0
    #define DISABLE_LOGGING_ERROR     0
    #define DISABLE_LOGGING_FATAL     0
    #define DISABLE_LOGGING_PROFILE   0
#endif

namespace apsara {

// just make common library's header file happy :(
namespace logging {
struct Logger;
struct LogRecord;
}

namespace pangu {

typedef ::ILogger IEasylogAdapter;

enum LogLevel
{
    LOG_LEVEL_ALL     = 0,      // enable all logs
    LOG_LEVEL_PROFILE = 100,
    LOG_LEVEL_DEBUG   = 200,
    LOG_LEVEL_INFO    = 300,
    LOG_LEVEL_WARNING = 400,
    LOG_LEVEL_ERROR   = 500,
    LOG_LEVEL_FATAL   = 600,
    LOG_LEVEL_NONE    = 10000,  // disable all logs
    LOG_LEVEL_DEFAULT = LOG_LEVEL_INFO,
    LOG_LEVEL_INHERIT = -1      // inherit from the parent
};

enum LogLevelIndex
{
    LOG_LEVEL_INDEX_ALL     = 0,
    LOG_LEVEL_INDEX_PROFILE = 1,
    LOG_LEVEL_INDEX_DEBUG   = 2,
    LOG_LEVEL_INDEX_INFO    = 3,
    LOG_LEVEL_INDEX_WARNING = 4,
    LOG_LEVEL_INDEX_ERROR   = 5,
    LOG_LEVEL_INDEX_FATAL   = 6,
    LOG_LEVEL_INDEX_NONE    = 7,  // disable all logs, must be last item
};

struct Variant
{
    enum Type { BOOL, INT64, DOUBLE, STRING };

    Type type;

    union
    {
        bool boolValue;
        int64_t int64Value;
        double doubleValue;
    };

    std::string stringValue;
};

struct LogPair
{
    std::string key;
    Variant value;
};

struct LoggingHeader
{
    const char* filename;
    const char* function;
    int line;
    LogLevel level;
    int headerLen;
    uint32_t tid;
};

class RefCountedLoggingData
{
public:
    RefCountedLoggingData(const char* data, uint32_t length, uint32_t refCnt)
    {
        mData = data;
        mLength = length;
        mRefCnt = refCnt;
    }
    void SubRef(uint32_t refCnt)
    {
        if (__sync_sub_and_fetch(&mRefCnt, refCnt) == 0)
        {
            free(const_cast<char*>(mData));
        }
    }
    void Release()
    {
        SubRef(1U);
    }
    const char* data() { return mData;   }
    size_t size()      { return mLength; }
private:
    const char* mData;
    uint32_t mLength;
    uint32_t mRefCnt;
};

class ILoggerAdaptor
{
public:
    virtual ~ILoggerAdaptor() {}

    /*
     * append certain log data
     * @param logging data protected by refCount
     *  user should call loggingData->Release() when no data required
     *  the callee's thread take charge of lots of works, and **IT IS**
     *  user-defined logging system's **RESPONSIBILITY** for not blocking thread
     */
    virtual void AppendLog(const LoggingHeader& header, RefCountedLoggingData* loggingData) = 0;

    /*
     * return log level for this adaptor
     * It is LoggingSystem's responsible to set the Logger's level or update when LoadConfig
     */
    LogLevel GetLogLevel() { return mLogLevel; }
    bool IsLevelEnabled(LogLevel level) { return mLogLevel <= level; }

protected:
    LogLevel mLogLevel;
};

class ILoggingSystem
{
public:
    ILoggingSystem(const std::string& name);
    virtual ~ILoggingSystem();

    /*
     * @brief, GetLogger for certain key
     *   Ensure same key return same ILoggerAdaptor
     */
    virtual ILoggerAdaptor* GetLogger(const std::string& key) = 0;

    const std::string& GetName() { return mName; }

    /*
     * Called Only **ONCE**, just before create any ILoggerAdaptor instance
     */
    virtual void Setup() = 0;
    /*
     * Reload configuration
     * @return: true for success and false otherwise
     */
    virtual bool LoadConfig(const std::string& jsonContent) = 0;
    /*
     * Flush all logging data from memory to disk
     */
    virtual void FlushLog() = 0;
    /*
     * Called Only **ONCE**, just before close logging system
     */
    virtual void TearDown() = 0;

private:
    std::string mName;
};

std::string StringPrintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

void InitLoggingSystem();
void UninitLoggingSystem();

void FlushLog();
void RotateLogFiles(const std::string& path, bool bCompress, int maxFileNum, int maxDay, std::string* tmpFile = NULL);

bool LoadConfig(const std::string& jsonContent);
bool LoadConfigFile(const std::string&  filePath="");
void ReloadLogLevel();

/*
 * @brief, format 2018-03-22 10:10:10.123456
 */
std::string GetCurrentTimeInString();

/*
 *@brief: register a new logging system to pangu, all later logging data
 *        will be send it by LoggerAdaptor's AppendLog method
 *@return: true for success or false when too many registed logging system
 */
bool RegisterLoggingSystem(ILoggingSystem* system);

/*
 *@brief: disable a exists logging system, just disable later logging data
 *        output, when this method return, some ongoing logging data
 *        may still be sent by LoggerAdaptor's AppendLog method
 */
void DisableLoggingSystem(const std::string& name);
/*
 *@brief: enable a exists logging system
 */
void EnableLoggingSystem(const std::string& name);

IEasylogAdapter* CreateEasylogAdapter(const std::string& loggerName);

ILoggingSystem* CreateApsaraLoggingSystem();

#ifdef SHOW_EASYUTHREAD_IN_LOGGING
#define UTHREAD_ADDR_FIELD ("uthread", easy_uthread_current())
#else
#define UTHREAD_ADDR_FIELD
#endif

#define PGLOG_X_DUMMY_BEGIN                                                   \
    do {                                                                      \
        ::apsara::pangu::LogMaker* __currentInternalLogMakerVar__ = NULL;     \
        StringPieceFormatter* __currentInternalLogFormatter__ = NULL;

#define PGLOG_X_DUMMY_END                                                     \
    } while (0)

// PGLOG_DEBUG
#if defined(DISABLE_LOGGING_DEBUG) && DISABLE_LOGGING_DEBUG == 1
#define PGLOG_DEBUG(...)        /* nothing to do */
#define PGLOG_DEBUG_IF(...)     /* nothing to do */
#define PGLOG_CALL_DEBUG(...)   /* nothing to do */
#define PGLOG_DEBUG_BEGIN(...)  PGLOG_X_DUMMY_BEGIN
#define PGLOG_DEBUG_END         PGLOG_X_DUMMY_END
#else
#define PGLOG_DEBUG(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_DEBUG, UNLIKELY)
#define PGLOG_DEBUG_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_DEBUG, UNLIKELY)
#define PGLOG_CALL_DEBUG(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_DEBUG, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#define PGLOG_DEBUG_BEGIN(logger, ...) \
    PGLOG_X_BEGIN(logger, ::apsara::pangu::LOG_LEVEL_DEBUG, UNLIKELY, ##__VA_ARGS__)
#define PGLOG_DEBUG_END \
    PGLOG_X_END
#endif

// PGLOG_INFO
#if defined(DISABLE_LOGGING_INFO) && DISABLE_LOGGING_INFO == 1
#define PGLOG_INFO(...)       /* nothing to do */
#define PGLOG_INFO_IF(...)    /* nothing to do */
#define PGLOG_CALL_INFO(...)  /* nothing to do */
#define PGLOG_INFO_BEGIN(...) PGLOG_X_DUMMY_BEGIN
#define PGLOG_INFO_END        PGLOG_X_DUMMY_END
#else
#define PGLOG_INFO(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_INFO, UNLIKELY)
#define PGLOG_INFO_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_INFO, UNLIKELY)
#define PGLOG_CALL_INFO(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_INFO, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#define PGLOG_INFO_BEGIN(logger, ...) \
    PGLOG_X_BEGIN(logger, ::apsara::pangu::LOG_LEVEL_INFO, UNLIKELY, ##__VA_ARGS__)
#define PGLOG_INFO_END \
    PGLOG_X_END
#endif

// PGLOG_WARNING
#if defined(DISABLE_LOGGING_WARNING) && DISABLE_LOGGING_WARNING == 1
#define PGLOG_WARNING(...)       /* nothing to do */
#define PGLOG_WARNING_IF(...)    /* nothing to do */
#define PGLOG_CALL_WARNING(...)  /* nothing to do */
#define PGLOG_WARNING_BEGIN(...) PGLOG_X_DUMMY_BEGIN
#define PGLOG_WARNING_END        PGLOG_X_DUMMY_END
#else
#define PGLOG_WARNING(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_WARNING, LIKELY)
#define PGLOG_WARNING_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_WARNING, LIKELY)
#define PGLOG_CALL_WARNING(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_WARNING, LIKELY, UTHREAD_ADDR_FIELD fields)
#define PGLOG_WARNING_BEGIN(logger, ...) \
    PGLOG_X_BEGIN(logger, ::apsara::pangu::LOG_LEVEL_WARNING, LIKELY, ##__VA_ARGS__)
#define PGLOG_WARNING_END \
    PGLOG_X_END
#endif

// PGLOG_ERROR
#if defined(DISABLE_LOGGING_ERROR) && DISABLE_LOGGING_ERROR == 1
#define PGLOG_ERROR(...)       /* nothing to do */
#define PGLOG_ERROR_IF(...)    /* nothing to do */
#define PGLOG_CALL_ERROR(...)  /* nothing to do */
#define PGLOG_ERROR_BEGIN(...) PGLOG_X_DUMMY_BEGIN
#define PGLOG_ERROR_END        PGLOG_X_DUMMY_END
#else
#define PGLOG_ERROR(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_ERROR, LIKELY)
#define PGLOG_ERROR_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_ERROR, LIKELY)
#define PGLOG_CALL_ERROR(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_ERROR, LIKELY, UTHREAD_ADDR_FIELD fields)
#define PGLOG_ERROR_BEGIN(logger, ...) \
    PGLOG_X_BEGIN(logger, ::apsara::pangu::LOG_LEVEL_ERROR, LIKELY, ##__VA_ARGS__)
#define PGLOG_ERROR_END \
    PGLOG_X_END
#endif

// PGLOG_FATAL
#if defined(DISABLE_LOGGING_FATAL) && DISABLE_LOGGING_FATAL == 1
#define PGLOG_FATAL(...)       /* nothing to do */
#define PGLOG_FATAL_IF(...)    /* nothing to do */
#define PGLOG_CALL_FATAL(...)  /* nothing to do */
#define PGLOG_FATAL_BEGIN(...) PGLOG_X_DUMMY_BEGIN
#define PGLOG_FATAL_END        PGLOG_X_DUMMY_END
#else
#define PGLOG_FATAL(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_FATAL, LIKELY)
#define PGLOG_FATAL_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_FATAL, LIKELY)
#define PGLOG_CALL_FATAL(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_FATAL, LIKELY, UTHREAD_ADDR_FIELD fields)
#define PGLOG_FATAL_BEGIN(logger, ...) \
    PGLOG_X_BEGIN(logger, ::apsara::pangu::LOG_LEVEL_FATAL, LIKELY, ##__VA_ARGS__)
#define PGLOG_FATAL_END \
    PGLOG_X_END
#endif

// PGLOG_PROFILE
#if defined(DISABLE_LOGGING_PROFILE) && DISABLE_LOGGING_PROFILE == 1
#define PGLOG_PROFILE(...)      /* nothing to do */
#define PGLOG_PROFILE_IF(...)   /* nothing to do */
#define PGLOG_CALL_PROFILE(...) /* nothing to do */
#else
#define PGLOG_PROFILE(logger, fields)   \
    PGLOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_PROFILE, UNLIKELY)
#define PGLOG_PROFILE_IF(logger, condition, fields) \
    PGLOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, ::apsara::pangu::LOG_LEVEL_PROFILE, UNLIKELY)
#define PGLOG_CALL_PROFILE(logger, fields...) \
    PGLOG_CALL(logger, ::apsara::pangu::LOG_LEVEL_PROFILE, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#endif

#define LOGF_PROFILE(...) \
        PGLOG_PROFILE(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_DEBUG(...) \
        PGLOG_DEBUG(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_INFO(...) \
        PGLOG_INFO(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_WARNING(...) \
        PGLOG_WARNING(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_ERROR(...) \
        PGLOG_ERROR(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_FATAL(...) \
        PGLOG_FATAL(sLogger, ("", StringPrintf(__VA_ARGS__)))

#ifndef TRANSLATE_LINE_TO_STR
#define TRANSLATE_LINEINT_TO_STR(count)  #count
#define TRANSLATE_LINE_TO_STR(count) TRANSLATE_LINEINT_TO_STR(count)
#endif

#define PGLOG_QPS_X(logger, throttle, LOG_METHOD, fields)                           \
    do                                                                              \
    {                                                                               \
        uint64_t waitTime;                                                          \
        if (throttle.TryDispatch(1, easy_io_time(), &waitTime))                     \
        {                                                                           \
            LOG_METHOD(logger, fields);                                             \
        }                                                                           \
    } while (false)

#define PGLOG_QPS(logger, qps, LOG_METHOD, fields)                                  \
    do                                                                              \
    {                                                                               \
        static ::apsara::pangu::GeneralThrottle sLoggingThrottle(qps, 1000000,      \
                __FILE__ ":" TRANSLATE_LINE_TO_STR(__LINE__));                      \
        PGLOG_QPS_X(logger, sLoggingThrottle, LOG_METHOD, fields);                  \
    } while (false)

#define PGLOG_INFO_QPS50(logger,   fields)    PGLOG_QPS(logger, 50,   PGLOG_INFO, fields)
#define PGLOG_INFO_QPS100(logger,  fields)    PGLOG_QPS(logger, 100,  PGLOG_INFO, fields)
#define PGLOG_INFO_QPS1000(logger, fields)    PGLOG_QPS(logger, 1000, PGLOG_INFO, fields)

#define PGLOG_WARNING_QPS50(logger,   fields) PGLOG_QPS(logger, 50,   PGLOG_WARNING, fields)
#define PGLOG_WARNING_QPS100(logger,  fields) PGLOG_QPS(logger, 100,  PGLOG_WARNING, fields)
#define PGLOG_WARNING_QPS1000(logger, fields) PGLOG_QPS(logger, 1000, PGLOG_WARNING, fields)

#define PGLOG_ERROR_QPS50(logger,   fields)   PGLOG_QPS(logger, 50,   PGLOG_ERROR, fields)
#define PGLOG_ERROR_QPS100(logger,  fields)   PGLOG_QPS(logger, 100,  PGLOG_ERROR, fields)
#define PGLOG_ERROR_QPS1000(logger, fields)   PGLOG_QPS(logger, 1000, PGLOG_ERROR, fields)

#define PGLOG_INFO_QPS(logger, throttle, fields)    PGLOG_QPS_X(logger, throttle, PGLOG_INFO, fields)
#define PGLOG_WARNING_QPS(logger, throttle, fields) PGLOG_QPS_X(logger, throttle, PGLOG_WARNING, fields)
#define PGLOG_ERROR_QPS(logger, throttle, fields)   PGLOG_QPS_X(logger, throttle, PGLOG_ERROR, fields)

// ===================== ALL Logging API **END** Here =================  //
// ********************************************************************* //
// ================== ALL Logging Helper **FROM ** Here ================ //

// helper class for logging, cannot used by user
template<typename T>
struct LoggingFieldNonPODTypeHelper {};

template<bool IsPod, typename T>
struct LoggingFieldTypeEncoderHelper;

template<typename T>
struct LoggingFieldTypeEncoderHelper<false, T>
{
    typedef const LoggingFieldNonPODTypeHelper<T>* FieldType;
    typedef const T& FieldParamType;
    static inline FieldType Encode(const T& value)
    {
        return reinterpret_cast<FieldType>(&value);
    }
};

template<typename T>
struct LoggingFieldTypeEncoderHelper<true, T>
{
    typedef typename std::tr1::remove_cv<T>::type FieldType;
    typedef FieldType FieldParamType;
    static FieldType Encode(FieldType value) { return value; }
};

template<typename T>
struct LoggingFieldTypeEncoder
{
    typedef LoggingFieldTypeEncoderHelper<std::tr1::is_scalar<T>::value, T>
        LoggingFieldTypeEncoderHelperType;
    typedef typename LoggingFieldTypeEncoderHelperType::FieldType FieldType;
    typedef typename LoggingFieldTypeEncoderHelperType::FieldParamType
        FieldParamType;
    static inline FieldType Encode(const FieldParamType value)
    {
        return LoggingFieldTypeEncoderHelperType::Encode(value);
    }
};

template<typename T>
struct LoggingFieldTypeDecodeHelper
{
    typedef T FieldType;
    static FieldType Decode(FieldType value) { return value; }
};

template<typename T>
struct LoggingFieldTypeDecodeHelper<const LoggingFieldNonPODTypeHelper<T>*>
{
    typedef const T& FieldType;
    static inline FieldType Decode(const LoggingFieldNonPODTypeHelper<T>* value)
    {
        return *reinterpret_cast<const T*>(value);
    }
};

template<typename T>
struct LoggingFieldTypeDecoder
{
    typedef typename LoggingFieldTypeDecodeHelper<T>::FieldType
        AppendDecodeParamType;
    static inline AppendDecodeParamType Decode(T value)
    {
        return LoggingFieldTypeDecodeHelper<T>::Decode(value);
    }
};

#define PGLOG_REPEAT_CODE_1(macro, ...)                                       \
    macro(1, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_2(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_3(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_4(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_5(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_6(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_7(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_8(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_9(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__) macro(9, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_10(macro, ...)                                      \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__) macro(9, __VA_ARGS__)         \
    macro(10, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_11(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_12(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \

#define PGLOG_REPEAT_CODE_13(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_14(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_15(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_16(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_17(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_18(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_19(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_20(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_21(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_22(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_23(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_24(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__) macro(24, __VA_ARGS__)

#define PGLOG_REPEAT_CODE_25(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__) macro(24, __VA_ARGS__)      \
    macro(25, __VA_ARGS__)

#define PGLOG_CAT(a, ...)        a ## __VA_ARGS__
#define PGLOG_EVAL(...)          __VA_ARGS__
#define PGLOG_IIF(cond)          PGLOG_IIF_ ## cond()
#define PGLOG_IIF_1(...)         1, 0
#define PGLOG_COMMA_1            ,
#define PGLOG_COMMA_0
#define PGLOG_SECOND(a, b, ...)  PGLOG_COMMA_ ## b
#define PGLOG_CHECK_PARAM(...)   PGLOG_SECOND(__VA_ARGS__, 1)
#define PGLOG_ADD_COMMA(a)       PGLOG_CHECK_PARAM(PGLOG_IIF(a))

#define PGLOG_TYPENAME_DECLARE(index, ...)                                    \
    PGLOG_ADD_COMMA(index) typename P ## index, typename V ## index

#define PGLOG_CTOR_TYPEVAR_DECLARE(index, ...)                                \
    PGLOG_ADD_COMMA(index) const P ## index& p ## index ## _,                 \
        const V ## index& v ## index ## _

#define PGLOG_PARAM_TYPEVAR_DECLARE(index, ...)                               \
    PGLOG_ADD_COMMA(index) const P ## index& p ## index,                      \
        const V ## index& v ## index

#define PGLOG_VARTYPE_DECLARE(index, ...)                                     \
    PGLOG_ADD_COMMA(index) P ## index, V ## index

#define PGLOG_APPEND_PARAM_INIT(index, ...)                                   \
    p ## index = p ## index ## _; v ## index = v ## index ## _;

#define PGLOG_APPEND_PARAM_DECLARE(index, ...)                                \
    P ## index p ## index; V ## index v ## index;

#define PGLOG_APPEND_PARAM_CTOR_LIST(index, ...)                              \
    PGLOG_ADD_COMMA(index)                                                    \
        LoggingFieldTypeEncoder<P ## index>::Encode(p ## index),              \
            LoggingFieldTypeEncoder<V ## index>::Encode(v ## index)

#define PGLOG_APPEND_PARAM_VARTYPE_DECLARE(index, ...)                        \
    PGLOG_ADD_COMMA(index)                                                    \
        typename LoggingFieldTypeEncoder<P ## index>::FieldType,              \
            typename LoggingFieldTypeEncoder<V ## index>::FieldType

#define PGLOG_APPEND_PARAM_DUMP(index, ...)                                   \
    formatter(LoggingFieldTypeDecoder<P ## index>::Decode(param.p ## index),  \
        LoggingFieldTypeDecoder<V ## index>::Decode(param.v ## index));

#define PGLOG_DEFINE_APPEND_PARAM(i)                                          \
    template<PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                      \
            (PGLOG_TYPENAME_DECLARE))>                                        \
    struct LoggingAppendParam##i                                              \
    {                                                                         \
        LoggingAppendParam##i(PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)     \
            (PGLOG_CTOR_TYPEVAR_DECLARE)))                                    \
        __attribute__((always_inline))                                        \
        {                                                                     \
            PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                       \
                (PGLOG_APPEND_PARAM_INIT));                                   \
        }                                                                     \
        PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                           \
                (PGLOG_APPEND_PARAM_DECLARE));                                \
    };

PGLOG_DEFINE_APPEND_PARAM(1);   PGLOG_DEFINE_APPEND_PARAM(2);
PGLOG_DEFINE_APPEND_PARAM(3);   PGLOG_DEFINE_APPEND_PARAM(4);
PGLOG_DEFINE_APPEND_PARAM(5);   PGLOG_DEFINE_APPEND_PARAM(6);
PGLOG_DEFINE_APPEND_PARAM(7);   PGLOG_DEFINE_APPEND_PARAM(8);
PGLOG_DEFINE_APPEND_PARAM(9);   PGLOG_DEFINE_APPEND_PARAM(10);
PGLOG_DEFINE_APPEND_PARAM(11);  PGLOG_DEFINE_APPEND_PARAM(12);
PGLOG_DEFINE_APPEND_PARAM(13);  PGLOG_DEFINE_APPEND_PARAM(14);
PGLOG_DEFINE_APPEND_PARAM(15);  PGLOG_DEFINE_APPEND_PARAM(16);
PGLOG_DEFINE_APPEND_PARAM(17);  PGLOG_DEFINE_APPEND_PARAM(18);
PGLOG_DEFINE_APPEND_PARAM(19);  PGLOG_DEFINE_APPEND_PARAM(20);
PGLOG_DEFINE_APPEND_PARAM(21);  PGLOG_DEFINE_APPEND_PARAM(22);
PGLOG_DEFINE_APPEND_PARAM(23);

#define PGLOG_NUMBER_SUB_2(i) PGLOG_CAT(PGLOG_NUMBER_SUB_2_, i)
#define PGLOG_NUMBER_SUB_2_3   1
#define PGLOG_NUMBER_SUB_2_4   2
#define PGLOG_NUMBER_SUB_2_5   3
#define PGLOG_NUMBER_SUB_2_6   4
#define PGLOG_NUMBER_SUB_2_7   5
#define PGLOG_NUMBER_SUB_2_8   6
#define PGLOG_NUMBER_SUB_2_9   7
#define PGLOG_NUMBER_SUB_2_10  8
#define PGLOG_NUMBER_SUB_2_11  9
#define PGLOG_NUMBER_SUB_2_12  10
#define PGLOG_NUMBER_SUB_2_13  11
#define PGLOG_NUMBER_SUB_2_14  12
#define PGLOG_NUMBER_SUB_2_15  13
#define PGLOG_NUMBER_SUB_2_16  14
#define PGLOG_NUMBER_SUB_2_17  15
#define PGLOG_NUMBER_SUB_2_18  16
#define PGLOG_NUMBER_SUB_2_19  17
#define PGLOG_NUMBER_SUB_2_20  18
#define PGLOG_NUMBER_SUB_2_21  19
#define PGLOG_NUMBER_SUB_2_22  20
#define PGLOG_NUMBER_SUB_2_23  21
#define PGLOG_NUMBER_SUB_2_24  22
#define PGLOG_NUMBER_SUB_2_25  23

#define PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_X(i)                               \
        LoggingAppendParam##i<PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)     \
            (PGLOG_APPEND_PARAM_VARTYPE_DECLARE))>                            \
                param(PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)             \
                    (PGLOG_APPEND_PARAM_CTOR_LIST)));
#define PGLOG_CREATE_APPEND_LOG_TMPL_PARAM(...)                               \
    PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_X(__VA_ARGS__)

#define PGLOG_CREATE_APPEND_LOG_API(i, ix)                                    \
    template<PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                      \
            (PGLOG_TYPENAME_DECLARE))>                                        \
        __attribute__((always_inline))                                        \
    void AppendLog(PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                \
            (PGLOG_PARAM_TYPEVAR_DECLARE)))                                   \
    {                                                                         \
        PGLOG_CREATE_APPEND_LOG_TMPL_PARAM(PGLOG_NUMBER_SUB_2(i));            \
        appendLog(param, LoggingFieldTypeEncoder<P ## ix>::Encode(p ## ix),   \
                LoggingFieldTypeEncoder<V ## ix>::Encode(v ## ix),            \
                LoggingFieldTypeEncoder<P ## i>::Encode(p ## i),              \
                LoggingFieldTypeEncoder<V ## i>::Encode(v ## i));             \
    }

#define PGLOG_CREATE_APPEND_LOG_IMPL_DUMP_PARAM(i)                            \
    PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)(PGLOG_APPEND_PARAM_DUMP))

#define PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE_X(i)                          \
    const LoggingAppendParam##i<PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)   \
        (PGLOG_VARTYPE_DECLARE))>& param

#define PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE(...)                          \
    PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE_X(__VA_ARGS__)

#define PGLOG_CREATE_APPEND_LOG_IMPL(i, ix)                                   \
    template<PGLOG_EVAL(PGLOG_CAT(PGLOG_REPEAT_CODE_, i)                      \
            (PGLOG_TYPENAME_DECLARE))>                                        \
        __attribute__((noinline))                                             \
    void appendLog(                                                           \
            PGLOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE(PGLOG_NUMBER_SUB_2(i)),   \
                const P ## ix p ## ix, const V ## ix v ## ix,                 \
                const P ## i p ## i, const V ## i v ## i)                     \
    {                                                                         \
        StringPieceFormatter formatter(1024, ":\t");                          \
        TLSLogHelper* helper = MakeLogHeader(formatter);                      \
        if (LIKELY(helper))                                             \
        {                                                                     \
            PGLOG_CREATE_APPEND_LOG_IMPL_DUMP_PARAM(PGLOG_NUMBER_SUB_2(i));   \
            formatter(LoggingFieldTypeDecoder<P ## ix>::Decode(p ## ix),      \
                    LoggingFieldTypeDecoder<V ## ix>::Decode(v ## ix))        \
                (LoggingFieldTypeDecoder<P ## i>::Decode(p ## i),             \
                 LoggingFieldTypeDecoder<V ## i>::Decode(v ## i));            \
            WriteLogToSink(formatter, helper);                                \
        }                                                                     \
    }

enum { MAX_SUPPORT_LOGGING_SYSTEM = 7 };

class TLSLogHelper;

struct ConstLoggingHeader
{
    const char* filename;
    const char* function;
    int line;
    LogLevel level;
};

class Logger
{
public:
    static Logger* GetLogger(const std::string& key);
    void AppendLog(const ConstLoggingHeader& header, size_t headerLen,
            size_t tid, const char* data, uint32_t datalen, uint32_t capacity);
    LogLevel GetLevel() { return mMinLevel; }
    void SetLevel(LogLevel level) { mMinLevel = level; }
    void ReloadLevel();
    static uint64_t GetGlobalCounter(LogLevel level);
    void AddAdaptor(ILoggerAdaptor* adaptor);
    void DisableAdaptor(ILoggerAdaptor* adaptor);
    void EnableAdaptor(ILoggerAdaptor* adaptor);

private:
    Logger();

    LogLevel mMinLevel;
    ILoggerAdaptor* mAdaptorList[MAX_SUPPORT_LOGGING_SYSTEM + 1];
};

class LogMaker
{
public:
    friend struct Logger;
    friend struct LogCallTracer;

    LogMaker(Logger* logger, const char* filename, int line,
            const char* function, LogLevel level);
    ~LogMaker();

    /**
     * @brief, append field to a LogMaker
     */
    template <typename P1, typename V1>
        __attribute__((always_inline))
    void AppendField(StringPieceFormatter& formatter, const P1& p1, const V1& v1)
    {
        appendField(formatter,
                LoggingFieldTypeEncoder<P1>::Encode(p1),
                LoggingFieldTypeEncoder<V1>::Encode(v1));
    }

    /**
     * @brief, record a completed log and send it to sink directly
     */
    void AppendLog() __attribute__((noinline))
    {
        StringPieceFormatter formatter(1024, ":\t");
        TLSLogHelper* helper = MakeLogHeader(formatter);
        if (LIKELY(helper))
        {
            WriteLogToSink(formatter, helper);
        }
    }
    template <typename P1, typename V1>
        __attribute__((always_inline))
    void AppendLog(const P1& p1, const V1& v1)
    {
        appendLog(LoggingFieldTypeEncoder<P1>::Encode(p1),
                LoggingFieldTypeEncoder<V1>::Encode(v1));
    }
    template <typename P1, typename V1, typename P2, typename V2>
        __attribute__((always_inline))
    void AppendLog(const P1& p1, const V1& v1, const P2& p2, const V2& v2)
    {
        appendLog(LoggingFieldTypeEncoder<P1>::Encode(p1),
                LoggingFieldTypeEncoder<V1>::Encode(v1),
                LoggingFieldTypeEncoder<P2>::Encode(p2),
                LoggingFieldTypeEncoder<V2>::Encode(v2));
    }

    PGLOG_CREATE_APPEND_LOG_API(3,  2);  PGLOG_CREATE_APPEND_LOG_API(4,  3);
    PGLOG_CREATE_APPEND_LOG_API(5,  4);  PGLOG_CREATE_APPEND_LOG_API(6,  5);
    PGLOG_CREATE_APPEND_LOG_API(7,  6);  PGLOG_CREATE_APPEND_LOG_API(8,  7);
    PGLOG_CREATE_APPEND_LOG_API(9,  8);  PGLOG_CREATE_APPEND_LOG_API(10, 9);
    PGLOG_CREATE_APPEND_LOG_API(11, 10); PGLOG_CREATE_APPEND_LOG_API(12, 11);
    PGLOG_CREATE_APPEND_LOG_API(13, 12); PGLOG_CREATE_APPEND_LOG_API(14, 13);
    PGLOG_CREATE_APPEND_LOG_API(15, 14); PGLOG_CREATE_APPEND_LOG_API(16, 15);
    PGLOG_CREATE_APPEND_LOG_API(17, 16); PGLOG_CREATE_APPEND_LOG_API(18, 17);
    PGLOG_CREATE_APPEND_LOG_API(19, 18); PGLOG_CREATE_APPEND_LOG_API(20, 19);
    PGLOG_CREATE_APPEND_LOG_API(21, 20); PGLOG_CREATE_APPEND_LOG_API(22, 21);
    PGLOG_CREATE_APPEND_LOG_API(23, 22); PGLOG_CREATE_APPEND_LOG_API(24, 23);
    PGLOG_CREATE_APPEND_LOG_API(25, 24);

    /*
     * @brief, output header information to formatter
     * @return: true for EnableThreadLog and otherwise, false
     */
    TLSLogHelper* MakeLogHeader(StringPieceFormatter& formatter);

    /*
     * @brief, last step for record log, put it to sink now,
     */
    void WriteLogToSink(StringPieceFormatter& formatter, TLSLogHelper* helper);

private:
    template <typename P1, typename V1>
        __attribute__((noinline))
    void appendField(StringPieceFormatter& formatter, const P1 p1, const V1 v1)
    {
        formatter(LoggingFieldTypeDecoder<P1>::Decode(p1),
                LoggingFieldTypeDecoder<V1>::Decode(v1));
    }

    template <typename P1, typename V1>
        __attribute__((noinline))
    void appendLog(const P1 p1, const V1 v1)
    {
        StringPieceFormatter formatter(1024, ":\t");
        TLSLogHelper* helper = MakeLogHeader(formatter);
        if (LIKELY(helper))
        {
            formatter(LoggingFieldTypeDecoder<P1>::Decode(p1),
                    LoggingFieldTypeDecoder<V1>::Decode(v1));
            WriteLogToSink(formatter, helper);
        }
    }
    template <typename P1, typename V1, typename P2, typename V2>
        __attribute__((noinline))
    void appendLog(const P1 p1, const V1 v1, const P2 p2, const V2 v2)
    {
        StringPieceFormatter formatter(1024, ":\t");
        TLSLogHelper* helper = MakeLogHeader(formatter);
        if (LIKELY(helper))
        {
            formatter(LoggingFieldTypeDecoder<P1>::Decode(p1),
                    LoggingFieldTypeDecoder<V1>::Decode(v1))
                (LoggingFieldTypeDecoder<P2>::Decode(p2),
                 LoggingFieldTypeDecoder<V2>::Decode(v2));
            WriteLogToSink(formatter, helper);
        }
    }

    PGLOG_CREATE_APPEND_LOG_IMPL(3,  2);  PGLOG_CREATE_APPEND_LOG_IMPL(4,  3);
    PGLOG_CREATE_APPEND_LOG_IMPL(5,  4);  PGLOG_CREATE_APPEND_LOG_IMPL(6,  5);
    PGLOG_CREATE_APPEND_LOG_IMPL(7,  6);  PGLOG_CREATE_APPEND_LOG_IMPL(8,  7);
    PGLOG_CREATE_APPEND_LOG_IMPL(9,  8);  PGLOG_CREATE_APPEND_LOG_IMPL(10, 9);
    PGLOG_CREATE_APPEND_LOG_IMPL(11, 10); PGLOG_CREATE_APPEND_LOG_IMPL(12, 11);
    PGLOG_CREATE_APPEND_LOG_IMPL(13, 12); PGLOG_CREATE_APPEND_LOG_IMPL(14, 13);
    PGLOG_CREATE_APPEND_LOG_IMPL(15, 14); PGLOG_CREATE_APPEND_LOG_IMPL(16, 15);
    PGLOG_CREATE_APPEND_LOG_IMPL(17, 16); PGLOG_CREATE_APPEND_LOG_IMPL(18, 17);
    PGLOG_CREATE_APPEND_LOG_IMPL(19, 18); PGLOG_CREATE_APPEND_LOG_IMPL(20, 19);
    PGLOG_CREATE_APPEND_LOG_IMPL(21, 20); PGLOG_CREATE_APPEND_LOG_IMPL(22, 21);
    PGLOG_CREATE_APPEND_LOG_IMPL(23, 22); PGLOG_CREATE_APPEND_LOG_IMPL(24, 22);
    PGLOG_CREATE_APPEND_LOG_IMPL(25, 24);

    Logger* mLogger;
    ConstLoggingHeader mLogHeader;
    uint64_t mLineCode;
    uint32_t mFileNameLen;
    uint32_t mLineCodeSize;
    uint32_t mHeaderNoTidLen;
    LogLevelIndex mLogLevelIndex;
};

class LogCallTracer
{
public:
    LogCallTracer(Logger* logger, LogLevel level, LogMaker* maker)
    {
        mLogMaker = NULL;
        if (UNLIKELY(logger->GetLevel() <= level))
        {
            mLogMaker = maker;
        }
    }
    bool EnableLogging()
    {
        return mLogMaker != NULL;
    }
    ~LogCallTracer()
    {
        if (UNLIKELY(mLogMaker))
        {
            recordLogCallLeave();
        }
    }
private:
    void recordLogCallLeave();
    LogMaker* mLogMaker;
};

struct StaticLogMakerCreator
{
    static ::apsara::pangu::LogMaker* Create(LogMaker** makerptr,
            Logger* logger, LogLevel level, const char* file,
            int line, const char* func) __attribute__((noinline))
    {
        if (*makerptr != NULL)
        {
            return *makerptr;
        }
        LogMaker* t = new ::apsara::pangu::LogMaker(logger,
                file, line, func, level);
        if (!__sync_bool_compare_and_swap(makerptr, (LogMaker*)NULL, t))
        {
            delete t;
        }
        return *makerptr;
    }
};

} // namespace pangu
} // namespace apsara

#define PGLOG_UNPACK_DEFER(...) __VA_ARGS__

#define PGLOG_GET_NEXT_OPERATION(x, op, ...)    op
#define PGLOG_GET_THIS_OPERATION(x, y, op, ...) op

#define PGLOG_OPERATION_COMMA_0
#define PGLOG_OPERATION_COMMA_1 ,

#define PGLOG_OPERATION_DUMP(cond, a, b)  PGLOG_OPERATION_COMMA_ ## cond a, b
#define PGLOG_OPERATION_DUMP_DONE(...)
#define PGLOG_OPERATION_NEXT_DONE

#define PGLOG_UNPACK_FIELD(cond, a, b, ...)                                 \
    PGLOG_GET_THIS_OPERATION(~, ##__VA_ARGS__, PGLOG_OPERATION_DUMP,        \
            PGLOG_OPERATION_DUMP_DONE)(cond, a, b)                          \
    PGLOG_GET_NEXT_OPERATION(~, ##__VA_ARGS__, PGLOG_OPERATION_NEXT_DONE)

#define PGLOG_UNPACK_1(...) PGLOG_UNPACK_FIELD(0, __VA_ARGS__, PGLOG_UNPACK_2)
#define PGLOG_UNPACK_2(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_3)
#define PGLOG_UNPACK_3(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_4)
#define PGLOG_UNPACK_4(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_5)
#define PGLOG_UNPACK_5(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_6)
#define PGLOG_UNPACK_6(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_7)
#define PGLOG_UNPACK_7(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_8)
#define PGLOG_UNPACK_8(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_9)
#define PGLOG_UNPACK_9(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_a)
#define PGLOG_UNPACK_a(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_b)
#define PGLOG_UNPACK_b(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_c)
#define PGLOG_UNPACK_c(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_d)
#define PGLOG_UNPACK_d(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_e)
#define PGLOG_UNPACK_e(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_f)
#define PGLOG_UNPACK_f(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_g)
#define PGLOG_UNPACK_g(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_h)
#define PGLOG_UNPACK_h(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_i)
#define PGLOG_UNPACK_i(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_j)
#define PGLOG_UNPACK_j(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_k)
#define PGLOG_UNPACK_k(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_l)
#define PGLOG_UNPACK_l(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_m)
#define PGLOG_UNPACK_m(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_n)
#define PGLOG_UNPACK_n(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_o)
#define PGLOG_UNPACK_o(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_p)
#define PGLOG_UNPACK_p(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_q)
#define PGLOG_UNPACK_q(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_r)
#define PGLOG_UNPACK_r(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_s)
#define PGLOG_UNPACK_s(...) PGLOG_UNPACK_FIELD(1, __VA_ARGS__, PGLOG_UNPACK_t)

#define PGLOG_CREATE_STATIC_LOGMAKER(globalmaker, localmaker,                 \
            logger, level, file, line, func)                                  \
    localmaker = ::apsara::pangu::StaticLogMakerCreator::Create(&globalmaker, \
            logger, level, file, line, func);

#define PGLOG_CREATE_LOGFORMATER(maker, formatter, helper)                    \
    struct StaticLogFormaterCreator                                           \
    {                                                                         \
        static ::apsara::pangu::StringPieceFormatter* Create(                 \
                ::apsara::pangu::LogMaker* __maker__,                         \
                ::apsara::pangu::TLSLogHelper** __helper__)                   \
            __attribute__((noinline))                                         \
        {                                                                     \
            ::apsara::pangu::StringPieceFormatter* __formatter__ =            \
                    new StringPieceFormatter(1024, ":\t");                    \
            *__helper__ = __maker__->MakeLogHeader(*__formatter__);           \
            if (UNLIKELY(!*__helper__))                                 \
            {                                                                 \
                delete __formatter__;                                         \
                return NULL;                                                  \
            }                                                                 \
            return __formatter__;                                             \
        }                                                                     \
    };                                                                        \
    formatter = StaticLogFormaterCreator::Create(maker, &helper);

#define PGLOG_RECORD(logger, level, fields)                                   \
    do {                                                                      \
        static ::apsara::pangu::LogMaker*                                     \
            __currentInternalLogMaker__ = NULL;                               \
        ::apsara::pangu::LogMaker* __currentInternalLogMakerVar__ =           \
            __currentInternalLogMaker__;                                      \
        if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))           \
        {                                                                     \
            PGLOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,         \
                    __currentInternalLogMakerVar__, logger, level,            \
                        __FILE__, __LINE__, __FUNCTION__);                    \
        }                                                                     \
        __currentInternalLogMakerVar__->AppendLog(                            \
                PGLOG_UNPACK_DEFER(PGLOG_UNPACK_1 fields ()));                \
    } while (false)

#define PGLOG_X_IF(logger, condition, fields, level, likeliness)              \
    do {                                                                      \
        if (condition && likeliness((logger)->GetLevel()<= level))            \
        {                                                                     \
            PGLOG_RECORD(logger, level, fields);                              \
        }                                                                     \
    } while (false)

#define PGLOG_CALL(logger, level, likeliness, fields...)                      \
    static ::apsara::pangu::LogMaker*                                         \
        __currentInternalLogMaker__  = NULL;                                  \
    ::apsara::pangu::LogMaker* __currentInternalLogMakerVar__ =               \
        __currentInternalLogMaker__;                                          \
    if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))               \
    {                                                                         \
        PGLOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,             \
                __currentInternalLogMakerVar__, logger, level,                \
                    __FILE__, __LINE__, __FUNCTION__);                        \
    }                                                                         \
    ::apsara::pangu::LogCallTracer logCallTracer(logger, level,               \
            __currentInternalLogMakerVar__);                                  \
    if (UNLIKELY(logCallTracer.EnableLogging()))                        \
    {                                                                         \
        __currentInternalLogMakerVar__->AppendLog(                            \
                PGLOG_UNPACK_DEFER(                                           \
                    PGLOG_UNPACK_1 ("Enter", __FUNCTION__) fields ()));       \
    }

#define PGLOG_X_BEGIN(logger, level, likeliness, fields...)                   \
    do {                                                                      \
        static ::apsara::pangu::LogMaker*                                     \
            __currentInternalLogMaker__ = NULL;                               \
        ::apsara::pangu::LogMaker* __currentInternalLogMakerVar__ =           \
            __currentInternalLogMaker__;                                      \
        ::apsara::pangu::TLSLogHelper* __currentInternalLogHelper__ = NULL;   \
        if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))           \
        {                                                                     \
            PGLOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,         \
                    __currentInternalLogMakerVar__, logger, level,            \
                        __FILE__, __LINE__, __FUNCTION__);                    \
        }                                                                     \
        StringPieceFormatter* __currentInternalLogFormatter__ = NULL;         \
        if (likeliness((logger)->GetLevel()<= level))                         \
        {                                                                     \
            PGLOG_CREATE_LOGFORMATER(__currentInternalLogMakerVar__,          \
                    __currentInternalLogFormatter__,                          \
                        __currentInternalLogHelper__);                        \
            (*__currentInternalLogFormatter__)fields();                       \
        }

#define PGLOG_FIELD(key, value)                                               \
    do {                                                                      \
        if (__currentInternalLogFormatter__)                                  \
        {                                                                     \
            __currentInternalLogMakerVar__->AppendField(                      \
                *__currentInternalLogFormatter__ , key, value);               \
        }                                                                     \
    } while (false)

#define PGLOG_X_END                                                           \
        if (__currentInternalLogFormatter__ != NULL)                          \
        {                                                                     \
            __currentInternalLogMakerVar__->WriteLogToSink(                   \
                *__currentInternalLogFormatter__,                             \
                    __currentInternalLogHelper__);                            \
            delete __currentInternalLogFormatter__;                           \
        }                                                                     \
    } while (false)

#endif //APSARA_LOGGING_H
