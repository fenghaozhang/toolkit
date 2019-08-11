#include "src/string/string_util.h"

#include <ctype.h>
#include <errno.h>
#include <limits>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "src/common/macro.h"
#include "src/string/dmg_fp/dmg_fp.h"

namespace {

const char* kTrueString = "true";
const char* kFalseString = "false";

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT
{
    // This is to avoid a compiler warning about unary minus on unsigned type.
    // For example, say you had the following code:
    //   template <typename INT>
    //   INT abs(INT value) { return value < 0 ? -value : value; }
    // Even though if INT is unsigned, it's impossible for value < 0, so the
    // unary minus will never be taken, the compiler will still generate a
    // warning.  We do a little specialization dance...
    template <typename INT2, typename UINT2, bool NEG2>
    struct ToUnsignedT {};

    template <typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, false>
    {
        static UINT2 ToUnsigned(INT2 value)
        {
            return static_cast<UINT2>(value);
        }
    };

    template <typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, true>
    {
        static UINT2 ToUnsigned(INT2 value)
        {
            return static_cast<UINT2>(value < 0 ? -value : value);
        }
    };

    // This set of templates is very similar to the above templates, but
    // for testing whether an integer is negative.
    template <typename INT2, bool NEG2>
    struct TestNegT {};
    template <typename INT2>
    struct TestNegT<INT2, false>
    {
        static bool TestNeg(INT2 value)
        {
            // value is unsigned, and can never be negative.
            return false;
        }
    };
    template <typename INT2>
    struct TestNegT<INT2, true>
    {
        static bool TestNeg(INT2 value)
        {
            return value < 0;
        }
    };

    static STR IntToString(INT value)
    {
        // log10(2) ~= 0.3 bytes needed per bit or per byte log10(2**8) ~= 2.4.
        // So round up to allocate 3 output characters per byte, plus 1 for '-'.
        const int kOutputBufSize = 3 * sizeof(INT) + 1;

        // Allocate the whole string right away, we will right back to front,
        // and then return the substr of what we ended up using.
        STR outbuf(kOutputBufSize, 0);

        bool is_neg = TestNegT<INT, NEG>::TestNeg(value);
        // Even though is_neg will never be true when INT is parameterized as
        // unsigned, even the presence of the unary operation causes a warning.
        UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

        for (typename STR::iterator it = outbuf.end();;) {
            --it;
            *it = static_cast<typename STR::value_type>((res % 10) + '0');
            res /= 10;

            // We're done..
            if (res == 0) {
                if (is_neg) {
                    --it;
                    *it = static_cast<typename STR::value_type>('-');
                }
                return STR(it, outbuf.end());
            }
        }
        return STR();
    }
};

// Utility to convert a character to a digit in a given base
template<typename CHAR, int BASE, bool BASE_LTE_10>
class BaseCharToDigit
{
};

// Faster specialization for bases <= 10
template<typename CHAR, int BASE>
class BaseCharToDigit<CHAR, BASE, true>
{
public:
    static bool Convert(CHAR c, unsigned char* digit) {
        if (c >= '0' && c < '0' + BASE) {
            *digit = c - '0';
            return true;
        }
        return false;
    }
};

// Specialization for bases where 10 < base <= 36
template<typename CHAR, int BASE>
class BaseCharToDigit<CHAR, BASE, false>
{
public:
    static bool Convert(CHAR c, unsigned char* digit)
    {
        if (c >= '0' && c <= '9') {
            *digit = c - '0';
        } else if (c >= 'a' && c < 'a' + BASE - 10) {
            *digit = c - 'a' + 10;
        } else if (c >= 'A' && c < 'A' + BASE - 10) {
            *digit = c - 'A' + 10;
        } else {
            return false;
        }
        return true;
    }
};

template<int BASE, typename CHAR>
bool CharToDigit(CHAR c, unsigned char* digit)
{
    return BaseCharToDigit<CHAR, BASE, BASE <= 10>::Convert(c, digit);
}

// There is an IsWhitespace for wchars defined in string_util.h, but it is
// locale independent, whereas the functions we are replacing were
// locale-dependent. TBD what is desired, but for the moment let's not introduce
// a change in behaviour.
template<typename CHAR> class WhitespaceHelper
{
};

template<> class WhitespaceHelper<char>
{
public:
    static bool Invoke(char c)
    {
        return 0 != isspace(static_cast<unsigned char>(c));
    }
};

template<> class WhitespaceHelper<wchar_t>
{
public:
    static bool Invoke(wchar_t c)
    {
        return 0 != iswspace(c);
    }
};

template<typename CHAR> bool LocalIsWhitespace(CHAR c)
{
    return WhitespaceHelper<CHAR>::Invoke(c);
}

// IteratorRangeToNumberTraits should provide:
//  - a typedef for iterator_type, the iterator type used as input.
//  - a typedef for value_type, the target numeric type.
//  - static functions min, max (returning the minimum and maximum permitted
//    values)
//  - constant kBase, the base in which to interpret the input
template<typename IteratorRangeToNumberTraits>
class IteratorRangeToNumber
{
public:
    typedef IteratorRangeToNumberTraits traits;
    typedef typename traits::iterator_type const_iterator;
    typedef typename traits::value_type value_type;

