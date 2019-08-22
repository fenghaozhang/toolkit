#include <gtest/gtest.h>

#include "src/common/logging.h"

DEFINE_FLAG_INT64(LoggingPerformanceTestNumber, "", 1024);
DEFINE_FLAG_INT64(LoggingPerformanceTestSleepTime, "", 0);
DEFINE_FLAG_BOOL(LoggingPerformanceTestDisableApsaraTest, "", false);

DECLARE_SLOGGER(sLogger, "./common/test/logging_test");

static apsara::logging::Logger* sApsaraLogger =
    apsara::logging::GetLogger("./common/unittest/logging_unittest");

class ScreenLoggerAdaptor : public ILoggerAdaptor
{
public:
    ScreenLoggerAdaptor() {}
    ~ScreenLoggerAdaptor() {}

    /*override*/ void AppendLog(const LoggingHeader& header,
                                RefCountedLoggingData* loggingData)
    {
        write(STDERR_FILENO, loggingData->data(), loggingData->size());
        loggingData->Release();
    }
private:
};

class ScreenLoggingSystem : public ILoggingSystem
{
public:
    ScreenLoggingSystem() : ILoggingSystem("screen")
    {
    }
    ~ScreenLoggingSystem() {}

    /*override*/ ILoggerAdaptor* GetLogger(const std::string& key)
    {
        return &mScreenLoggerAdaptor;
    }

    /*override */ void Setup() {}
    /*override */ bool LoadConfig(const std::string& jsonContent) {return true;}
    /*override */ void FlushLog() {}
    /*override */ void TearDown() {}
private:
    ScreenLoggerAdaptor mScreenLoggerAdaptor;
};

class LoggingTest: public apsara::UnitTestFixtureBase<LoggingTest>
{
public:
    APSARA_UNIT_TEST_CASE(TestLogging, 10240);
    APSARA_UNIT_TEST_CASE(TestLoggingAdaptor, 10240);

public:
    void TestLogging()
    {
        uint64_t chunkVersion = 0x4123456789abcdefULL;
        uint64_t logicalLength = 63 * 1024 * 1024;
        uint32_t diskId = 1;
        uint32_t type = 2;
        bool isWritable = true;
        bool isRAF = false;
        bool chase = false;
        bool isLoading = false;
        uint8_t maxSizeBits = 26;
        uint32_t errorDetectionType = 2;
        uint32_t checksum = 0x12345678;
        uint32_t checksumLength = 4096;
        uint64_t checksumTimestamps = apsara::common::GetCurrentTimeInUs();
        int32_t chunkStatus = 1;
        uint32_t ssdDiskId = INVALID_DISK_ID;
        uint32_t accessCount = 0;
        std::string function = __FUNCTION__;

        uint64_t loopCnt = INT64_FLAG(LoggingPerformanceTestNumber);
        // apsara logging
        if (!BOOL_FLAG(LoggingPerformanceTestDisableApsaraTest))
        {
            uint64_t start = apsara::common::GetCurrentTimeInUs();
            for (size_t idx = 0; idx < loopCnt; ++idx)
            {
                LOG_WARNING(sApsaraLogger, (__FUNCTION__, function)
                        ("ChunkVersion", chunkVersion)
                        ("LogicalLength", logicalLength)
                        ("Type", type)
                        ("DiskId", diskId)
                        ("IsWritable", isWritable)
                        ("IsRAF", isRAF)
                        ("Chase", chase)
                        ("ErrorDetectType", errorDetectionType)
                        ("Checksum", checksum)
                        ("ChecksumLen", checksumLength)
                        ("ChecksumTime", checksumTimestamps)
                        ("ChunkStatus", chunkStatus)
                        ("SSDDiskId", ssdDiskId)
                        ("IsLoading", isLoading)
                        ("AccessCnt", accessCount));
            }
            uint64_t end = apsara::common::GetCurrentTimeInUs();
            fprintf(stderr, ">>> Apsara Logging: Latency: %luns, QPS: %lu\n",
                    (end - start) * 1000UL / loopCnt,
                    loopCnt * 1000000UL / (end - start));
        }

        apsara::logging::FlushLog();

        {
            uint64_t start = apsara::common::GetCurrentTimeInUs();
            for (size_t idx = 0; idx < loopCnt; ++idx)
            {
                PGLOG_WARNING(sLogger, (__FUNCTION__, function)
                        ("ChunkVersion", chunkVersion)
                        ("LogicalLength", logicalLength)
                        ("Type", type)
                        ("DiskId", diskId)
                        ("IsWritable", isWritable)
                        ("IsRAF", isRAF)
                        ("Chase", chase)
                        ("ErrorDetectType", errorDetectionType)
                        ("Checksum", checksum)
                        ("ChecksumLen", checksumLength)
                        ("ChecksumTime", checksumTimestamps)
                        ("ChunkStatus", chunkStatus)
                        ("SSDDiskId", ssdDiskId)
                        ("IsLoading", isLoading)
                        ("AccessCnt", accessCount));
            }
            uint64_t end = apsara::common::GetCurrentTimeInUs();
            fprintf(stderr, ">>> Logging: Latency: %luns, QPS: %lu\n",
                    (end - start) * 1000UL / loopCnt,
                    loopCnt * 1000000UL / (end - start));
        }

        sleep(INT64_FLAG(LoggingPerformanceTestSleepTime));
    }

    void TestLoggingAdaptor()
    {
        ScreenLoggingSystem* loggingSystem = new ScreenLoggingSystem;
        RegisterLoggingSystem(loggingSystem);

        PGLOG_WARNING(sLogger, (__FUNCTION__, 123456)
                ("ToScren", "JustDoIt"));

        DisableLoggingSystem(loggingSystem->GetName());

        PGLOG_WARNING(sLogger, (__FUNCTION__, 123456)
                ("ToScren", "ShouldNotShowOnScreen"));

        EnableLoggingSystem(loggingSystem->GetName());

        PGLOG_WARNING(sLogger, (__FUNCTION__, 123456)
                ("ToScren", "ShouldShowOnScreen"));

        DisableLoggingSystem(loggingSystem->GetName());
    }
};
