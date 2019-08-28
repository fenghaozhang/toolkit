#ifndef _SRC_BASE_STATUS_H
#define _SRC_BASE_STATUS_H

#include <stdint.h>
#include <string.h>
#include <string>

#include "src/common/errorcode.h"

class Status
{
public:
    typedef int CodeType;

    /* implicit */ Status(CodeType code = OK);  // NO_LINT(runtime/explicit)
    Status(const Status& other);
    Status(CodeType code, const char* msg);
    Status(CodeType code, const std::string& msg);
    ~Status();

    Status& operator=(CodeType code) { return Assign(code); }
    Status& operator=(const Status& other) { return CopyFrom(other); }
    Status& Assign(CodeType code);
    Status& Assign(CodeType code, const char* msg);
    Status& Assign(CodeType code, const std::string& msg);
    Status& CopyFrom(const Status& other);

    bool IsOk() const { return mCode == OK; }
    CodeType Code() const { return mCode; }

    bool operator==(const Status& other) const { return Code() == other.Code(); }
    bool operator==(CodeType code) const { return Code() == code; }

    /**
     * Return the detailed message.  A null-terminated string will be returned,
     * which will get invalidate after reassignment to this Status object.  If
     * this status object is created without a message, an empty string will be
     * returned.
     */
    const char* GetMessage() const;

    std::string ToString() const;

private:
    CodeType mCode;
    std::string* mMsg;
};

Status FirstFailureOf(const Status& status1, const Status& status);
int FirstFailureOf(int errorcode1, int errorcode2);

#endif  // _SRC_BASE_STATUS_H
