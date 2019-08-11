#include <gtest/gtest.h>
#include <iostream>
#include <limits.h>
#include <limits>
#include <math.h>

#include "src/string/string_util.h"

TEST(StringUtilTest, StartWith)
{
    EXPECT_TRUE(StartWith("", ""));
    EXPECT_TRUE(StartWith("a", "a"));
    EXPECT_FALSE(StartWith("", "a"));
    EXPECT_TRUE(StartWith("ab", ""));
    EXPECT_TRUE(StartWith("ab", "a"));
    EXPECT_TRUE(StartWith("ab", "ab"));
    EXPECT_FALSE(StartWith("ab", "b"));
    EXPECT_FALSE(StartWith("ab", "abc"));
}

template <typename STR>
void SplitStringTest()
{
    std::vector<STR> r;
    size_t size;

    size = SplitString(STR("This is a string"), STR(" "), &r);
    EXPECT_EQ(4U, size);
    ASSERT_EQ(4U, r.size());
    EXPECT_EQ(r[0], STR("This"));
    EXPECT_EQ(r[1], STR("is"));
    EXPECT_EQ(r[2], STR("a"));
    EXPECT_EQ(r[3], STR("string"));
    r.clear();

    size = SplitString(STR("one,two,three"), STR(","), &r);
    EXPECT_EQ(3U, size);
    ASSERT_EQ(3U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR("two"));
    EXPECT_EQ(r[2], STR("three"));
    r.clear();

    size = SplitString(STR("one,two:three;four"), STR(",:"), &r);
    EXPECT_EQ(3U, size);
    ASSERT_EQ(3U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR("two"));
    EXPECT_EQ(r[2], STR("three;four"));
    r.clear();

    size = SplitString(STR("one,two:three;four"), STR(";,:"), &r);
    EXPECT_EQ(4U, size);
    ASSERT_EQ(4U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR("two"));
    EXPECT_EQ(r[2], STR("three"));
    EXPECT_EQ(r[3], STR("four"));
    r.clear();

    size = SplitString(STR("one, two, three"), STR(","), &r);
    EXPECT_EQ(3U, size);
    ASSERT_EQ(3U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR(" two"));
    EXPECT_EQ(r[2], STR(" three"));
    r.clear();

    size = SplitString(STR("one, two, three, "), STR(","), &r);
    EXPECT_EQ(4U, size);
    ASSERT_EQ(4U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR(" two"));
    EXPECT_EQ(r[2], STR(" three"));
    EXPECT_EQ(r[3], STR(" "));
    r.clear();

    size = SplitString(STR("one, two, three,"), STR(","), &r);
    EXPECT_EQ(3U, size);
    ASSERT_EQ(3U, r.size());
    EXPECT_EQ(r[0], STR("one"));
    EXPECT_EQ(r[1], STR(" two"));
    EXPECT_EQ(r[2], STR(" three"));
    r.clear();

    size = SplitString(STR(), STR(","), &r);
    EXPECT_EQ(0U, size);
    ASSERT_EQ(0U, r.size());
    r.clear();

    size = SplitString(STR(","), STR(","), &r);
    EXPECT_EQ(0U, size);
    ASSERT_EQ(0U, r.size());
    r.clear();

    size = SplitString(STR(",;:."), STR(".:;,"), &r);
    EXPECT_EQ(0U, size);
    ASSERT_EQ(0U, r.size());
    r.clear();

    size = SplitString(STR("\t\ta\t"), STR("\t"), &r);
    EXPECT_EQ(1U, size);
    ASSERT_EQ(1U, r.size());
    EXPECT_EQ(r[0], STR("a"));
    r.clear();

    size = SplitString(STR("\ta\t\nb\tcc"), STR("\n"), &r);
    EXPECT_EQ(2U, size);
    ASSERT_EQ(2U, r.size());
    EXPECT_EQ(r[0], STR("\ta\t"));
    EXPECT_EQ(r[1], STR("b\tcc"));
    r.clear();

    size = SplitString(STR("aaa//bbb"), STR("//"), &r);
    EXPECT_EQ(2U, size);
    ASSERT_EQ(2U, r.size());
    EXPECT_EQ(r[0], STR("aaa"));
    EXPECT_EQ(r[1], STR("bbb"));
    r.clear();
}

TEST(SplitStringTest, SplitStdString)
{
    SplitStringTest<std::string>();
}