    // Generalized iterator-range-to-number conversion.
    //
    static bool Invoke(const_iterator begin,
                       const_iterator end,
                       value_type* output)
    {
        bool valid = true;

        while (begin != end && LocalIsWhitespace(*begin)) {
            valid = false;
            ++begin;
        }

        if (begin != end && *begin == '-') {
            if (!Negative::Invoke(begin + 1, end, output)) {
                valid = false;
            }
        } else {
            if (begin != end && *begin == '+') {
                ++begin;
            }
            if (!Positive::Invoke(begin, end, output)) {
                valid = false;
            }
        }

        return valid;
    }

private:
    // Sign provides:
    //  - a static function, CheckBounds, that determines whether the next digit
    //    causes an overflow/underflow
    //  - a static function, Increment, that appends the next digit
    //    appropriately according to the sign of the number being parsed.
    template<typename Sign>
    class Base
    {
    public:
        static bool Invoke(const_iterator begin, const_iterator end,
                           typename traits::value_type* output)
        {
            *output = 0;

            if (begin == end) {
                return false;
            }

            // Note: no performance difference was found when using template
            // specialization to remove this check in bases other than 16
            if (traits::kBase == 16 && end - begin > 2 && *begin == '0' &&
                (*(begin + 1) == 'x' || *(begin + 1) == 'X')) {
                begin += 2;
            }

            for (const_iterator current = begin; current != end; ++current) {
                unsigned char new_digit = 0;

                if (!CharToDigit<traits::kBase>(*current, &new_digit)) {
                    return false;
                }

                if (current != begin) {
                    if (!Sign::CheckBounds(output, new_digit)) {
                        return false;
                    }
                    *output *= traits::kBase;
                }

                Sign::Increment(new_digit, output);
            }
            return true;
        }
    };

    class Positive : public Base<Positive>
    {
    public:
        static bool CheckBounds(value_type* output, unsigned char new_digit)
        {
            value_type baseMax =
                static_cast<value_type>(traits::max() / traits::kBase);
            if (*output > baseMax ||
                (*output == baseMax &&
                 new_digit > traits::max() % traits::kBase)) {
                *output = traits::max();
                return false;
            }
            return true;
        }

        static void Increment(unsigned char increment, value_type* output)
        {
            *output += increment;
        }
    };

