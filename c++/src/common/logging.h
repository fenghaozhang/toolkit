#ifndef _SRC_COMMON_LOGGING_H
#define _SRC_COMMON_LOGGING_H

#include <stdint.h>
#include <string>
#include <tr1/type_traits>

#include "src/common/macro.h"

#define DECLARE_SLOGGER(logger, key)    \
    static Logger* logger = Logger::GetLogger(key)

#define GET_SLOGGER(key) Logger::GetLogger(key)

// To disable logging by loglevel, set DISABLE_LOGGING_XXX to 1
#ifdef __DEBUG__  // debug mode
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

namespace logging
{
struct Logger;
struct LogRecord;
}

enum LogLevel
{
    LOG_LEVEL_ALL     = 0,      // Enable all logs
    LOG_LEVEL_PROFILE = 100,
    LOG_LEVEL_DEBUG   = 200,
    LOG_LEVEL_INFO    = 300,
    LOG_LEVEL_WARNING = 400,
    LOG_LEVEL_ERROR   = 500,
    LOG_LEVEL_FATAL   = 600,
    LOG_LEVEL_NONE    = 10000,  // Disable all logs
    LOG_LEVEL_DEFAULT = LOG_LEVEL_INFO,
    LOG_LEVEL_INHERIT = -1      // Inherit from the parent
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
    LOG_LEVEL_INDEX_NONE    = 7,  // Disable all logs, must be last item
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
void RotateLogFiles(const std::string& path,
                    bool bCompress,
                    int maxFileNum,
                    int maxDay,
                    std::string* tmpFile = NULL);

bool LoadConfig(const std::string& jsonContent);
bool LoadConfigFile(const std::string&  filePath="");
void ReloadLogLevel();

/*
 * @brief, format 2018-03-22 10:10:10.123456
 */
std::string GetCurrentTimeInString();

/*
 *@brief: register a new logging system, all later logging data
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

ILoggingSystem* CreateApsaraLoggingSystem();

#ifdef SHOW_EASYUTHREAD_IN_LOGGING
#define UTHREAD_ADDR_FIELD ("uthread", easy_uthread_current())
#else
#define UTHREAD_ADDR_FIELD
#endif

#define LOG_X_DUMMY_BEGIN                                                   \
    do {                                                                      \
        LogMaker* __currentInternalLogMakerVar__ = NULL;     \
        StringPieceFormatter* __currentInternalLogFormatter__ = NULL;

#define LOG_X_DUMMY_END                                                     \
    } while (0)

// LOG_DEBUG
#if defined(DISABLE_LOGGING_DEBUG) && DISABLE_LOGGING_DEBUG == 1
#define LOG_DEBUG(...)        /* nothing to do */
#define LOG_DEBUG_IF(...)     /* nothing to do */
#define LOG_CALL_DEBUG(...)   /* nothing to do */
#define LOG_DEBUG_BEGIN(...)  LOG_X_DUMMY_BEGIN
#define LOG_DEBUG_END         LOG_X_DUMMY_END
#else
#define LOG_DEBUG(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_DEBUG, UNLIKELY)
#define LOG_DEBUG_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_DEBUG, UNLIKELY)
#define LOG_CALL_DEBUG(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_DEBUG, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#define LOG_DEBUG_BEGIN(logger, ...) \
    LOG_X_BEGIN(logger, LOG_LEVEL_DEBUG, UNLIKELY, ##__VA_ARGS__)
#define LOG_DEBUG_END \
    LOG_X_END
#endif

// LOG_INFO
#if defined(DISABLE_LOGGING_INFO) && DISABLE_LOGGING_INFO == 1
#define LOG_INFO(...)       /* nothing to do */
#define LOG_INFO_IF(...)    /* nothing to do */
#define LOG_CALL_INFO(...)  /* nothing to do */
#define LOG_INFO_BEGIN(...) LOG_X_DUMMY_BEGIN
#define LOG_INFO_END        LOG_X_DUMMY_END
#else
#define LOG_INFO(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_INFO, UNLIKELY)
#define LOG_INFO_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_INFO, UNLIKELY)
#define LOG_CALL_INFO(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_INFO, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#define LOG_INFO_BEGIN(logger, ...) \
    LOG_X_BEGIN(logger, LOG_LEVEL_INFO, UNLIKELY, ##__VA_ARGS__)
#define LOG_INFO_END \
    LOG_X_END
#endif

// LOG_WARNING
#if defined(DISABLE_LOGGING_WARNING) && DISABLE_LOGGING_WARNING == 1
#define LOG_WARNING(...)       /* nothing to do */
#define LOG_WARNING_IF(...)    /* nothing to do */
#define LOG_CALL_WARNING(...)  /* nothing to do */
#define LOG_WARNING_BEGIN(...) LOG_X_DUMMY_BEGIN
#define LOG_WARNING_END        LOG_X_DUMMY_END
#else
#define LOG_WARNING(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_WARNING, LIKELY)
#define LOG_WARNING_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_WARNING, LIKELY)
#define LOG_CALL_WARNING(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_WARNING, LIKELY, UTHREAD_ADDR_FIELD fields)
#define LOG_WARNING_BEGIN(logger, ...) \
    LOG_X_BEGIN(logger, LOG_LEVEL_WARNING, LIKELY, ##__VA_ARGS__)
#define LOG_WARNING_END \
    LOG_X_END
#endif

// LOG_ERROR
#if defined(DISABLE_LOGGING_ERROR) && DISABLE_LOGGING_ERROR == 1
#define LOG_ERROR(...)       /* nothing to do */
#define LOG_ERROR_IF(...)    /* nothing to do */
#define LOG_CALL_ERROR(...)  /* nothing to do */
#define LOG_ERROR_BEGIN(...) LOG_X_DUMMY_BEGIN
#define LOG_ERROR_END        LOG_X_DUMMY_END
#else
#define LOG_ERROR(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_ERROR, LIKELY)
#define LOG_ERROR_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_ERROR, LIKELY)
#define LOG_CALL_ERROR(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_ERROR, LIKELY, UTHREAD_ADDR_FIELD fields)
#define LOG_ERROR_BEGIN(logger, ...) \
    LOG_X_BEGIN(logger, LOG_LEVEL_ERROR, LIKELY, ##__VA_ARGS__)
#define LOG_ERROR_END \
    LOG_X_END
#endif

// LOG_FATAL
#if defined(DISABLE_LOGGING_FATAL) && DISABLE_LOGGING_FATAL == 1
#define LOG_FATAL(...)       /* nothing to do */
#define LOG_FATAL_IF(...)    /* nothing to do */
#define LOG_CALL_FATAL(...)  /* nothing to do */
#define LOG_FATAL_BEGIN(...) LOG_X_DUMMY_BEGIN
#define LOG_FATAL_END        LOG_X_DUMMY_END
#else
#define LOG_FATAL(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_FATAL, LIKELY)
#define LOG_FATAL_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_FATAL, LIKELY)
#define LOG_CALL_FATAL(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_FATAL, LIKELY, UTHREAD_ADDR_FIELD fields)
#define LOG_FATAL_BEGIN(logger, ...) \
    LOG_X_BEGIN(logger, LOG_LEVEL_FATAL, LIKELY, ##__VA_ARGS__)
#define LOG_FATAL_END \
    LOG_X_END
#endif

// LOG_PROFILE
#if defined(DISABLE_LOGGING_PROFILE) && DISABLE_LOGGING_PROFILE == 1
#define LOG_PROFILE(...)      /* nothing to do */
#define LOG_PROFILE_IF(...)   /* nothing to do */
#define LOG_CALL_PROFILE(...) /* nothing to do */
#else
#define LOG_PROFILE(logger, fields)   \
    LOG_X_IF(logger, true, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_PROFILE, UNLIKELY)
#define LOG_PROFILE_IF(logger, condition, fields) \
    LOG_X_IF(logger, condition, UTHREAD_ADDR_FIELD fields, LOG_LEVEL_PROFILE, UNLIKELY)
#define LOG_CALL_PROFILE(logger, fields...) \
    LOG_CALL(logger, LOG_LEVEL_PROFILE, UNLIKELY, UTHREAD_ADDR_FIELD fields)
#endif

#define LOGF_PROFILE(...) \
        LOG_PROFILE(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_DEBUG(...) \
        LOG_DEBUG(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_INFO(...) \
        LOG_INFO(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_WARNING(...) \
        LOG_WARNING(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_ERROR(...) \
        LOG_ERROR(sLogger, ("", StringPrintf(__VA_ARGS__)))
#define LOGF_FATAL(...) \
        LOG_FATAL(sLogger, ("", StringPrintf(__VA_ARGS__)))

#ifndef TRANSLATE_LINE_TO_STR
#define TRANSLATE_LINEINT_TO_STR(count)  #count
#define TRANSLATE_LINE_TO_STR(count) TRANSLATE_LINEINT_TO_STR(count)
#endif

#define LOG_QPS_X(logger, throttle, LOG_METHOD, fields)                           \
    do                                                                              \
    {                                                                               \
        uint64_t waitTime;                                                          \
        if (throttle.TryDispatch(1, easy_io_time(), &waitTime))                     \
        {                                                                           \
            LOG_METHOD(logger, fields);                                             \
        }                                                                           \
    } while (false)

#define LOG_QPS(logger, qps, LOG_METHOD, fields)                                  \
    do                                                                              \
    {                                                                               \
        static GeneralThrottle sLoggingThrottle(qps, 1000000,      \
                __FILE__ ":" TRANSLATE_LINE_TO_STR(__LINE__));                      \
        LOG_QPS_X(logger, sLoggingThrottle, LOG_METHOD, fields);                  \
    } while (false)

#define LOG_INFO_QPS50(logger,   fields)    LOG_QPS(logger, 50,   LOG_INFO, fields)
#define LOG_INFO_QPS100(logger,  fields)    LOG_QPS(logger, 100,  LOG_INFO, fields)
#define LOG_INFO_QPS1000(logger, fields)    LOG_QPS(logger, 1000, LOG_INFO, fields)

#define LOG_WARNING_QPS50(logger,   fields) LOG_QPS(logger, 50,   LOG_WARNING, fields)
#define LOG_WARNING_QPS100(logger,  fields) LOG_QPS(logger, 100,  LOG_WARNING, fields)
#define LOG_WARNING_QPS1000(logger, fields) LOG_QPS(logger, 1000, LOG_WARNING, fields)

#define LOG_ERROR_QPS50(logger,   fields)   LOG_QPS(logger, 50,   LOG_ERROR, fields)
#define LOG_ERROR_QPS100(logger,  fields)   LOG_QPS(logger, 100,  LOG_ERROR, fields)
#define LOG_ERROR_QPS1000(logger, fields)   LOG_QPS(logger, 1000, LOG_ERROR, fields)

#define LOG_INFO_QPS(logger, throttle, fields)    LOG_QPS_X(logger, throttle, LOG_INFO, fields)
#define LOG_WARNING_QPS(logger, throttle, fields) LOG_QPS_X(logger, throttle, LOG_WARNING, fields)
#define LOG_ERROR_QPS(logger, throttle, fields)   LOG_QPS_X(logger, throttle, LOG_ERROR, fields)

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

#define LOG_REPEAT_CODE_1(macro, ...)                                       \
    macro(1, __VA_ARGS__)

#define LOG_REPEAT_CODE_2(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__)

#define LOG_REPEAT_CODE_3(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)

#define LOG_REPEAT_CODE_4(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__)

#define LOG_REPEAT_CODE_5(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__)

#define LOG_REPEAT_CODE_6(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)

#define LOG_REPEAT_CODE_7(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__)

#define LOG_REPEAT_CODE_8(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__)

#define LOG_REPEAT_CODE_9(macro, ...)                                       \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__) macro(9, __VA_ARGS__)

#define LOG_REPEAT_CODE_10(macro, ...)                                      \
    macro(1, __VA_ARGS__) macro(2, __VA_ARGS__) macro(3, __VA_ARGS__)         \
    macro(4, __VA_ARGS__) macro(5, __VA_ARGS__) macro(6, __VA_ARGS__)         \
    macro(7, __VA_ARGS__) macro(8, __VA_ARGS__) macro(9, __VA_ARGS__)         \
    macro(10, __VA_ARGS__)

#define LOG_REPEAT_CODE_11(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__)

#define LOG_REPEAT_CODE_12(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \

#define LOG_REPEAT_CODE_13(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__)

#define LOG_REPEAT_CODE_14(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__)

#define LOG_REPEAT_CODE_15(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)

#define LOG_REPEAT_CODE_16(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__)

#define LOG_REPEAT_CODE_17(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__)

#define LOG_REPEAT_CODE_18(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)

#define LOG_REPEAT_CODE_19(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__)

#define LOG_REPEAT_CODE_20(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__)

#define LOG_REPEAT_CODE_21(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)

#define LOG_REPEAT_CODE_22(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__)

#define LOG_REPEAT_CODE_23(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__)

#define LOG_REPEAT_CODE_24(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__) macro(24, __VA_ARGS__)

#define LOG_REPEAT_CODE_25(macro, ...)                                      \
    macro(1, __VA_ARGS__)  macro(2, __VA_ARGS__)  macro(3, __VA_ARGS__)       \
    macro(4, __VA_ARGS__)  macro(5, __VA_ARGS__)  macro(6, __VA_ARGS__)       \
    macro(7, __VA_ARGS__)  macro(8, __VA_ARGS__)  macro(9, __VA_ARGS__)       \
    macro(10, __VA_ARGS__) macro(11, __VA_ARGS__) macro(12, __VA_ARGS__)      \
    macro(13, __VA_ARGS__) macro(14, __VA_ARGS__) macro(15, __VA_ARGS__)      \
    macro(16, __VA_ARGS__) macro(17, __VA_ARGS__) macro(18, __VA_ARGS__)      \
    macro(19, __VA_ARGS__) macro(20, __VA_ARGS__) macro(21, __VA_ARGS__)      \
    macro(22, __VA_ARGS__) macro(23, __VA_ARGS__) macro(24, __VA_ARGS__)      \
    macro(25, __VA_ARGS__)

#define LOG_CAT(a, ...)        a ## __VA_ARGS__
#define LOG_EVAL(...)          __VA_ARGS__
#define LOG_IIF(cond)          LOG_IIF_ ## cond()
#define LOG_IIF_1(...)         1, 0
#define LOG_COMMA_1            ,
#define LOG_COMMA_0
#define LOG_SECOND(a, b, ...)  LOG_COMMA_ ## b
#define LOG_CHECK_PARAM(...)   LOG_SECOND(__VA_ARGS__, 1)
#define LOG_ADD_COMMA(a)       LOG_CHECK_PARAM(LOG_IIF(a))

#define LOG_TYPENAME_DECLARE(index, ...)                                    \
    LOG_ADD_COMMA(index) typename P ## index, typename V ## index

#define LOG_CTOR_TYPEVAR_DECLARE(index, ...)                                \
    LOG_ADD_COMMA(index) const P ## index& p ## index ## _,                 \
        const V ## index& v ## index ## _

#define LOG_PARAM_TYPEVAR_DECLARE(index, ...)                               \
    LOG_ADD_COMMA(index) const P ## index& p ## index,                      \
        const V ## index& v ## index

#define LOG_VARTYPE_DECLARE(index, ...)                                     \
    LOG_ADD_COMMA(index) P ## index, V ## index

#define LOG_APPEND_PARAM_INIT(index, ...)                                   \
    p ## index = p ## index ## _; v ## index = v ## index ## _;

#define LOG_APPEND_PARAM_DECLARE(index, ...)                                \
    P ## index p ## index; V ## index v ## index;

#define LOG_APPEND_PARAM_CTOR_LIST(index, ...)                              \
    LOG_ADD_COMMA(index)                                                    \
        LoggingFieldTypeEncoder<P ## index>::Encode(p ## index),              \
            LoggingFieldTypeEncoder<V ## index>::Encode(v ## index)

#define LOG_APPEND_PARAM_VARTYPE_DECLARE(index, ...)                        \
    LOG_ADD_COMMA(index)                                                    \
        typename LoggingFieldTypeEncoder<P ## index>::FieldType,              \
            typename LoggingFieldTypeEncoder<V ## index>::FieldType

#define LOG_APPEND_PARAM_DUMP(index, ...)                                   \
    formatter(LoggingFieldTypeDecoder<P ## index>::Decode(param.p ## index),  \
        LoggingFieldTypeDecoder<V ## index>::Decode(param.v ## index));

#define LOG_DEFINE_APPEND_PARAM(i)                                          \
    template<LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                      \
            (LOG_TYPENAME_DECLARE))>                                        \
    struct LoggingAppendParam##i                                              \
    {                                                                         \
        LoggingAppendParam##i(LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)     \
            (LOG_CTOR_TYPEVAR_DECLARE)))                                    \
        __attribute__((always_inline))                                        \
        {                                                                     \
            LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                       \
                (LOG_APPEND_PARAM_INIT));                                   \
        }                                                                     \
        LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                           \
                (LOG_APPEND_PARAM_DECLARE));                                \
    };

LOG_DEFINE_APPEND_PARAM(1);   LOG_DEFINE_APPEND_PARAM(2);
LOG_DEFINE_APPEND_PARAM(3);   LOG_DEFINE_APPEND_PARAM(4);
LOG_DEFINE_APPEND_PARAM(5);   LOG_DEFINE_APPEND_PARAM(6);
LOG_DEFINE_APPEND_PARAM(7);   LOG_DEFINE_APPEND_PARAM(8);
LOG_DEFINE_APPEND_PARAM(9);   LOG_DEFINE_APPEND_PARAM(10);
LOG_DEFINE_APPEND_PARAM(11);  LOG_DEFINE_APPEND_PARAM(12);
LOG_DEFINE_APPEND_PARAM(13);  LOG_DEFINE_APPEND_PARAM(14);
LOG_DEFINE_APPEND_PARAM(15);  LOG_DEFINE_APPEND_PARAM(16);
LOG_DEFINE_APPEND_PARAM(17);  LOG_DEFINE_APPEND_PARAM(18);
LOG_DEFINE_APPEND_PARAM(19);  LOG_DEFINE_APPEND_PARAM(20);
LOG_DEFINE_APPEND_PARAM(21);  LOG_DEFINE_APPEND_PARAM(22);
LOG_DEFINE_APPEND_PARAM(23);

#define LOG_NUMBER_SUB_2(i) LOG_CAT(LOG_NUMBER_SUB_2_, i)
#define LOG_NUMBER_SUB_2_3   1
#define LOG_NUMBER_SUB_2_4   2
#define LOG_NUMBER_SUB_2_5   3
#define LOG_NUMBER_SUB_2_6   4
#define LOG_NUMBER_SUB_2_7   5
#define LOG_NUMBER_SUB_2_8   6
#define LOG_NUMBER_SUB_2_9   7
#define LOG_NUMBER_SUB_2_10  8
#define LOG_NUMBER_SUB_2_11  9
#define LOG_NUMBER_SUB_2_12  10
#define LOG_NUMBER_SUB_2_13  11
#define LOG_NUMBER_SUB_2_14  12
#define LOG_NUMBER_SUB_2_15  13
#define LOG_NUMBER_SUB_2_16  14
#define LOG_NUMBER_SUB_2_17  15
#define LOG_NUMBER_SUB_2_18  16
#define LOG_NUMBER_SUB_2_19  17
#define LOG_NUMBER_SUB_2_20  18
#define LOG_NUMBER_SUB_2_21  19
#define LOG_NUMBER_SUB_2_22  20
#define LOG_NUMBER_SUB_2_23  21
#define LOG_NUMBER_SUB_2_24  22
#define LOG_NUMBER_SUB_2_25  23

#define LOG_CREATE_APPEND_LOG_TMPL_PARAM_X(i)                               \
        LoggingAppendParam##i<LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)     \
            (LOG_APPEND_PARAM_VARTYPE_DECLARE))>                            \
                param(LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)             \
                    (LOG_APPEND_PARAM_CTOR_LIST)));
#define LOG_CREATE_APPEND_LOG_TMPL_PARAM(...)                               \
    LOG_CREATE_APPEND_LOG_TMPL_PARAM_X(__VA_ARGS__)

#define LOG_CREATE_APPEND_LOG_API(i, ix)                                    \
    template<LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                      \
            (LOG_TYPENAME_DECLARE))>                                        \
        __attribute__((always_inline))                                        \
    void AppendLog(LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                \
            (LOG_PARAM_TYPEVAR_DECLARE)))                                   \
    {                                                                         \
        LOG_CREATE_APPEND_LOG_TMPL_PARAM(LOG_NUMBER_SUB_2(i));            \
        appendLog(param, LoggingFieldTypeEncoder<P ## ix>::Encode(p ## ix),   \
                LoggingFieldTypeEncoder<V ## ix>::Encode(v ## ix),            \
                LoggingFieldTypeEncoder<P ## i>::Encode(p ## i),              \
                LoggingFieldTypeEncoder<V ## i>::Encode(v ## i));             \
    }

#define LOG_CREATE_APPEND_LOG_IMPL_DUMP_PARAM(i)                            \
    LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)(LOG_APPEND_PARAM_DUMP))

