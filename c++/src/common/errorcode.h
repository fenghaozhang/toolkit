#ifndef _SRC_COMMON_ERRORCODE_H
#define _SRC_COMMON_ERRORCODE_H

#include <string>

#define DEFINE_ERRORCODE(name, value, msg...)  const int name = value;
#include "src/common/errorcode_def.h"
#undef DEFINE_ERRORCODE

std::string GetErrorString(int errorCode);

void RegisterErrorCode(int errorCode, const char* symbol, const char* message);

#endif  // _SRC_COMMON_ERRORCODE_H
