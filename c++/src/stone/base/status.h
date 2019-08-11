// Copyright (c) 2013, Alibaba Inc.
// All right reserved.
//
// Author: DongPing HUANG <dongping.huang@alibaba-inc.com>
// Created: 2013/05/15
// Description:

#ifndef STONE_BASE_STATUS_H
#define STONE_BASE_STATUS_H

#include <string>

namespace apsara {
namespace pangu {
namespace stone {

class Status
{
public:
    enum Code
    {
        kOk = 0,
        kAgain = 1,
        kPermissionDenied = 10,
        kNotSupported = 20,
        kNotFound = 30,
        // argument error
        kInvalidArgument = 40,
        kArgumentOutOfRange = 41,
        kArgumentNull = 42,
        // file or folder error
        kIOError = 150,
        kFolderExist = 151,
        kFileExist = 152,
        kFileLocked = 153,
        // resource etc
        kOutOfResource = 260,
        kQuotaExceeded = 261,
        // network related
        kInvalidAddress = 380,
        kTimeout = 400,
        kNotReady = 410,
        kBusy = 420,
        kCanceled = 430,
        kConnClosed = 440,
        //
        kSecondary = 140,
        // others
        kOtherError = 1000,
    };

    Status(Status::Code code = Status::kOk, const char* msg = NULL);
    Status(const Status& other);
    ~Status();

    Status& operator=(Status::Code code);
    Status& operator=(const Status& other);
    Status& Assign(Status::Code code, const char* msg = NULL);

    static Status OK() { return Status(); }

    bool ok() const { return mCode == kOk; }
    Code code() const { return mCode; }

    std::string ToString() const;

private:
    char* CopyMessage(const char* msg);

    // not allowed to compare
    bool operator==(const Status& other);

private:
    Code mCode;
    char* mMsg;
};

} // namespace stone
} // namespace pangu
} // namespace apsara

#endif // STONE_BASE_STATUS_H
