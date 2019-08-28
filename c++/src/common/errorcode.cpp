#include "src/common/errorcode.h"

#include <string>

#include "src/common/assert.h"
#include "src/sync/atomic.h"
#include "src/sync/lock.h"

#define MAX_ERROR_CODE      8192
#define MAX_ERROR_CODE_MOD  4093

struct ErrorCodeInfo
{
    ~ErrorCodeInfo()
    {
        delete symbol;
        delete message;
    }
    int errorCode;
    uint32_t next;
    std::string *symbol;
    std::string *message;
    LightSpinLock lock;
} __attribute__((aligned(32)));

static ErrorCodeInfo sErrorCodeIndex[MAX_ERROR_CODE];
static uint32_t      sErrorCodeConflictId = MAX_ERROR_CODE - 1;

const std::string GetErrorMessage(int errorCode)
{
    STATIC_ASSERT(sizeof(ErrorCodeInfo) == 32);

    uint32_t pos = static_cast<uint32_t>(errorCode) % MAX_ERROR_CODE_MOD;
    ErrorCodeInfo* ptr = NULL;
    do {
        ptr = &sErrorCodeIndex[pos];
        if (ptr->errorCode == errorCode)
        {
            return *ptr->message;
        }
        pos = ptr->next;
    } while (pos != 0);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Unknown_Error_%d", errorCode);
    return buffer;
}

void AddErrorMessage(int errorCode, const std::string& symbol, const std::string& message)
{
    uint32_t pos = static_cast<uint32_t>(errorCode) % MAX_ERROR_CODE_MOD;
    ErrorCodeInfo* head = &sErrorCodeIndex[pos];

    LightSpinLock::Locker lock(head->lock);
    // Check duplication first
    ErrorCodeInfo* ptr = NULL;
    do {
        ptr = &sErrorCodeIndex[pos];
        if (ptr->errorCode == errorCode)
        {
            ASSERT_DEBUG(false);
            ASSERT(*ptr->symbol == symbol);
            return;
        }
        pos = ptr->next;
    } while (pos != 0);
    // Add new errorCode here
    if (head == ptr && ptr->errorCode == 0)
    {
        ptr->symbol = new std::string(symbol);
        ptr->message = new std::string(message);
        ASSERT(AtomicExchange(&ptr->errorCode, errorCode) == 0);
    }
    else
    {
        uint32_t nextpos = sErrorCodeConflictId--;
        ASSERT(nextpos >= MAX_ERROR_CODE_MOD);
        ErrorCodeInfo* nextptr = &sErrorCodeIndex[nextpos];
        nextptr->symbol = new std::string(symbol);
        nextptr->message = new std::string(message);
        nextptr->errorCode = errorCode;
        ASSERT(AtomicExchange(&ptr->next, nextpos) == 0);
    }
}

void RegisterErrorCode(int errorCode, const char* symbol, const char* message)
{
    if (errorCode != OK)
    {
        AddErrorMessage(errorCode, symbol, message);
    }
}

std::string GetErrorString(int errorCode)
{
    if (errorCode == OK)
    {
        return "OK";
    }
    return GetErrorMessage(errorCode);
}

struct ErrorCodeRegisterHelper
{
    ErrorCodeRegisterHelper()
    {
#define DEFINE_ERRORCODE(name, value, msg...)           \
        DEFINE_ERRORCODE_INTERNAL(name, value, #name)
#define DEFINE_ERRORCODE_INTERNAL(name, value, msg)     \
        RegisterErrorCode(value, #name, msg);
#include "src/common/errorcode_def.h"
#undef  DEFINE_ERRORCODE_INTERNAL
#undef  DEFINE_ERRORCODE
    }
};

static ErrorCodeRegisterHelper sErrorCodeRegisterHelper;