#define LOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE_X(i)                          \
    const LoggingAppendParam##i<LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)   \
        (LOG_VARTYPE_DECLARE))>& param

#define LOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE(...)                          \
    LOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE_X(__VA_ARGS__)

#define LOG_CREATE_APPEND_LOG_IMPL(i, ix)                                   \
    template<LOG_EVAL(LOG_CAT(LOG_REPEAT_CODE_, i)                      \
            (LOG_TYPENAME_DECLARE))>                                        \
        __attribute__((noinline))                                             \
    void appendLog(                                                           \
            LOG_CREATE_APPEND_LOG_TMPL_PARAM_TYPE(LOG_NUMBER_SUB_2(i)),   \
                const P ## ix p ## ix, const V ## ix v ## ix,                 \
                const P ## i p ## i, const V ## i v ## i)                     \
    {                                                                         \
        StringPieceFormatter formatter(1024, ":\t");                          \
        TLSLogHelper* helper = MakeLogHeader(formatter);                      \
        if (LIKELY(helper))                                             \
        {                                                                     \
            LOG_CREATE_APPEND_LOG_IMPL_DUMP_PARAM(LOG_NUMBER_SUB_2(i));   \
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

    LOG_CREATE_APPEND_LOG_API(3,  2);  LOG_CREATE_APPEND_LOG_API(4,  3);
    LOG_CREATE_APPEND_LOG_API(5,  4);  LOG_CREATE_APPEND_LOG_API(6,  5);
    LOG_CREATE_APPEND_LOG_API(7,  6);  LOG_CREATE_APPEND_LOG_API(8,  7);
    LOG_CREATE_APPEND_LOG_API(9,  8);  LOG_CREATE_APPEND_LOG_API(10, 9);
    LOG_CREATE_APPEND_LOG_API(11, 10); LOG_CREATE_APPEND_LOG_API(12, 11);
    LOG_CREATE_APPEND_LOG_API(13, 12); LOG_CREATE_APPEND_LOG_API(14, 13);
    LOG_CREATE_APPEND_LOG_API(15, 14); LOG_CREATE_APPEND_LOG_API(16, 15);
    LOG_CREATE_APPEND_LOG_API(17, 16); LOG_CREATE_APPEND_LOG_API(18, 17);
    LOG_CREATE_APPEND_LOG_API(19, 18); LOG_CREATE_APPEND_LOG_API(20, 19);
    LOG_CREATE_APPEND_LOG_API(21, 20); LOG_CREATE_APPEND_LOG_API(22, 21);
    LOG_CREATE_APPEND_LOG_API(23, 22); LOG_CREATE_APPEND_LOG_API(24, 23);
    LOG_CREATE_APPEND_LOG_API(25, 24);

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

    LOG_CREATE_APPEND_LOG_IMPL(3,  2);  LOG_CREATE_APPEND_LOG_IMPL(4,  3);
    LOG_CREATE_APPEND_LOG_IMPL(5,  4);  LOG_CREATE_APPEND_LOG_IMPL(6,  5);
    LOG_CREATE_APPEND_LOG_IMPL(7,  6);  LOG_CREATE_APPEND_LOG_IMPL(8,  7);
    LOG_CREATE_APPEND_LOG_IMPL(9,  8);  LOG_CREATE_APPEND_LOG_IMPL(10, 9);
    LOG_CREATE_APPEND_LOG_IMPL(11, 10); LOG_CREATE_APPEND_LOG_IMPL(12, 11);
    LOG_CREATE_APPEND_LOG_IMPL(13, 12); LOG_CREATE_APPEND_LOG_IMPL(14, 13);
    LOG_CREATE_APPEND_LOG_IMPL(15, 14); LOG_CREATE_APPEND_LOG_IMPL(16, 15);
    LOG_CREATE_APPEND_LOG_IMPL(17, 16); LOG_CREATE_APPEND_LOG_IMPL(18, 17);
    LOG_CREATE_APPEND_LOG_IMPL(19, 18); LOG_CREATE_APPEND_LOG_IMPL(20, 19);
    LOG_CREATE_APPEND_LOG_IMPL(21, 20); LOG_CREATE_APPEND_LOG_IMPL(22, 21);
    LOG_CREATE_APPEND_LOG_IMPL(23, 22); LOG_CREATE_APPEND_LOG_IMPL(24, 22);
    LOG_CREATE_APPEND_LOG_IMPL(25, 24);

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
    static LogMaker* Create(LogMaker** makerptr,
            Logger* logger, LogLevel level, const char* file,
            int line, const char* func) __attribute__((noinline))
    {
        if (*makerptr != NULL)
        {
            return *makerptr;
        }
        LogMaker* t = new LogMaker(logger, file, line, func, level);
        if (!__sync_bool_compare_and_swap(makerptr, (LogMaker*)NULL, t))
        {
            delete t;
        }
        return *makerptr;
    }
};

#define LOG_UNPACK_DEFER(...) __VA_ARGS__

#define LOG_GET_NEXT_OPERATION(x, op, ...)    op
#define LOG_GET_THIS_OPERATION(x, y, op, ...) op

#define LOG_OPERATION_COMMA_0
#define LOG_OPERATION_COMMA_1 ,

#define LOG_OPERATION_DUMP(cond, a, b)  LOG_OPERATION_COMMA_ ## cond a, b
#define LOG_OPERATION_DUMP_DONE(...)
#define LOG_OPERATION_NEXT_DONE

#define LOG_UNPACK_FIELD(cond, a, b, ...)                                 \
    LOG_GET_THIS_OPERATION(~, ##__VA_ARGS__, LOG_OPERATION_DUMP,        \
            LOG_OPERATION_DUMP_DONE)(cond, a, b)                          \
    LOG_GET_NEXT_OPERATION(~, ##__VA_ARGS__, LOG_OPERATION_NEXT_DONE)

#define LOG_UNPACK_1(...) LOG_UNPACK_FIELD(0, __VA_ARGS__, LOG_UNPACK_2)
#define LOG_UNPACK_2(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_3)
#define LOG_UNPACK_3(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_4)
#define LOG_UNPACK_4(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_5)
#define LOG_UNPACK_5(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_6)
#define LOG_UNPACK_6(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_7)
#define LOG_UNPACK_7(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_8)
#define LOG_UNPACK_8(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_9)
#define LOG_UNPACK_9(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_a)
#define LOG_UNPACK_a(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_b)
#define LOG_UNPACK_b(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_c)
#define LOG_UNPACK_c(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_d)
#define LOG_UNPACK_d(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_e)
#define LOG_UNPACK_e(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_f)
#define LOG_UNPACK_f(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_g)
#define LOG_UNPACK_g(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_h)
#define LOG_UNPACK_h(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_i)
#define LOG_UNPACK_i(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_j)
#define LOG_UNPACK_j(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_k)
#define LOG_UNPACK_k(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_l)
#define LOG_UNPACK_l(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_m)
#define LOG_UNPACK_m(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_n)
#define LOG_UNPACK_n(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_o)
#define LOG_UNPACK_o(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_p)
#define LOG_UNPACK_p(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_q)
#define LOG_UNPACK_q(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_r)
#define LOG_UNPACK_r(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_s)
#define LOG_UNPACK_s(...) LOG_UNPACK_FIELD(1, __VA_ARGS__, LOG_UNPACK_t)

#define LOG_CREATE_STATIC_LOGMAKER(globalmaker, localmaker,                 \
            logger, level, file, line, func)                                  \
    localmaker = StaticLogMakerCreator::Create(&globalmaker, \
            logger, level, file, line, func);

#define LOG_CREATE_LOGFORMATER(maker, formatter, helper)                    \
    struct StaticLogFormaterCreator                                           \
    {                                                                         \
        static StringPieceFormatter* Create(                 \
                LogMaker* __maker__,                         \
                TLSLogHelper** __helper__)                   \
            __attribute__((noinline))                                         \
        {                                                                     \
            StringPieceFormatter* __formatter__ =            \
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

#define LOG_RECORD(logger, level, fields)                                     \
    do {                                                                      \
        static LogMaker*                                     \
            __currentInternalLogMaker__ = NULL;                               \
        LogMaker* __currentInternalLogMakerVar__ =           \
            __currentInternalLogMaker__;                                      \
        if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))           \
        {                                                                     \
            LOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,         \
                    __currentInternalLogMakerVar__, logger, level,            \
                        __FILE__, __LINE__, __FUNCTION__);                    \
        }                                                                     \
        __currentInternalLogMakerVar__->AppendLog(                            \
                LOG_UNPACK_DEFER(LOG_UNPACK_1 fields ()));                \
    } while (false)

#define LOG_X_IF(logger, condition, fields, level, likeliness)              \
    do {                                                                      \
        if (condition && likeliness((logger)->GetLevel()<= level))            \
        {                                                                     \
            LOG_RECORD(logger, level, fields);                              \
        }                                                                     \
    } while (false)

#define LOG_CALL(logger, level, likeliness, fields...)                      \
    static LogMaker*                                         \
        __currentInternalLogMaker__  = NULL;                                  \
    LogMaker* __currentInternalLogMakerVar__ =               \
        __currentInternalLogMaker__;                                          \
    if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))               \
    {                                                                         \
        LOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,             \
                __currentInternalLogMakerVar__, logger, level,                \
                    __FILE__, __LINE__, __FUNCTION__);                        \
    }                                                                         \
    LogCallTracer logCallTracer(logger, level,               \
            __currentInternalLogMakerVar__);                                  \
    if (UNLIKELY(logCallTracer.EnableLogging()))                        \
    {                                                                         \
        __currentInternalLogMakerVar__->AppendLog(                            \
                LOG_UNPACK_DEFER(                                           \
                    LOG_UNPACK_1 ("Enter", __FUNCTION__) fields ()));       \
    }

