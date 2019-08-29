#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

#include "src/base/env.h"

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;

class MockEnv : public Env
{
public:
    MockEnv() { Env::Reset(this); }
    ~MockEnv() { Env::Reset(NULL); }

    MOCK_METHOD1(GetHostName, Status(std::string* hostname));
};

class MockEnvTest : public testing::Test
{
public:
    MockEnvTest() { }
    ~MockEnvTest() { }

    Status GetHostName(std::string* hostname)
    {
        *hostname = "mock_hostname";
        return OK;
    }

protected:
    MockEnv mMockEnv;
};

TEST(Env, BasicTest)
{
    std::string hostname;
    Status status = Env::Instance()->GetHostName(&hostname);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(MockEnvTest, BasicTest)
{
    EXPECT_CALL(mMockEnv, GetHostName(_))
        .WillRepeatedly(Invoke(this, &MockEnvTest::GetHostName));
    std::string hostname;
    Status status = Env::Instance()->GetHostName(&hostname);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(hostname, "mock_hostname");
}