    class Negative : public Base<Negative>
    {
    public:
        static bool CheckBounds(value_type* output, unsigned char new_digit)
        {
            if (*output < traits::min() / traits::kBase ||
                (*output == traits::min() / traits::kBase &&
                 new_digit > 0 - traits::min() % traits::kBase)) {
                *output = traits::min();
                return false;
            }
            return true;
        }
        static void Increment(unsigned char increment, value_type* output)
        {
            *output -= increment;
        }
    };
};

template<typename ITERATOR, typename VALUE, int BASE>
class BaseIteratorRangeToNumberTraits
{
public:
    typedef ITERATOR iterator_type;
    typedef VALUE value_type;
    static value_type min() {
        return std::numeric_limits<value_type>::min();
    }
    static value_type max() {
        return std::numeric_limits<value_type>::max();
    }
    static const int kBase = BASE;
};

template<typename ITERATOR>
class BaseHexIteratorRangeToIntTraits
: public BaseIteratorRangeToNumberTraits<ITERATOR, int, 16>
{
};

template<typename ITERATOR>
class BaseHexIteratorRangeToInt64Traits
: public BaseIteratorRangeToNumberTraits<ITERATOR, int64_t, 16>
{
};

template<typename ITERATOR>
class BaseHexIteratorRangeToUInt64Traits
: public BaseIteratorRangeToNumberTraits<ITERATOR, uint64_t, 16>
{
};

typedef BaseHexIteratorRangeToIntTraits<std::string::const_iterator>
HexIteratorRangeToIntTraits;

typedef BaseHexIteratorRangeToInt64Traits<std::string::const_iterator>
HexIteratorRangeToInt64Traits;

typedef BaseHexIteratorRangeToUInt64Traits<std::string::const_iterator>
HexIteratorRangeToUInt64Traits;

template<typename STR>
bool HexStringToBytesT(const STR& input, std::vector<unsigned char>* output)
{
    size_t count = input.size();
    if (count == 0 || (count % 2) != 0)
        return false;
    for (uintptr_t i = 0; i < count / 2; ++i) {
        unsigned char msb = 0;  // most significant 4 bits
        unsigned char lsb = 0;  // least significant 4 bits
        if (!CharToDigit<16>(input[i * 2], &msb) ||
            !CharToDigit<16>(input[i * 2 + 1], &lsb))
            return false;
        output->push_back((msb << 4) | lsb);
    }
    return true;
}

template <typename VALUE, int BASE>
class StringToNumberTraits
: public BaseIteratorRangeToNumberTraits<std::string::const_iterator,
    VALUE,
    BASE>
{
};

template <typename VALUE>
bool StringToIntImpl(const std::string& input, VALUE* output)
{
    return IteratorRangeToNumber<StringToNumberTraits<VALUE, 10> >::Invoke(
        input.begin(), input.end(), output);
}

struct Symbol
{
    char     unit;
    uint8_t  bits;
};

const Symbol SymbolTable[] = {
    { 'K', 10 },
    { 'M', 20 },
    { 'G', 30 },
    { 'T', 40 },
    { 'P', 50 },
};

}  // namespace

std::string IntToString(int value) {
    return IntToStringT<std::string, int, unsigned int, true>::
        IntToString(value);
}

// std::wstring IntToWString(int value) {
//     return IntToStringT<std::wstring, int, unsigned int, true>::
//         IntToString(value);
// }

std::string UintToString(unsigned int value) {
    return IntToStringT<std::string, unsigned int, unsigned int, false>::
        IntToString(value);
}

// std::wstring UintToWString(unsigned int value) {
//     return IntToStringT<std::wstring, unsigned int, unsigned int, false>::
//         IntToString(value);
// }

std::string Int64ToString(int64_t value) {
    return IntToStringT<std::string, int64_t, uint64_t, true>::
        IntToString(value);
}

// std::wstring Int64ToWString(int64_t value) {
//     return IntToStringT<std::wstring, int64_t, uint64_t, true>::
//                IntToString(value);
// }

std::string Uint64ToString(uint64_t value) {
    return IntToStringT<std::string, uint64_t, uint64_t, false>::
        IntToString(value);
}

