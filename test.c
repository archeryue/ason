#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ason.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do { \
        test_count++; \
        if (equality) { \
            test_pass++; \
        } else { \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
                __FILE__, __LINE__, expect, actual); \
            main_ret = 1; \
        } \
    } while(0)

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#define EXPECT_EQ_INT(expect, actual) \
    EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) \
    EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, length) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == length && memcmp(expect, actual, length) == 0, expect, actual, "%s")
#define EXPECT_EQ_SIZE_T(expect, actual) \
    EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")

static void test_parse_null() {
    ason_value v;
    ason_init(&v);
    ason_set_boolean(&v, 0);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "null"));
    EXPECT_EQ_INT(ASON_NULL, ason_get_type(&v));
    ason_free(&v);
}

static void test_parse_false() {
    ason_value v;
    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "false"));
    EXPECT_EQ_INT(ASON_FALSE, ason_get_type(&v));
    ason_free(&v);
}

static void test_parse_true() {
    ason_value v;
    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "true"));
    EXPECT_EQ_INT(ASON_TRUE, ason_get_type(&v));
    ason_free(&v);
}

#define TEST_NUMBER(expect, json) \
    do { \
        ason_value v; \
        ason_init(&v); \
        EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, json)); \
        EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(&v)); \
        EXPECT_EQ_DOUBLE(expect, ason_get_number(&v)); \
        ason_free(&v); \
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(0E10, "0E10");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json) \
    do { \
        ason_value v; \
        ason_init(&v); \
        EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, json)); \
        EXPECT_EQ_STRING(expect, ason_get_string(&v), ason_get_string_length(&v)); \
        ason_free(&v); \
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("hello world", "\"hello world\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
    size_t i, j;
    ason_value v;

    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "[ ]"));
    EXPECT_EQ_INT(ASON_ARRAY, ason_get_type(&v));
    EXPECT_EQ_SIZE_T(0, ason_get_array_size(&v));
    ason_free(&v);

    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(ASON_ARRAY, ason_get_type(&v));
    EXPECT_EQ_SIZE_T(5, ason_get_array_size(&v));
    const ason_value* arr = ason_get_array_element(&v, 0);
    EXPECT_EQ_INT(ASON_NULL, ason_get_type(&arr[0]));
    EXPECT_EQ_INT(ASON_FALSE, ason_get_type(&arr[1]));
    EXPECT_EQ_INT(ASON_TRUE, ason_get_type(&arr[2]));
    EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(&arr[3]));
    EXPECT_EQ_DOUBLE(123.0, ason_get_number(&arr[3]));
    EXPECT_EQ_INT(ASON_STRING, ason_get_type(&arr[4]));
    EXPECT_EQ_STRING("abc", ason_get_string(&arr[4]), ason_get_string_length(&arr[4]));
    ason_free(&v);

    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(ASON_ARRAY, ason_get_type(&v));
    EXPECT_EQ_SIZE_T(4, ason_get_array_size(&v));
    for (i = 0; i < 4; i++) {
        ason_value* a = ason_get_array_element(&v, i);
        EXPECT_EQ_INT(ASON_ARRAY, ason_get_type(a));
        EXPECT_EQ_SIZE_T(i, ason_get_array_size(a));
        for (j = 0; j < i; j++) {
            ason_value* e = ason_get_array_element(a, j);
            EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, ason_get_number(e));
        }
    }
    ason_free(&v);
}

