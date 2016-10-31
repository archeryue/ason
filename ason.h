#ifndef ASON_H__
#define ASON_H__

typedef enum {
    ASON_NULL,
    ASON_FALSE,
    ASON_TRUE,
    ASON_NUMBER,
    ASON_STRING,
    ASON_ARRAY,
    ASON_OBJECT
} ason_type;

typedef struct ason_value ason_value;
typedef struct ason_entry ason_entry;

typedef struct {
    double d;
} ason_number;

typedef struct {
    char* s;
    size_t len;
} ason_string;

typedef struct {
    ason_value* m;
    size_t size;
} ason_array;

typedef struct {
    ason_entry* e;
    size_t size;
} ason_object;

struct ason_value {
    union {
        ason_number num;
        ason_string str;
        ason_array  arr;
        ason_object obj;
    } u;
    ason_type type;
};

struct ason_entry {
    ason_string k;
    ason_value v;
};

enum {
    ASON_PARSE_OK = 0,
    ASON_PARSE_EXPECT_VALUE,
    ASON_PARSE_INVALID_VALUE,
    ASON_PARSE_ROOT_NOT_SINGULAR,
    ASON_PARSE_NUMBER_TOO_BIG,
    ASON_PARSE_MISS_QUOTATION_MARK,
    ASON_PARSE_INVALID_STRING_ESCAPE,
    ASON_PARSE_INVALID_STRING_CHAR,
    ASON_PARSE_INVALID_UNICODE_HEX,
    ASON_PARSE_INVALID_UNICODE_SURROGATE,
    ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    ASON_PARSE_MISS_KEY,
    ASON_PARSE_MISS_COLON,
    ASON_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#define ason_init(v) do {(v)->type = ASON_NULL;} while(0)

int ason_parse(ason_value* v, const char* json);

void ason_free(ason_value* v);

ason_type ason_get_type(const ason_value* v);

#define ason_set_null(v) ason_free(v)

int ason_get_boolean(const ason_value* v);
void ason_set_boolean(ason_value* v, int b);

double ason_get_number(const ason_value* v);
void ason_set_number(ason_value* v, double n);

const char* ason_get_string(const ason_value* v);
size_t ason_get_string_length(const ason_value* v);
void ason_new_string(ason_string* str, const char* s, size_t len);
void ason_set_string(ason_value* v, const char* s, size_t len);

size_t ason_get_array_size(const ason_value* v);
ason_value* ason_get_array_element(const ason_value* v, size_t index);

size_t ason_get_object_entry_size(const ason_value* v);
const char* ason_get_object_key(const ason_value* v, size_t index);
size_t ason_get_object_key_length(const ason_value* v, size_t index);
ason_value* ason_get_object_value(const ason_value* v, size_t index);

#endif