// std::wstring Uint64ToWString(uint64_t value) {
//     return IntToStringT<std::wstring, uint64_t, uint64_t, false>::
//         IntToString(value);
// }

std::string DoubleToString(double value) {
    // According to g_fmt.cc, it is sufficient to declare a buffer of size 32.
    char buffer[32];
    dmg_fp::g_fmt(buffer, value);
    return std::string(buffer);
}

bool StringToInt(const std::string& input, int* output) {
    return StringToIntImpl(input, output);
}

// bool StringToInt(const std::wstring& input, int* output) {
//     return WStringToIntImpl(input, output);
// }

bool StringToUint(const std::string& input, unsigned* output) {
    return StringToIntImpl(input, output);
}

// bool StringToUint(const std::wstring& input, unsigned* output) {
//     return WStringToIntImpl(input, output);
// }

bool StringToInt64(const std::string& input, int64_t* output) {
    return StringToIntImpl(input, output);
}

// bool StringToInt64(const std::wstring& input, int64_t* output) {
//     return WStringToIntImpl(input, output);
// }

bool StringToUint64(const std::string& input, uint64_t* output) {
    return StringToIntImpl(input, output);
}

// bool StringToUint64(const std::wstring& input, uint64_t* output) {
//     return WStringToIntImpl(input, output);
// }

bool StringToSizeT(const std::string& input, size_t* output) {
    return StringToIntImpl(input, output);
}

// bool StringToSizeT(const std::wstring& input, size_t* output) {
//     return WStringToIntImpl(input, output);
// }

bool StringToDouble(const std::string& input, double* output)
{
    errno = 0;  // Thread-safe?  It is on at least Mac, Linux, and Windows.
    char* endptr = NULL;
    *output = dmg_fp::strtod(input.c_str(), &endptr);

    // Cases to return false:
    //  - If errno is ERANGE, there was an overflow or underflow.
    //  - If the input string is empty, there was nothing to parse.
    //  - If endptr does not point to the end of the string, there are either
    //    characters remaining in the string after a parsed number, or the
    //    string
    //    does not begin with a parseable number.  endptr is compared to the
    //    expected end given the string's stated length to correctly catch cases
    //    where the string contains embedded NUL characters.
    //  - If the first character is a space, there was leading whitespace
    return errno == 0 &&
        !input.empty() &&
        input.c_str() + input.length() == endptr &&
        !isspace(input[0]);
}

bool StringToBool(const std::string& input, bool* output)
{
    std::string lowCase = StringToLowerASCII(input);
    bool ret = false;
    if (lowCase == kTrueString) {
        *output = ret = true;
    } else if (lowCase == kFalseString) {
        *output = false;
        ret = true;
    }
    return ret;
}

std::string BoolToString(const bool input)
{
    if (input == true) {
        return kTrueString;
    } else {
        return kFalseString;
    }
}

// Note: if you need to add WStringToDouble, first ask yourself if it's
// really necessary. If it is, probably the best implementation here is to
// convert to 8-bit and then use the 8-bit version.

// Note: if you need to add an iterator range version of StringToDouble, first
// ask yourself if it's really necessary. If it is, probably the best
// implementation here is to instantiate a string and use the string version.

std::string HexEncode(const void* bytes, size_t size)
{
    static const char kHexChars[] = "0123456789ABCDEF";

    // Each input byte creates two output hex characters.
    std::string ret(size * 2, '\0');

    for (size_t i = 0; i < size; ++i) {
        char b = reinterpret_cast<const char*>(bytes)[i];
        ret[(i * 2)] = kHexChars[(b >> 4) & 0xf];
        ret[(i * 2) + 1] = kHexChars[b & 0xf];
    }
    return ret;
}

bool HexStringToInt(const std::string& input, int* output)
{
    return IteratorRangeToNumber<HexIteratorRangeToIntTraits>::Invoke(
        input.begin(), input.end(), output);
}

