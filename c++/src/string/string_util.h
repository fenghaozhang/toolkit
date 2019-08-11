#ifndef PANGU2_COMMON_STRING_STRING_UTIL_H
#define PANGU2_COMMON_STRING_STRING_UTIL_H

#include <stdint.h>
#include <string>
#include <vector>

/** --------------- String Conversions -------------- */

std::string IntToString(int value);
// std::wstring IntToWString(int value);

std::string UintToString(unsigned value);
// std::wstring UintToWString(unsigned value);

std::string Int64ToString(int64_t value);
// std::wstring Int64ToWString(int64_t value);

std::string Uint64ToString(uint64_t value);
// std::wstring Uint64ToWString(uint64_t value);

// DoubleToString converts the double to a string format that ignores the
// locale. If you want to use locale specific formatting, use ICU.
std::string DoubleToString(double value);

// String -> number conversions ------------------------------------------------

// Perform a best-effort conversion of the input string to a numeric type,
// setting |*output| to the result of the conversion.  Returns true for
// "perfect" conversions; returns false in the following cases:
//  - Overflow/underflow.  |*output| will be set to the maximum value supported
//    by the data type.
//  - Trailing characters in the string after parsing the number.  |*output|
//    will be set to the value of the number that was parsed.
//  - Leading whitespace in the string before parsing the number. |*output| will
//    be set to the value of the number that was parsed.
//  - No characters parseable as a number at the beginning of the string.
//    |*output| will be set to 0.
//  - Empty string.  |*output| will be set to 0.
bool StringToInt(const std::string& input, int* output);
// bool StringToInt(const std::wstring& input, int* output);

bool StringToUint(const std::string& input, unsigned* output);
// bool StringToUint(const std::wstring& input, unsigned* output);

bool StringToInt64(const std::string& input, int64_t* output);
// bool StringToInt64(const std::wstring& input, int64_t* output);

bool StringToUint64(const std::string& input, uint64_t* output);
// bool StringToUint64(const std::wstring& input, uint64_t* output);

bool StringToSizeT(const std::string& input, size_t* output);
// bool StringToSizeT(const std::wstring& input, size_t* output);

// For floating-point conversions, only conversions of input strings in decimal
// form are defined to work.  Behavior with strings representing floating-point
// numbers in hexadecimal, and strings representing non-fininte values (such as
// NaN and inf) is undefined.  Otherwise, these behave the same as the integral
// variants.  This expects the input string to NOT be specific to the locale.
// If your input is locale specific, use ICU to read the number.
bool StringToDouble(const std::string& input, double* output);

// String <-> bool conversion --------------------------------------------------

// parameters:
//   input: can be uppper/lower or mixed case strings of "true"/"false". E.g.:
//          "true", "TRUE", "True", "TrUe" etc are all valid.
//   output: the converted result
//
// return:
//   indicate whether the conversion is succeeded or failed.
//   E.g.: StringToBool("truee", &output) returns false.
bool StringToBool(const std::string& input, bool* output);

// parameters:
//   input: bool type true/false
//
// return:
//   returns lower case strings "true"/"false"
std::string BoolToString(const bool input);

// Hex encoding ----------------------------------------------------------------

// Returns a hex string representation of a binary buffer. The returned hex
// string will be in upper case. This function does not check if |size| is
// within reasonable limits since it's written with trusted data in mind.  If
// you suspect that the data you want to format might be large, the absolute
// max size for |size| should be is
//   std::numeric_limits<size_t>::max() / 2
std::string HexEncode(const void* bytes, size_t size);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// -0x80000000 < |input| < 0x7FFFFFFF.
bool HexStringToInt(const std::string& input, int* output);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// -0x8000000000000000 < |input| < 0x7FFFFFFFFFFFFFFF.
bool HexStringToInt64(const std::string& input, int64_t* output);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// 0x0000000000000000 < |input| < 0xFFFFFFFFFFFFFFFF.
// The string is not required to start with 0x.
bool HexStringToUInt64(const std::string& input, uint64_t* output);

// Similar to the previous functions, except that output is a vector of bytes.
// |*output| will contain as many bytes as were successfully parsed prior to the
// error.  There is no overflow, but input.size() must be evenly divisible by 2.
// Leading 0x or +/- are not allowed.
bool HexStringToBytes(const std::string& input,
                      std::vector<unsigned char>* output);

