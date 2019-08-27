#include <gtest/gtest.h>

#include "src/base/crc32c.h"
#include "src/base/gettime.h"

uint32_t crc32c_combine_slow_for_test(
        uint32_t crc1, uint32_t crc2, size_t len2);
uint32_t crc32c_combine_hw_for_test(
        uint32_t crc1, uint32_t crc2, size_t crc2len);
uint32_t crc32c_combine_sw_for_test(
        uint32_t crc1, uint32_t crc2, size_t crc2len);

void randomFill(char* buf, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = rand() % 0xff;
    }
}

TEST(Crc32C, Basic)
{
    const size_t buf_size = 4096;
    const size_t loop_cnt = 1024 * 1024 * 1;
    char* buf = new char[buf_size];

    randomFill(buf, buf_size);
    {
        uint32_t crc = 0;
        uint64_t start = GetCurrentTimeInUs();
        for (size_t idx = 0; idx < loop_cnt; ++idx)
        {
            crc = docrc32c_intel(crc, buf, buf_size);
        }
        uint64_t end = GetCurrentTimeInUs();
        fprintf(stderr, "Crc: %u, time: %lu\n", crc, end - start);
    }
}

TEST(Crc32C, TestCombine)
{
    const uint32_t outer_rounds = 10;
    const uint32_t inner_rounds = 100;

    for (uint32_t j = 0; j < outer_rounds; j++)
    {
        size_t buf_size = rand() % (4*1024*1024) + 1024*1024;
        char* buf = new char[buf_size];
        randomFill(buf, buf_size);
        uint32_t crcfull;

        crcfull = docrc32c_intel(0, buf, buf_size);
        for (uint32_t i = 0; i < inner_rounds; i++)
        {
            size_t len1, len2;
            uint32_t crc1, crc2, crccombine;

            len1 = rand() % buf_size;
            len2 = buf_size - len1;
            crc1 = docrc32c_intel(0, buf, len1);
            crc2 = docrc32c_intel(0, buf+len1, len2);
            crccombine = crc32c_combine(crc1, crc2, len2);
            uint32_t crccombine_hw =
                crc32c_combine_hw_for_test(crc1, crc2, len2);
            uint32_t crccombine_sw =
                crc32c_combine_sw_for_test(crc1, crc2, len2);
            EXPECT_EQ(crcfull, crccombine);
            EXPECT_EQ(crcfull, crccombine_hw);
            EXPECT_EQ(crcfull, crccombine_sw);
        }
        delete [] buf;
    }
}

// Multiple crc_combines
TEST(Crc32C, TestCombine1)
{
    const uint32_t rounds = 100;

    size_t buf_size = rand() % (4*1024*1024) + 1024*1024;
    char* buf = new char[buf_size];
    randomFill(buf, buf_size);
    uint32_t crcfull;

    crcfull = docrc32c_intel(0, buf, buf_size);
    for (uint32_t i = 0; i < rounds; i++)
    {
        size_t len, offset;
        uint32_t crc, crccombine;

        crccombine = 0;
        offset = 0;
        while (offset < buf_size)
        {
            len = rand() % 8192;
            if (offset + len > buf_size)
                len = buf_size - offset;
            crc = docrc32c_intel(0, buf + offset, len);
            uint32_t crccombine_x = crc32c_combine(crccombine, crc, len);
            uint32_t crccombine_hw =
                crc32c_combine_hw_for_test(crccombine, crc, len);
            uint32_t crccombine_sw =
                crc32c_combine_sw_for_test(crccombine, crc, len);
            EXPECT_EQ(crccombine_x, crccombine_hw);
            EXPECT_EQ(crccombine_x, crccombine_sw);
            crccombine = crccombine_x;
            offset += len;
        }
        EXPECT_EQ(crcfull, crccombine);
    }
    delete [] buf;
}