static void test_parse_object() {
    ason_value v;
    size_t i;

    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v, " { } "));
    EXPECT_EQ_INT(ASON_OBJECT, ason_get_type(&v));
    EXPECT_EQ_SIZE_T(0, ason_get_object_entry_size(&v));
    ason_free(&v);

    ason_init(&v);
    EXPECT_EQ_INT(ASON_PARSE_OK, ason_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(ASON_OBJECT, ason_get_type(&v));
    EXPECT_EQ_SIZE_T(7, ason_get_object_entry_size(&v));
    EXPECT_EQ_STRING("n", ason_get_object_key(&v, 0), ason_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(ASON_NULL,   ason_get_type(ason_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", ason_get_object_key(&v, 1), ason_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(ASON_FALSE,  ason_get_type(ason_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", ason_get_object_key(&v, 2), ason_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(ASON_TRUE,   ason_get_type(ason_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", ason_get_object_key(&v, 3), ason_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(ason_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, ason_get_number(ason_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", ason_get_object_key(&v, 4), ason_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(ASON_STRING, ason_get_type(ason_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", ason_get_string(ason_get_object_value(&v, 4)), ason_get_string_length(ason_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", ason_get_object_key(&v, 5), ason_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(ASON_ARRAY, ason_get_type(ason_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, ason_get_array_size(ason_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        ason_value* m = ason_get_array_element(ason_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(m));
        EXPECT_EQ_DOUBLE(i + 1.0, ason_get_number(m));
    }
    EXPECT_EQ_STRING("o", ason_get_object_key(&v, 6), ason_get_object_key_length(&v, 6));
    {
        ason_value* o = ason_get_object_value(&v, 6);
        EXPECT_EQ_INT(ASON_OBJECT, ason_get_type(o));
        EXPECT_EQ_SIZE_T(3, ason_get_object_entry_size(o));
        for (i = 0; i < 3; i++) {
            ason_value* ov = ason_get_object_value(o, i);
            EXPECT_TRUE('1' + i == ason_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, ason_get_object_key_length(o, i));
            EXPECT_EQ_INT(ASON_NUMBER, ason_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, ason_get_number(ov));
        }
    }
    ason_free(&v);
}

#define TEST_ERROR(error, json) \
    do { \
        ason_value v; \
        ason_init(&v); \
        ason_set_boolean(&v, 0); \
        EXPECT_EQ_INT(error, ason_parse(&v, json)); \
        EXPECT_EQ_INT(ASON_NULL, ason_get_type(&v)); \
        ason_free(&v); \
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(ASON_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(ASON_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, " ? ");
    /* invalid number */
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, ".1");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "1.");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "nan");
    /* invalid array */
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "[\"a\", nul]");
    /* invalid object */
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "{\"a\":}");
    TEST_ERROR(ASON_PARSE_INVALID_VALUE, "{\"a\": tru}");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(ASON_PARSE_ROOT_NOT_SINGULAR, "null true");
    /* invalid number */
    TEST_ERROR(ASON_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(ASON_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(ASON_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(ASON_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(ASON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_miss_quotation_mark() {
    TEST_ERROR(ASON_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(ASON_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(ASON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(ASON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(ASON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(ASON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(ASON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(ASON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDC01\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(ASON_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(ASON_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(ASON_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(ASON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_access_null() {
    ason_value v;
    ason_init(&v);
    ason_set_string(&v, "a", 1);
    ason_set_null(&v);
    EXPECT_EQ_INT(ASON_NULL, ason_get_type(&v));
    ason_free(&v);
}

static void test_access_boolean() {
    ason_value v;
    ason_init(&v);
    ason_set_string(&v, "a", 1);
    ason_set_boolean(&v, 1);
    EXPECT_TRUE(ason_get_boolean(&v));
    ason_set_boolean(&v, 0);
    EXPECT_FALSE(ason_get_boolean(&v));
    ason_free(&v);
}

static void test_access_number() {
    ason_value v;
    ason_init(&v);
    ason_set_string(&v, "a", 1);
    ason_set_number(&v, 3.14159);
    EXPECT_EQ_DOUBLE(3.14159, ason_get_number(&v));
    ason_free(&v);
}

static void test_access_string() {
    ason_value v;
    ason_init(&v);
    ason_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", ason_get_string(&v), ason_get_string_length(&v));
    ason_set_string(&v, "hello", 5);
    EXPECT_EQ_STRING("hello", ason_get_string(&v), ason_get_string_length(&v));
    ason_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_false();
    test_parse_true();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