bool HexStringToInt64(const std::string& input, int64_t* output)
{
    return IteratorRangeToNumber<HexIteratorRangeToInt64Traits>::Invoke(
        input.begin(), input.end(), output);
}

bool HexStringToUInt64(const std::string& input, uint64_t* output)
{
    return IteratorRangeToNumber<HexIteratorRangeToUInt64Traits>::Invoke(
        input.begin(), input.end(), output);
}

bool HexStringToBytes(const std::string& input,
                      std::vector<unsigned char>* output)
{
    return HexStringToBytesT(input, output);
}

bool ParseFromSize(const std::string& s, uint64_t* size)
{
    if (s.empty())
    {
        return false;
    }
    if (size == NULL)
    {
        return false;
    }
    size_t length = s.length();
    uint8_t bits = 0;
    for (size_t i = 0; i < COUNT_OF(SymbolTable); i++)
    {
        const Symbol& symbol = SymbolTable[i];
        char unit = s[s.length() - 1];
        if (toupper(unit) == symbol.unit)
        {
            bits = symbol.bits;
            length = s.length() - 1;  // skip last char
            break;
        }
    }
    if (!StringToSizeT(s.substr(0, length), size))
    {
        return false;
    }
    *size = *size << bits;
    return true;
}

template<typename CHAR> struct CaseInsensitiveCompare
{
public:
    bool operator()(CHAR x, CHAR y) const
    {
        return tolower(x) == tolower(y);
    }
};

bool StartWith(const std::string& s, const std::string prefix)
{
    if (s.length() < prefix.length())
    {
        return false;
    }
    for (size_t i = 0; i < prefix.length(); i++)
    {
        if (s[i] != prefix[i])
        {
            return false;
        }
    }
    return true;
}

int64_t StringToLongLong(const std::string& s)
{
    return atoll(s.c_str());
}

template<typename STR>
static size_t SplitStringT(
        const STR& str,
        const STR& delimiters,
        std::vector<STR>* tokens)
{
    tokens->clear();
    typename STR::size_type start =
        str.find_first_not_of(delimiters);
    while (start != STR::npos) {
        typename STR::size_type end =
            str.find_first_of(delimiters, start + 1);
        if (end == STR::npos) {
            tokens->push_back(str.substr(start));
            break;
        } else {
            tokens->push_back(str.substr(start, end - start));
            start = str.find_first_not_of(delimiters, end + 1);
        }
    }

    return tokens->size();
}

size_t SplitString(const std::string& str,
                   const std::string& delimiters,
                   std::vector<std::string>* tokens)
{
    return SplitStringT(str, delimiters, tokens);
}

template <typename STR>
bool EndsWithT(const STR& str, const STR& search, bool case_sensitive)
{
    typename STR::size_type str_length = str.length();
    typename STR::size_type search_length = search.length();
    if (search_length > str_length)
        return false;
    if (case_sensitive) {
        return
            str.compare(str_length - search_length, search_length, search) == 0;
    } else {
        return
            std::equal(search.begin(), search.end(),
                       str.begin() + (str_length - search_length),
                       CaseInsensitiveCompare<typename STR::value_type>());
    }
}

bool EndsWith(const std::string& str, const std::string& search,
              bool case_sensitive)
{
    return EndsWithT(str, search, case_sensitive);
}

std::string LeftTrimString(const std::string& string, const char trimChar)
{
    size_t pos = 0;
    while (pos < string.size() && string[pos] == trimChar) ++pos;
    return string.substr(pos);
}

std::string RightTrimString(const std::string& string, const char trimChar)
{
    size_t pos = string.size() - 1;
    while (pos != (size_t)-1  && string[pos] == trimChar) --pos;
    return string.substr(0, pos+1);
}

std::string TrimString(
        const std::string& string,
        const char leftTrimChar,
        const char rightTrimChar)
{
    return LeftTrimString(RightTrimString(string, rightTrimChar), leftTrimChar);
}