// Mix crc_combine with crc
TEST(Crc32C, TestCombine2)
{
    const uint32_t rounds = 100;

    size_t buf_size = rand() % (4*1024*1024) + 1024*1024;
    char* buf = new char[buf_size];
    randomFill(buf, buf_size);
    uint32_t crcfull;

    crcfull = docrc32c_intel(0, buf, buf_size);
    for (uint32_t i = 0; i < rounds; i++)
    {
        size_t len, offset;
        uint32_t crc, crccombine;

        crccombine = 0;
        offset = 0;
        while (offset < buf_size)
        {
            len = rand() % 8192;
            if (offset + len > buf_size)
                len = buf_size - offset;
            if (rand() % 2 == 0)
            {
                crc = docrc32c_intel(0, buf + offset, len);
                uint32_t crccombine_x = crc32c_combine(crccombine, crc, len);
                uint32_t crccombine_hw =
                    crc32c_combine_hw_for_test(crccombine, crc, len);
                uint32_t crccombine_sw =
                    crc32c_combine_sw_for_test(crccombine, crc, len);
                EXPECT_EQ(crccombine_x, crccombine_hw);
                EXPECT_EQ(crccombine_x, crccombine_sw);
                crccombine = crccombine_x;
            }
            else
            {
                crccombine = docrc32c_intel(crccombine, buf + offset, len);
            }
            offset += len;
        }
        EXPECT_EQ(crcfull, crccombine);
    }
    delete [] buf;
}

TEST(Crc32C, TestCombinePerformance)
{
    size_t loopCnt = 1024;
    {
        uint32_t crc = 0;
        uint64_t start = GetCurrentTimeInUs();
        for (size_t idx = 0; idx < loopCnt; ++idx)
        {
            crc = crc32c_combine_slow_for_test(
                    crc, idx, (crc % 4096) + 1);
        }
        uint64_t end = GetCurrentTimeInUs();
        fprintf(stderr, "slow crc: %u, latency: %luns, qps: %lu\n",
                crc, (end - start) * 1000UL / loopCnt,
                loopCnt * 1000000UL / (end - start));
    }
    {
        uint32_t crc = 0;
        uint64_t start = GetCurrentTimeInUs();
        for (size_t idx = 0; idx < loopCnt; ++idx)
        {
            crc = crc32c_combine_hw_for_test(
                    crc, idx, (crc % 4096) + 1);
        }
        uint64_t end = GetCurrentTimeInUs();
        fprintf(stderr, "hw crc: %u, latency: %luns, qps: %lu\n",
                crc, (end - start) * 1000UL / loopCnt,
                loopCnt * 1000000UL / (end - start));
    }
    {
        uint32_t crc = 0;
        uint64_t start = GetCurrentTimeInUs();
        for (size_t idx = 0; idx < loopCnt; ++idx)
        {
            crc = crc32c_combine_sw_for_test(
                    crc, idx, (crc % 4096) + 1);
        }
        uint64_t end = GetCurrentTimeInUs();
        fprintf(stderr, "sw crc: %u, latency: %luns, qps: %lu\n",
                crc, (end - start) * 1000UL / loopCnt,
                loopCnt * 1000000UL / (end - start));
    }
}

TEST(Crc32C, TestCombine4KB)
{
    const uint32_t rounds = 1000;
    char* buf = new char[4*1024*1024 + 4*1024];

    randomFill(buf, 4*1024*1024 + 4*1024);
    for (uint32_t i = 0; i < rounds; i++)
    {
        size_t buf_size = rand() % (4*1024*1024) + 4*1024;
        uint32_t crcfull;

        crcfull = docrc32c_intel(0, buf, buf_size);
        size_t len1, len2;
        uint32_t crc1, crc2, crccombine;

        len1 = buf_size - 4*1024;
        len2 = 4*1024;
        crc1 = docrc32c_intel(0, buf, len1);
        crc2 = docrc32c_intel(0, buf+len1, len2);
        crccombine = crc32c_combine_4KB(crc1, crc2);
        EXPECT_EQ(crcfull, crccombine);
    }
    delete [] buf;
}
