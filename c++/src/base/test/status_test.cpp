#include <gtest/gtest.h>

#include "src/base/status.h"

#define EXPECT_OK(error) EXPECT_EQ(error, OK)
#define EXPECT_ERROR(expError, actualError) EXPECT_EQ(expError, actualError)

TEST(StatusTest, StatusOKTest)
{
    Status s1 = OK;
    EXPECT_TRUE(s1.IsOk());
    EXPECT_STREQ(s1.GetMessage(), "");
    EXPECT_EQ(s1.ToString(), "OK");
}

TEST(StatusTest, LongMessageTest)
{
    std::string detail =
        "This operation is not allowed for your accout, "
        "contact administrator please.";
    std::string errMsg = "INTERNAL_ERROR (-1): ";
    errMsg += detail;

    Status s1(INTERNAL_ERROR, detail);
    EXPECT_TRUE(!s1.IsOk());
    EXPECT_EQ(s1.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s1.GetMessage(), detail.c_str());
    EXPECT_EQ(s1.ToString(), errMsg);

    Status s2(s1);
    EXPECT_TRUE(!s2.IsOk());
    EXPECT_EQ(s2.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s2.GetMessage(), detail.c_str());
    EXPECT_EQ(s2.ToString(), errMsg);

    Status s3 = s1;
    EXPECT_TRUE(!s3.IsOk());
    EXPECT_EQ(s3.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s3.GetMessage(), detail.c_str());
    EXPECT_EQ(s3.ToString(), errMsg);
}

TEST(StatusTest, ShortMessageTest)
{
    std::string detail =
        "This operation is not allowed for your accout.";
    std::string errMsg = "INTERNAL_ERROR (-1): ";
    errMsg += detail;

    Status s1(INTERNAL_ERROR, detail);
    EXPECT_TRUE(!s1.IsOk());
    EXPECT_EQ(s1.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s1.GetMessage(), detail.c_str());
    EXPECT_EQ(s1.ToString(), errMsg);

    Status s2(s1);
    EXPECT_TRUE(!s2.IsOk());
    EXPECT_EQ(s2.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s1.GetMessage(), detail.c_str());
    EXPECT_EQ(s2.ToString(), errMsg);

    Status s3 = s1;
    EXPECT_TRUE(!s3.IsOk());
    EXPECT_EQ(s3.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s3.GetMessage(), detail.c_str());
    EXPECT_EQ(s3.ToString(), errMsg);
}

TEST(StatusTest, HybridMessageTest)
{
    std::string detail =
        "This operation is not allowed for your accout.";
    std::string errMsg = "INTERNAL_ERROR (-1): ";
    errMsg += detail;

    Status s1(INTERNAL_ERROR, detail);
    EXPECT_TRUE(!s1.IsOk());
    EXPECT_EQ(s1.Code(), INTERNAL_ERROR);
    EXPECT_STREQ(s1.GetMessage(), detail.c_str());
    EXPECT_EQ(s1.ToString(), errMsg);

    detail = "This operation is not allowed for your accout, "
             "contact administrator please.";
    errMsg = "INVALID_PARAMETER (-2): ";
    errMsg += detail;
    Status s2(INVALID_PARAMETER, detail);
    EXPECT_TRUE(!s2.IsOk());
    EXPECT_EQ(s2.Code(), INVALID_PARAMETER);
    EXPECT_STREQ(s2.GetMessage(), detail.c_str());
    EXPECT_EQ(s2.ToString(), errMsg);

    s1 = s2;
    EXPECT_TRUE(!s1.IsOk());
    EXPECT_EQ(s1.Code(), INVALID_PARAMETER);
    EXPECT_STREQ(s1.GetMessage(), detail.c_str());
    EXPECT_EQ(s1.ToString(), errMsg);
}

TEST(StatusTest, FirstFailureStatus)
{
    Status status1 = OK;
    Status status2 = OK;
    EXPECT_OK(FirstFailureOf(status1, status2));
    EXPECT_ERROR(Status(INTERNAL_ERROR), FirstFailureOf(status1, INTERNAL_ERROR));
    EXPECT_ERROR(Status(INTERNAL_ERROR), FirstFailureOf(INTERNAL_ERROR, status1));
}

TEST(StatusTest, FirstFailureInt)
{
    EXPECT_OK(FirstFailureOf(OK, OK));
    EXPECT_ERROR(INTERNAL_ERROR, FirstFailureOf(OK, INTERNAL_ERROR));
    EXPECT_ERROR(INTERNAL_ERROR, FirstFailureOf(INTERNAL_ERROR, OK));
}