static const int kPathChar = '/';
static const int kDotChar = '.';

char* NameOf(const char* filename, char* s, size_t maxChars)
{
    const char *t = strrchr(filename, kPathChar);
    return strncpy(s, (t == NULL) ? filename : (t + 1), maxChars);
}

char* BaseOf(const char* filename, char* s, size_t maxChars)
{ NameOf(filename, s, maxChars);
    char* t = strrchr(s, kDotChar);
    if (t == NULL) { return s; }
    if (t >= s) { *t = '\0'; }
    return s;
}

char* PathOf(const char* filename, char* s, size_t maxChars)
{
    const char* t = strrchr(filename, kPathChar);
    if (t == NULL)
    {
        *s = '\0';
    }
    else
    {
        const size_t n = std::min<size_t>(maxChars, t + 1 - filename);
        strncpy(s, filename, n);
        s[n] = '\0';
    }
    return s;
}

char* ExtnOf(const char* filename, char *s, size_t maxChars)
{
    const char* t = strrchr(filename, kDotChar);
    if (t == NULL)
    {
        *s = '\0';
        return s;
    }
    return strncpy(s, t + 1, maxChars);
}

char* FullBaseOf(const char* filename, char* s, size_t maxChars)
{
    const char* t = strrchr(filename, kPathChar);
    if (t == NULL)
    {
        BaseOf(filename, s, maxChars);
    }
    else
    {
        const size_t n = std::min<size_t>(maxChars, t + 1 - filename);
        strncpy(s, filename, n);
        BaseOf(filename + n, s + n, maxChars - n);
    }
    return s;
}

bool RemovePrefix(
        const std::string& str,
        const std::string& prefix,
        std::string* result)
{
    if (str.length() < prefix.length())
    {
        return false;
    }
    if (memcmp(str.c_str(), prefix.c_str(), prefix.length()) != 0)
    {
        return false;
    }
    size_t newLength = str.length() - prefix.length();
    result->assign(str, prefix.length(), newLength);
    return true;
}

bool RemovePrefixInPlace(std::string* str, const std::string& prefix)
{
    return RemovePrefix(*str, prefix, str);
}

bool RemoveSuffix(
        const std::string& str,
        const std::string& suffix,
        std::string* result)
{
    if (str.length() < suffix.length())
    {
        return false;
    }
    size_t newLength = str.length() - suffix.length();
    if (memcmp(&str.c_str()[newLength],
               suffix.c_str(), suffix.length()) != 0)
    {
        return false;
    }
    result->assign(str, 0, newLength);
    return true;
}

bool RemoveSuffixInPlace(
        std::string* str,
        const std::string& suffix)
{
    return RemoveSuffix(*str, suffix, str);
}

std::string ConcatPath(const std::string& s1, const std::string& s2)
{
    std::string path = s1;
    ConcatPathInPlace(&path, s2);
    return path;
}

void ConcatPathInPlace(std::string* s1, const std::string& s2)
{
    // The below code is taken from stone/file/file_system.h with slight
    // modification.
    s1->reserve(s1->size() + s2.size() + 1);
    const char seperator = '/';
    if (!s1->empty() && (*s1)[s1->size() - 1] != seperator)
    {
        s1->push_back(seperator);
    }
    s1->append(s2);
}

inline void StringAppendV(std::string* out, const char* fmt, va_list vl)
{
    if (out == NULL)
    {
        return;
    }
    char* s = NULL;
    int n = vasprintf(&s, fmt, vl);
    if (n < 0)
    {
        return;  // format string is wrong
    }
    out->append(s, n);
    free(s);
    s = NULL;
}

std::string StringPrintf(const char* fmt, ...)
{
    std::string s;
    va_list vl;
    va_start(vl, fmt);
    StringAppendV(&s, fmt, vl);
    va_end(vl);
    return s;
}

void StringAppendF(std::string* out, const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    StringAppendV(out, fmt, vl);
    va_end(vl);
}