TEST(StringUtilTest, EndsWith)
{
    EXPECT_TRUE(EndsWith("Foo.plugin", ".plugin", true));
    EXPECT_FALSE(EndsWith("Foo.Plugin", ".plugin", true));
    EXPECT_TRUE(EndsWith("Foo.plugin", ".plugin", false));
    EXPECT_TRUE(EndsWith("Foo.Plugin", ".plugin", false));
    EXPECT_FALSE(EndsWith(".plug", ".plugin", true));
    EXPECT_FALSE(EndsWith(".plug", ".plugin", false));
    EXPECT_FALSE(EndsWith("Foo.plugin Bar", ".plugin", true));
    EXPECT_FALSE(EndsWith("Foo.plugin Bar", ".plugin", false));
    EXPECT_FALSE(EndsWith(std::string(), ".plugin", false));
    EXPECT_FALSE(EndsWith(std::string(), ".plugin", true));
    EXPECT_TRUE(EndsWith("Foo.plugin", std::string(), false));
    EXPECT_TRUE(EndsWith("Foo.plugin", std::string(), true));
    EXPECT_TRUE(EndsWith(".plugin", ".plugin", false));
    EXPECT_TRUE(EndsWith(".plugin", ".plugin", true));
    EXPECT_TRUE(EndsWith(std::string(), std::string(), false));
    EXPECT_TRUE(EndsWith(std::string(), std::string(), true));
    EXPECT_TRUE(EndsWith("www.aliyun.com/", "/", true));
    EXPECT_TRUE(EndsWith("www.aliyun.com/", "/", false));
}

TEST(StringUtilTest, TrimString)
{
    EXPECT_EQ(TrimString("   "), "");
    EXPECT_EQ(TrimString("   abc"), "abc");
    EXPECT_EQ(TrimString("abc   "), "abc");
    EXPECT_EQ(TrimString("  abc  "), "abc");

    EXPECT_EQ(LeftTrimString("   "), "");
    EXPECT_EQ(LeftTrimString("   abc"), "abc");
    EXPECT_EQ(LeftTrimString("abc   "), "abc   ");
    EXPECT_EQ(LeftTrimString("  abc  "), "abc  ");

    EXPECT_EQ(RightTrimString("   "), "");
    EXPECT_EQ(RightTrimString("   abc"), "   abc");
    EXPECT_EQ(RightTrimString("abc   "), "abc");
    EXPECT_EQ(RightTrimString("  abc  "), "  abc");
}

TEST(StringUtilTest, FileNameUtils)
{
    char s[256];
    // NameOf
    EXPECT_STREQ("foo.bar", NameOf("/top/dir1/dir2/foo.bar", s, 255));
    EXPECT_STREQ("foo.bar", NameOf("foo.bar", s, 255));
    EXPECT_STREQ("", NameOf("/dir1/dir2/", s, 255));
    EXPECT_STREQ("", NameOf("", s, 255));
    // BaseOf
    EXPECT_STREQ("foo", BaseOf("/top/dir1/dir2/foo.bar", s, 255));
    EXPECT_STREQ("foo", BaseOf("foo.bar", s, 255));
    EXPECT_STREQ("", BaseOf("/dir1/dir2/", s, 255));
    EXPECT_STREQ("", BaseOf("", s, 255));
    // PathOf
    EXPECT_STREQ("/top/dir1/dir2/", PathOf("/top/dir1/dir2/foo.bar", s, 255));
    EXPECT_STREQ("", PathOf("foo.bar", s, 255));
    EXPECT_STREQ("/dir1/dir2/", PathOf("/dir1/dir2/", s, 255));
    EXPECT_STREQ("//", PathOf("//", s, 255));
    EXPECT_STREQ("", PathOf("", s, 255));
    // ExtOf
    EXPECT_STREQ("bar", ExtnOf("/top/dir1/dir2/foo.bar", s, 255));
    EXPECT_STREQ("bar", ExtnOf("foo.bar", s, 255));
    EXPECT_STREQ("", ExtnOf("/dir1/dir2/", s, 255));
    EXPECT_STREQ("", ExtnOf("/top/dir1/dir2/foo", s, 255));
    EXPECT_STREQ("", ExtnOf("", s, 255));
    // FullBaseOf
    EXPECT_STREQ("/top/dir1/dir2/foo",
                FullBaseOf("/top/dir1/dir2/foo.bar", s, 255));
    EXPECT_STREQ("foo", FullBaseOf("foo.bar", s, 255));
    EXPECT_STREQ("/dir1/dir2/", FullBaseOf("/dir1/dir2/", s, 255));
    EXPECT_STREQ("/dir1/dir2.r/foo", FullBaseOf("/dir1/dir2.r/foo", s, 255));
    EXPECT_STREQ("", FullBaseOf("", s, 255));
}