// up<->low case  --------------------------------------------------------------

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToLowerASCII(Char c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToUpperASCII(Char c) {
    return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToLowerASCII(str* s)
{
    for (typename str::iterator i = s->begin(); i != s->end(); ++i)
        *i = ToLowerASCII(*i);
}

template <class str> inline str StringToLowerASCII(const str& s)
{
    // for std::string and std::wstring
    str output(s);
    StringToLowerASCII(&output);
    return output;
}

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToUpperASCII(str* s)
{
    for (typename str::iterator i = s->begin(); i != s->end(); ++i)
        *i = ToUpperASCII(*i);
}

template <class str> inline str StringToUpperASCII(const str& s)
{
    // for std::string and std::wstring
    str output(s);
    StringToUpperASCII(&output);
    return output;
}

/**
 * Parse a size in human-friendly format.
 *
 * Supporte "k/K", "m/M", "g/G", "t/T" and "p/P"
 *
 * Input  | Output
 * -------|-------
 * "0"    | 0
 * "1"    | 1
 * "1K"   | 1024
 * "1024" | 1024
 * "1M"   | 1048576
 * "0.5G" | error
 *
 * @return true if "s" successfully convert
 */
bool ParseFromSize(const std::string& s, uint64_t* size);


/** --------------- String Check & Modify -------------- */


/**
 * Test if a string has a given prefix.
 *
 * Examples:
 *
 *   StartWith("ab", "")    == true
 *   StartWith("ab", "a")   == true
 *   StartWith("ab", "ab")  == true
 *   StartWith("ab", "abc") == false
 */
bool StartWith(const std::string& s, const std::string prefix);

int64_t StringToLongLong(const std::string& s);

// Split string  --------------------------------------------------------------
// Splits a string into its fields delimited by any of the characters in
// |delimiters|.  Each field is added to the |tokens| vector.  Returns the
// number of tokens found.
size_t SplitString(
        const std::string& str,
        const std::string& delimiters,
        std::vector<std::string>* tokens);

bool EndsWith(
        const std::string& str,
        const std::string& search,
        bool case_sensitive);

/** Remove whitespaces out from head and tail */
std::string TrimString(
        const std::string& str,
        const char leftTrimChar = ' ',
        const char rightTrimChar = ' ');

std::string LeftTrimString(
        const std::string& str,
        const char trimChar = ' ');
std::string RightTrimString(const std::string& str, const char trimChar = ' ');

// File Name Handling Utils in C-style:
// A filename of the form "p/n.x", the following functions
// return "n.x", "n", "p" and "x", respectively.

char* NameOf(const char* filename, char* s, size_t maxChars);

char* BaseOf(const char* filename, char* s, size_t maxChars);

char* PathOf(const char* filename, char* s, size_t maxChars);

char* ExtnOf(const char* filename, char *s, size_t maxChars);

char* FullBaseOf(const char* filename, char* s, size_t maxChars);

/**
 * Remove prefix from a input string, and store result into another string
 * on success.
 *
 * @param str     the input string
 * @param prefix  the prefix
 * @param result  the reuslt, may be pointed to "str".
 * @return whether or not "str" starts with "prefix"
 */
bool RemovePrefix(
        const std::string& str,
        const std::string& prefix,
        std::string* result);

/** In-place variant of RemovePrefix. */
bool RemovePrefixInPlace(std::string* str, const std::string& prefix);

/**
 * Remove suffix from a input string, and store result into another string
 * on success.
 *
 * @param str     the input string
 * @param suffix  the suffix
 * @param result  the reuslt, may be pointed to "str".
 * @return whether or not "str" ends with "suffix"
 */
bool RemoveSuffix(
        const std::string& str,
        const std::string& prefix,
        std::string* result);

/** In-place variant of RemoveSuffix. */
bool RemoveSuffixInPlace(
        std::string* str,
        const std::string& suffix);

/**
 * Concatenate two paths and adding necessary slashes in between.
 *
 * The second string should be a relative path (i.e. not starting with "/"),
 * otherwise the result will look weird (see example below).
 *
 * s1   | s2  | result
 * -----|-----|---------
 * ""   | ""  | ""
 * a    | ""  | a/
 * ""   | b   | b
 * a/   | b   | a/b
 * a    | /b   | a//b
 */
std::string ConcatPath(const std::string& s1, const std::string& s2);

void ConcatPathInPlace(std::string* s1, const std::string& s2);

/** --------------- String Printf -------------- */
// The following utility functions are inspired by protobuf/stubs/stringprintf.h.

/**
 * C-style formatting method that returns a string.
 *
 * Examples:
 *
 *      StringPrintf("hello") == "hello"
 *      StringPrintf("%s %s", "hello", "world") == "hello world"
 *      StringPrintf("%d", 10) == "10"
 *
 * Note that the parameter to formatted must be valid until return of StringPrintf.
 *
 * The format string should be a string literal, so that we could leverage the
 * compiler to check it (as specified in __attribute__(...)) .
 */
std::string StringPrintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * Append the formatted string to another string.
 */
void StringAppendF(std::string* out, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#endif  // PANGU2_COMMON_STRING_STRING_UTIL_H
