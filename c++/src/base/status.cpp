#include "src/base/status.h"

#include <string.h>

#include "src/string/string_util.h"

Status::Status(CodeType code)
  : mCode(code),
    mMsg(NULL)
{
}

Status::Status(const Status& other)
  : mCode(OK),
    mMsg(NULL)
{
    CopyFrom(other);
}


Status::Status(Status::CodeType code, const char* msg)
  : mCode(OK),
    mMsg(NULL)
{
    Assign(code, msg);
}

Status::Status(Status::CodeType code, const std::string& msg)
  : mCode(OK),
    mMsg(NULL)
{
    Assign(code, msg);
}

Status::~Status()
{
    if (mMsg != NULL)
    {
        delete mMsg;
    }
}

Status& Status::Assign(Status::CodeType code)
{
    if (__builtin_expect(mMsg == NULL, 1))
    {
        // fast path
        mCode = code;
        return *this;
    }
    else
    {
        // slow path
        return Assign(code, NULL);
    }
}

Status& Status::Assign(Status::CodeType code, const char* msg)
{
    mCode = code;

    if (msg != NULL && msg[0] != '\0')
    {
        if (mMsg != NULL)
        {
            mMsg->assign(msg);
        }
        else
        {
            mMsg = new std::string(msg);
        }
    }
    else
    {
        delete mMsg;
        mMsg = NULL;
    }

    return *this;
}

Status& Status::Assign(Status::CodeType code, const std::string& msg)
{
    mCode = code;

    if (!msg.empty())
    {
        if (mMsg != NULL)
        {
            mMsg->assign(msg);
        }
        else
        {
            mMsg = new std::string(msg);
        }
    }
    else
    {
        delete mMsg;
        mMsg = NULL;
    }

    return *this;
}

const char* Status::GetMessage() const
{
    if (mMsg != NULL)
    {
        return mMsg->c_str();
    }

    return "";
}

std::string Status::ToString() const
{
    std::string result;
    StringAppendF(&result, "%s", GetErrorString(mCode).c_str());
    if (mCode != OK)
    {
        StringAppendF(&result, " (%d)", mCode);
    }
    if (mMsg != NULL)
    {
        StringAppendF(&result, ": %s", mMsg->c_str());
    }
    return result;
}

Status& Status::CopyFrom(const Status& other)
{
    if (__builtin_expect(this == &other, 0))
    {
        return *this;
    }
    mCode = other.mCode;
    if (__builtin_expect(mMsg == NULL && other.mMsg == NULL, 1))
    {
        return *this;
    }

    delete mMsg;
    mMsg = NULL;

    if (other.mMsg != NULL)
    {
        mMsg = new std::string(*other.mMsg);
    }
    return *this;
}

Status FirstFailureOf(const Status& status1, const Status& status2)
{
    if (__builtin_expect(!status1.IsOk(), 0))
    {
        return status1;
    }
    return status2;
}

int FirstFailureOf(int errorcode1, int errorcode2)
{
    if (__builtin_expect(errorcode1 != OK, 0))
    {
        return errorcode1;
    }
    if (__builtin_expect(errorcode2 != OK, 0))
    {
        return errorcode2;
    }
    return OK;
}