TEST(StringUtilTest, RemovePrefix)
{
    std::string s;
#define CHECK_GOOD(str, prefix, result)           \
    EXPECT_TRUE(RemovePrefix(str, prefix, &s));   \
    EXPECT_EQ(result, s);                         \
    s = str;                                      \
    EXPECT_TRUE(RemovePrefixInPlace(&s, prefix)); \
    EXPECT_EQ(result, s);
    CHECK_GOOD("", "", "");
    CHECK_GOOD("a", "", "a");
    CHECK_GOOD("a", "a", "");
    CHECK_GOOD("ab", "a", "b");
    CHECK_GOOD("ab", "ab", "");
    CHECK_GOOD("abc", "a", "bc");
    CHECK_GOOD("abc", "ab", "c");
#undef CHECK_GOOD
#define CHECK_BAD(str, prefix) \
    EXPECT_FALSE(RemovePrefix(str, prefix, &s));
    CHECK_BAD("ab", "b");
    CHECK_BAD("ab", "abc");
#undef CHECK_BAD
}

TEST(StringUtilTest, RemoveSuffix)
{
    std::string s;
#define CHECK_GOOD(str, suffix, result)           \
    EXPECT_TRUE(RemoveSuffix(str, suffix, &s));   \
    EXPECT_EQ(result, s);                         \
    s = str;                                      \
    EXPECT_TRUE(RemoveSuffixInPlace(&s, suffix)); \
    EXPECT_EQ(result, s);
    CHECK_GOOD("", "", "");
    CHECK_GOOD("a", "", "a");
    CHECK_GOOD("a", "a", "");
    CHECK_GOOD("ab", "b", "a");
    CHECK_GOOD("ab", "ab", "");
    CHECK_GOOD("abc", "c", "ab");
    CHECK_GOOD("abc", "bc", "a");
#undef CHECK_GOOD
#define CHECK_BAD(str, suffix) \
    EXPECT_FALSE(RemoveSuffix(str, suffix, &s));
    CHECK_BAD("ab", "a");
    CHECK_BAD("ab", "abc");
#undef CHECK_BAD
}

TEST(ConcatPath, Simple)
{
    EXPECT_EQ("", ConcatPath("", ""));
    EXPECT_EQ("a/", ConcatPath("a", ""));
    EXPECT_EQ("b", ConcatPath("", "b"));
    EXPECT_EQ("a/b", ConcatPath("a", "b"));
    EXPECT_EQ("a/b", ConcatPath("a/", "b"));
    EXPECT_EQ("a//b", ConcatPath("a", "/b"));
}

namespace {

template <typename INT>
struct IntToStringTest {
    INT num;
    const char* sexpected;
    const char* uexpected;
};

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
#define GG_INT64_C(x)   GG_LONGLONG(x)
#define GG_UINT64_C(x)  GG_ULONGLONG(x)

const int64_t kint64min  =
static_cast<int64_t>(GG_LONGLONG(0x8000000000000000));
const uint64_t kuint64max =
static_cast<uint64_t>(GG_LONGLONG(0xFFFFFFFFFFFFFFFF));
const int64_t kint64max  =
static_cast<int64_t>(GG_LONGLONG(0x7FFFFFFFFFFFFFFF));

#define ARRAYSIZE_UNSAFE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
}  // namespace

TEST(StringNumberConversionsTest, IntToString)
{
    static const IntToStringTest<int> int_tests[] = {
        { 0, "0", "0" },
        { -1, "-1", "4294967295" },
        { std::numeric_limits<int>::max(), "2147483647", "2147483647" },
        { std::numeric_limits<int>::min(), "-2147483648", "2147483648" },
    };
    static const IntToStringTest<int64_t> int64_tests[] = {
        { 0, "0", "0" },
        { -1, "-1", "18446744073709551615" },
        { std::numeric_limits<int64_t>::max(),
            "9223372036854775807",
            "9223372036854775807", },
        { std::numeric_limits<int64_t>::min(),
            "-9223372036854775808",
            "9223372036854775808" },
    };

    for (size_t i = 0; i < arraysize(int_tests); ++i) {
        const IntToStringTest<int>* test = &int_tests[i];
        EXPECT_EQ(IntToString(test->num), test->sexpected);
        // EXPECT_EQ(IntToWString(test->num), UTF8ToUTF16(test->sexpected));
        EXPECT_EQ(UintToString(test->num), test->uexpected);
        // EXPECT_EQ(UintToWString(test->num), UTF8ToUTF16(test->uexpected));
    }
    for (size_t i = 0; i < arraysize(int64_tests); ++i) {
        const IntToStringTest<int64_t>* test = &int64_tests[i];
        EXPECT_EQ(Int64ToString(test->num), test->sexpected);
        // EXPECT_EQ(Int64ToWString(test->num), UTF8ToUTF16(test->sexpected));
        EXPECT_EQ(Uint64ToString(test->num), test->uexpected);
        // EXPECT_EQ(Uint64ToWString(test->num), UTF8ToUTF16(test->uexpected));
    }
}