#define LOG_X_BEGIN(logger, level, likeliness, fields...)                   \
    do {                                                                      \
        static LogMaker*                                     \
            __currentInternalLogMaker__ = NULL;                               \
        LogMaker* __currentInternalLogMakerVar__ =           \
            __currentInternalLogMaker__;                                      \
        TLSLogHelper* __currentInternalLogHelper__ = NULL;   \
        if (UNLIKELY(__currentInternalLogMakerVar__ == NULL))           \
        {                                                                     \
            LOG_CREATE_STATIC_LOGMAKER(__currentInternalLogMaker__,         \
                    __currentInternalLogMakerVar__, logger, level,            \
                        __FILE__, __LINE__, __FUNCTION__);                    \
        }                                                                     \
        StringPieceFormatter* __currentInternalLogFormatter__ = NULL;         \
        if (likeliness((logger)->GetLevel()<= level))                         \
        {                                                                     \
            LOG_CREATE_LOGFORMATER(__currentInternalLogMakerVar__,          \
                    __currentInternalLogFormatter__,                          \
                        __currentInternalLogHelper__);                        \
            (*__currentInternalLogFormatter__)fields();                       \
        }

#define LOG_FIELD(key, value)                                               \
    do {                                                                      \
        if (__currentInternalLogFormatter__)                                  \
        {                                                                     \
            __currentInternalLogMakerVar__->AppendField(                      \
                *__currentInternalLogFormatter__ , key, value);               \
        }                                                                     \
    } while (false)

#define LOG_X_END                                                           \
        if (__currentInternalLogFormatter__ != NULL)                          \
        {                                                                     \
            __currentInternalLogMakerVar__->WriteLogToSink(                   \
                *__currentInternalLogFormatter__,                             \
                    __currentInternalLogHelper__);                            \
            delete __currentInternalLogFormatter__;                           \
        }                                                                     \
    } while (false)

#endif  // _SRC_COMMON_LOGGING_H