TEST(StringNumberConversionsTest, Uint64ToString)
{
    static const struct {
        uint64_t input;
        std::string output;
    } cases[] = {
        {0, "0"},
        {42, "42"},
        {INT_MAX, "2147483647"},
        {kuint64max, "18446744073709551615"},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
        EXPECT_EQ(cases[i].output, Uint64ToString(cases[i].input));
}

TEST(StringNumberConversionsTest, StringToInt)
{
    static const struct {
        std::string input;
        int output;
        bool success;
    } cases[] = {
        {"0", 0, true},
        {"42", 42, true},
        {"42\x99", 42, false},
        {"\x99" "42\x99", 0, false},
        {"-2147483648", INT_MIN, true},
        {"2147483647", INT_MAX, true},
        {"", 0, false},
        {" 42", 42, false},
        {"42 ", 42, false},
        {"\t\n\v\f\r 42", 42, false},
        {"blah42", 0, false},
        {"42blah", 42, false},
        {"blah42blah", 0, false},
        {"-273.15", -273, false},
        {"+98.6", 98, false},
        {"--123", 0, false},
        {"++123", 0, false},
        {"-+123", 0, false},
        {"+-123", 0, false},
        {"-", 0, false},
        {"-2147483649", INT_MIN, false},
        {"-99999999999", INT_MIN, false},
        {"2147483648", INT_MAX, false},
        {"99999999999", INT_MAX, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        int output = 0;
        EXPECT_EQ(cases[i].success, StringToInt(cases[i].input, &output));
        EXPECT_EQ(cases[i].output, output);

        // string16 utf16_input = UTF8ToUTF16(cases[i].input);
        // output = 0;
        // EXPECT_EQ(cases[i].success, StringToInt(utf16_input, &output));
        // EXPECT_EQ(cases[i].output, output);
    }

    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "6\06";
    std::string input_string(input, arraysize(input) - 1);
    int output;
    EXPECT_FALSE(StringToInt(input_string, &output));
    EXPECT_EQ(6, output);

    // string16 utf16_input = UTF8ToUTF16(input_string);
    // output = 0;
    // EXPECT_FALSE(StringToInt(utf16_input, &output));
    // EXPECT_EQ(6, output);

    // output = 0;
    // const char16 negative_wide_input[] = { 0xFF4D, '4', '2', 0};
    // EXPECT_FALSE(StringToInt(string16(negative_wide_input), &output));
    // EXPECT_EQ(0, output);
}

TEST(StringNumberConversionsTest, StringToInt64)
{
    static const struct {
        std::string input;
        int64_t output;
        bool success;
    } cases[] = {
        {"0", 0, true},
        {"42", 42, true},
        {"-2147483648", INT_MIN, true},
        {"2147483647", INT_MAX, true},
        {"-2147483649", GG_INT64_C(-2147483649), true},
        {"-99999999999", GG_INT64_C(-99999999999), true},
        {"2147483648", GG_INT64_C(2147483648), true},
        {"99999999999", GG_INT64_C(99999999999), true},
        {"9223372036854775807", kint64max, true},
        {"-9223372036854775808", kint64min, true},
        {"09", 9, true},
        {"-09", -9, true},
        {"", 0, false},
        {" 42", 42, false},
        {"42 ", 42, false},
        {"0x42", 0, false},
        {"\t\n\v\f\r 42", 42, false},
        {"blah42", 0, false},
        {"42blah", 42, false},
        {"blah42blah", 0, false},
        {"-273.15", -273, false},
        {"+98.6", 98, false},
        {"--123", 0, false},
        {"++123", 0, false},
        {"-+123", 0, false},
        {"+-123", 0, false},
        {"-", 0, false},
        {"-9223372036854775809", kint64min, false},
        {"-99999999999999999999", kint64min, false},
        {"9223372036854775808", kint64max, false},
        {"99999999999999999999", kint64max, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        int64_t output = 0;
        EXPECT_EQ(cases[i].success, StringToInt64(cases[i].input, &output));
        EXPECT_EQ(cases[i].output, output);

        // string16 utf16_input = UTF8ToUTF16(cases[i].input);
        // output = 0;
        // EXPECT_EQ(cases[i].success, StringToInt64(utf16_input, &output));
        // EXPECT_EQ(cases[i].output, output);
    }

    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "6\06";
    std::string input_string(input, arraysize(input) - 1);
    int64_t output;
    EXPECT_FALSE(StringToInt64(input_string, &output));
    EXPECT_EQ(6, output);

    // string16 utf16_input = UTF8ToUTF16(input_string);
    // output = 0;
    // EXPECT_FALSE(StringToInt64(utf16_input, &output));
    // EXPECT_EQ(6, output);
}

TEST(StringNumberConversionsTest, HexStringToInt)
{
    static const struct {
        std::string input;
        int64_t output;
        bool success;
    } cases[] = {
        {"0", 0, true},
        {"42", 66, true},
        {"-42", -66, true},
        {"+42", 66, true},
        {"7fffffff", INT_MAX, true},
        {"-80000000", INT_MIN, true},
        {"80000000", INT_MAX, false},  // Overflow test.
        {"-80000001", INT_MIN, false},  // Underflow test.
        {"0x42", 66, true},
        {"-0x42", -66, true},
        {"+0x42", 66, true},
        {"0x7fffffff", INT_MAX, true},
        {"-0x80000000", INT_MIN, true},
        {"-80000000", INT_MIN, true},
        {"80000000", INT_MAX, false},  // Overflow test.
        {"-80000001", INT_MIN, false},  // Underflow test.
        {"0x0f", 15, true},
        {"0f", 15, true},
        {" 45", 0x45, false},
        {"\t\n\v\f\r 0x45", 0x45, false},
        {" 45", 0x45, false},
        {"45 ", 0x45, false},
        {"45:", 0x45, false},
        {"efgh", 0xef, false},
        {"0xefgh", 0xef, false},
        {"hgfe", 0, false},
        {"-", 0, false},
        {"", 0, false},
        {"0x", 0, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        int output = 0;
        EXPECT_EQ(cases[i].success, HexStringToInt(cases[i].input, &output));
        EXPECT_EQ(cases[i].output, output);
    }
    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "0xc0ffee\09";
    std::string input_string(input, arraysize(input) - 1);
    int output;
    EXPECT_FALSE(HexStringToInt(input_string, &output));
    EXPECT_EQ(0xc0ffee, output);
}

TEST(StringNumberConversionsTest, HexStringToInt64)
{
    static const struct {
        std::string input;
        int64_t output;
        bool success;
    } cases[] = {
        {"0", 0, true},
        {"42", 66, true},
        {"-42", -66, true},
        {"+42", 66, true},
        {"40acd88557b", GG_INT64_C(4444444448123), true},
        {"7fffffff", INT_MAX, true},
        {"-80000000", INT_MIN, true},
        {"ffffffff", 0xffffffff, true},
        {"DeadBeef", 0xdeadbeef, true},
        {"0x42", 66, true},
        {"-0x42", -66, true},
        {"+0x42", 66, true},
        {"0x40acd88557b", GG_INT64_C(4444444448123), true},
        {"0x7fffffff", INT_MAX, true},
        {"-0x80000000", INT_MIN, true},
        {"0xffffffff", 0xffffffff, true},
        {"0XDeadBeef", 0xdeadbeef, true},
        {"0x7fffffffffffffff", kint64max, true},
        {"-0x8000000000000000", kint64min, true},
        {"0x8000000000000000", kint64max, false},  // Overflow test.
        {"-0x8000000000000001", kint64min, false},  // Underflow test.
        {"0x0f", 15, true},
        {"0f", 15, true},
        {" 45", 0x45, false},
        {"\t\n\v\f\r 0x45", 0x45, false},
        {" 45", 0x45, false},
        {"45 ", 0x45, false},
        {"45:", 0x45, false},
        {"efgh", 0xef, false},
        {"0xefgh", 0xef, false},
        {"hgfe", 0, false},
        {"-", 0, false},
        {"", 0, false},
        {"0x", 0, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        int64_t output = 0;
        EXPECT_EQ(cases[i].success, HexStringToInt64(cases[i].input, &output));
        EXPECT_EQ(cases[i].output, output);
    }
    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "0xc0ffee\09";
    std::string input_string(input, arraysize(input) - 1);
    int64_t output;
    EXPECT_FALSE(HexStringToInt64(input_string, &output));
    EXPECT_EQ(0xc0ffee, output);
}

TEST(StringNumberConversionsTest, HexStringToUInt64)
{
    static const struct {
        std::string input;
        uint64_t output;
        bool success;
    } cases[] = {
        {"0", 0, true},
        {"42", 66, true},
        {"-42", static_cast<uint64_t>(-66), true},
        {"+42", 66, true},
        {"40acd88557b", GG_INT64_C(4444444448123), true},
        {"7fffffff", INT_MAX, true},
        {"-80000000", static_cast<uint64_t>(INT_MIN), true},
        {"ffffffff", 0xffffffff, true},
        {"DeadBeef", 0xdeadbeef, true},
        {"0x42", 66, true},
        {"-0x42", static_cast<uint64_t>(-66), true},
        {"+0x42", 66, true},
        {"0x40acd88557b", GG_INT64_C(4444444448123), true},
        {"0x7fffffff", INT_MAX, true},
        {"-0x80000000", static_cast<uint64_t>(INT_MIN), true},
        {"0xffffffff", 0xffffffff, true},
        {"0XDeadBeef", 0xdeadbeef, true},
        {"0x7fffffffffffffff", kint64max, true},
        {"-0x8000000000000000", GG_UINT64_C(0x8000000000000000), true},
        {"0x8000000000000000", GG_UINT64_C(0x8000000000000000), true},
        {"-0x8000000000000001", GG_UINT64_C(0x7fffffffffffffff), true},
        {"0xFFFFFFFFFFFFFFFF", kuint64max, true},
        {"FFFFFFFFFFFFFFFF", kuint64max, true},
        {"0x0000000000000000", 0, true},
        {"0000000000000000", 0, true},
        {"1FFFFFFFFFFFFFFFF", kuint64max, false}, // Overflow test.
        {"0x0f", 15, true},
        {"0f", 15, true},
        {" 45", 0x45, false},
        {"\t\n\v\f\r 0x45", 0x45, false},
        {" 45", 0x45, false},
        {"45 ", 0x45, false},
        {"45:", 0x45, false},
        {"efgh", 0xef, false},
        {"0xefgh", 0xef, false},
        {"hgfe", 0, false},
        {"-", 0, false},
        {"", 0, false},
        {"0x", 0, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        uint64_t output = 0;
        EXPECT_EQ(cases[i].success, HexStringToUInt64(cases[i].input, &output));
        EXPECT_EQ(cases[i].output, output);
    }
    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "0xc0ffee\09";
    std::string input_string(input, arraysize(input) - 1);
    uint64_t output;
    EXPECT_FALSE(HexStringToUInt64(input_string, &output));
    EXPECT_EQ(0xc0ffeeU, output);
}

TEST(StringNumberConversionsTest, HexStringToBytes)
{
    static const struct {
        const std::string input;
        const char* output;
        size_t output_len;
        bool success;
    } cases[] = {
        {"0", "", 0, false},  // odd number of characters fails
        {"00", "\0", 1, true},
        {"42", "\x42", 1, true},
        {"-42", "", 0, false},  // any non-hex value fails
        {"+42", "", 0, false},
        {"7fffffff", "\x7f\xff\xff\xff", 4, true},
        {"80000000", "\x80\0\0\0", 4, true},
        {"deadbeef", "\xde\xad\xbe\xef", 4, true},
        {"DeadBeef", "\xde\xad\xbe\xef", 4, true},
        {"0x42", "", 0, false},  // leading 0x fails (x is not hex)
        {"0f", "\xf", 1, true},
        {"45  ", "\x45", 1, false},
        {"efgh", "\xef", 1, false},
        {"", "", 0, false},
        {"0123456789ABCDEF", "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8, true},
        {"0123456789ABCDEF012345",
            "\x01\x23\x45\x67\x89\xAB\xCD\xEF\x01\x23\x45", 11, true},
    };


    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        std::vector<unsigned char> output;
        std::vector<unsigned char> compare;
        EXPECT_EQ(cases[i].success, HexStringToBytes(cases[i].input, &output))
            << i << ": " << cases[i].input;
        for (size_t j = 0; j < cases[i].output_len; ++j)
            compare.push_back(static_cast<unsigned char>(cases[i].output[j]));
        ASSERT_EQ(output.size(), compare.size()) << i << ": " << cases[i].input;
        EXPECT_TRUE(std::equal(output.begin(), output.end(), compare.begin()))
            << i << ": " << cases[i].input;
    }
}

TEST(StringNumberConversionsTest, StringToDouble)
{
    static const struct {
        std::string input;
        double output;
        bool success;
    } cases[] = {
        {"0", 0.0, true},
        {"42", 42.0, true},
        {"-42", -42.0, true},
        {"123.45", 123.45, true},
        {"-123.45", -123.45, true},
        {"+123.45", 123.45, true},
        {"2.99792458e8", 299792458.0, true},
        {"149597870.691E+3", 149597870691.0, true},
        {"6.", 6.0, true},
        {"9e99999999999999999999", HUGE_VAL, false},
        {"-9e99999999999999999999", -HUGE_VAL, false},
        {"1e-2", 0.01, true},
        {"42 ", 42.0, false},
        {" 1e-2", 0.01, false},
        {"1e-2 ", 0.01, false},
        {"-1E-7", -0.0000001, true},
        {"01e02", 100, true},
        {"2.3e15", 2.3e15, true},
        {"\t\n\v\f\r -123.45e2", -12345.0, false},
        {"+123 e4", 123.0, false},
        {"123e ", 123.0, false},
        {"123e", 123.0, false},
        {" 2.99", 2.99, false},
        {"1e3.4", 1000.0, false},
        {"nothing", 0.0, false},
        {"-", 0.0, false},
        {"+", 0.0, false},
        {"", 0.0, false},
        {"42.225", 42.225, true},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        double output;
        EXPECT_EQ(cases[i].success, StringToDouble(cases[i].input, &output));
        EXPECT_DOUBLE_EQ(cases[i].output, output);
    }

    double tt;
    StringToDouble("42.225", &tt);

    // One additional test to verify that conversion of numbers in strings with
    // embedded NUL characters.  The NUL and extra data after it should be
    // interpreted as junk after the number.
    const char input[] = "3.14\0159";
    std::string input_string(input, arraysize(input) - 1);
    double output;
    EXPECT_FALSE(StringToDouble(input_string, &output));
    EXPECT_DOUBLE_EQ(3.14, output);
}

TEST(StringNumberConversionsTest, DoubleToString)
{
    static const struct {
        double input;
        const char* expected;
    } cases[] = {
        {0.0, "0"},
        {1.25, "1.25"},
        {1.33518e+012, "1.33518e+12"},
        {1.33489e+012, "1.33489e+12"},
        {1.33505e+012, "1.33505e+12"},
        {1.33545e+009, "1335450000"},
        {1.33503e+009, "1335030000"},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        EXPECT_EQ(cases[i].expected, DoubleToString(cases[i].input));
    }

    // The following two values were seen in crashes in the wild.
    const char input_bytes[8] = {0, 0, 0, 0, '\xee', '\x6d', '\x73', '\x42'};
    double input = 0;
    memcpy(&input, input_bytes, arraysize(input_bytes));
    EXPECT_EQ("1335179083776", DoubleToString(input));
    const char input_bytes2[8] =
    {0, 0, 0, '\xa0', '\xda', '\x6c', '\x73', '\x42'};
    input = 0;
    memcpy(&input, input_bytes2, arraysize(input_bytes2));
    EXPECT_EQ("1334890332160", DoubleToString(input));
}

TEST(StringNumberConversionsTest, HexEncode) {
    std::string hex(HexEncode(NULL, 0));
    EXPECT_EQ(hex.length(), 0U);
    unsigned char bytes[] = {0x01, 0xff, 0x02, 0xfe, 0x03, 0x80, 0x81};
    hex = HexEncode(bytes, sizeof(bytes));
    EXPECT_EQ(hex.compare("01FF02FE038081"), 0);
}

// TEST(StringNumberConversionsTest, DoubleToStringPerfTest)
// {
//     const int kCount = 100000;
//     const double kDouble = 43.423145256667;
//     const char* kString = "43.423145256667";
//
//     std::string result;
//
//     int64_t start = stone::GetTimeStampInMs();
//     for (int i = 0; i < kCount; ++i) {
//         result = DoubleToString(kDouble);
//     }
//     int64_t end = stone::GetTimeStampInMs();
//     // int64_t chromiumTime = end - start;
//     // std::cout << "Chromium version time(ms):" << end - start << std::endl;
//     EXPECT_EQ(kString, result);
//
//     start = stone::GetTimeStampInMs();
//     for (int i = 0; i < kCount; ++i) {
//         result = apsara::ToString(kDouble);
//     }
//     end = stone::GetTimeStampInMs();
//     // int64_t apsaraTime = end - start;
//     // std::cout << "Apsara version time(ms): " << end - start << std::endl;
//     EXPECT_EQ(kString, result);
//
//     // ASSERT_LT(chromiumTime, apsaraTime);
// }
//
// TEST(StingToDoublePerfTest, FasterThanApsara)
// {
//     const int kCount = 100000;
//     const char* kString = "43.423145256667";
//     double result;
//
//     int64_t start = stone::GetTimeStampInMs();
//     for (int i = 0; i < kCount; ++i) {
//         StringToDouble(kString, &result);
//     }
//     int64_t end = stone::GetTimeStampInMs();
//     // int64_t chromiumTime = end - start;
//     // std::cout << "Chromium version time(ms):" << end - start << std::endl;
//
//     start = stone::GetTimeStampInMs();
//     for (int i = 0; i < kCount; ++i) {
//         result = apsara::StringTo<double>(kString);
//     }
//     end = stone::GetTimeStampInMs();
//     // int64_t apsaraTime = end - start;
//     // std::cout << "Apsara version time(ms): " << end - start << std::endl;
//
//     // ASSERT_LT(chromiumTime, apsaraTime);
// }

TEST(StringBoolConversionTest, StringToBool)
{
    static const struct {
        const char* input;
        bool expected;
        bool success;
    } cases[] = {
        {"TRUE", true, true},
        {"true", true, true},
        {"True", true, true},
        {"TrUe", true, true},
        {"trUe", true, true},
        {"truee", false, false},
        {"tru", false, false},
        {"FALSE", false, true},
        {"false", false, true},
        {"False", false, true},
        {"FaLse", false, true},
        {"falSe", false, true},
        {"fallllll", false, false},
        {"fa", false, false},
        {"", false, false},
        {"true false", false, false},
        {" true", false, false},
        {" false", false, false},
        {"ste 3432", false, false},
    };

    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
        bool output = false;
        StringToBool(cases[i].input, &output);
        EXPECT_EQ(cases[i].success, StringToBool(cases[i].input, &output));
        EXPECT_EQ(cases[i].expected, output);
    }
}

TEST(StringBoolConversionTest, BoolToString)
{
    EXPECT_EQ("true", BoolToString(true));
    EXPECT_EQ("false", BoolToString(false));
    EXPECT_EQ("true", BoolToString(2));
    EXPECT_EQ("false", BoolToString(0));
    EXPECT_EQ("true", BoolToString(-1));
}

TEST(HumanFriendlyFormat, ParseFromSize)
{
#define CHECK_OK(str, n)                                           \
    {                                                              \
        size_t tmp = 0;                                            \
        EXPECT_TRUE(ParseFromSize(str, &tmp));  \
        EXPECT_EQ(n, tmp);                                         \
    }
#define CHECK_ERROR(str)                                            \
    {                                                               \
        size_t tmp = 0;                                             \
        EXPECT_FALSE(ParseFromSize(str, &tmp));  \
    }
    CHECK_OK("0", 0UL);
    CHECK_OK("1", 1UL);
    CHECK_OK("001", 1UL);
    CHECK_OK("1K", 1024UL);
    CHECK_OK("1k", 1024UL);
    CHECK_OK("1024", 1024UL);
    CHECK_OK("1M", 1024UL * 1024UL);
    CHECK_OK("1G", 1024UL * 1024UL * 1024UL);
    CHECK_OK("1T", 1024UL * 1024UL * 1024UL * 1024UL);
    CHECK_OK("1P", 1024UL * 1024UL * 1024UL * 1024UL * 1024UL);
    CHECK_ERROR("");
    CHECK_ERROR("abc");
    CHECK_ERROR("1KK");
    CHECK_ERROR("1111111111111111111111111111111111111111111");
    CHECK_ERROR("1 0 0");
    CHECK_ERROR("100 ");
#undef CHECK_OK
#undef CHECK_ERROR
}

static std::string RepeatString(const std::string& s, int n)
{
    std::string t;
    t.resize(s.size() * n);
    for (int i = 0; i < n; i++)
    {
        memcpy(&t[i * s.size()], s.c_str(), s.size());
    }
    return t;
}

TEST(StringPrintfTest, StringPrintf)
{
    EXPECT_STREQ(StringPrintf("hello").c_str(), "hello");
    EXPECT_STREQ(StringPrintf("%s %s", "hello", "world").c_str(), "hello world");
    EXPECT_STREQ(StringPrintf("%s %d", "hello", 123).c_str(), "hello 123");
    std::string repeatStr = RepeatString("abc", 1000);
    EXPECT_STREQ(StringPrintf("%s %d", repeatStr.c_str(), 1000).c_str(), (repeatStr + " 1000").c_str());
}

TEST(StringPrintfTest, StringAppendF)
{
    std::string s;
    StringAppendF(&s, "%s ", "hello");
    StringAppendF(&s, "%s %d", "abc", 100);
    StringAppendF(&s, "!!!");
    EXPECT_STREQ(s.c_str(), "hello abc 100!!!");
}

TEST(StringPrintfTest, InvalidParameter1)
{
    EXPECT_STREQ(StringPrintf(NULL).c_str(), "");
}

TEST(StringPrintfTest, InvalidParameter2)
{
    std::string s = "abc";
    StringAppendF(&s, NULL);
    EXPECT_STREQ(s.c_str(), "abc");
    StringAppendF(NULL, "hello");  // should not crash
}
