#ifndef CJSON_H__
#define CJSON_H__

typedef enum {
    JSON_NULL,
    JSON_FALSE,
    JSON_TRUE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct {
    union {
        struct {
            char* s;
            size_t len;
        } str;
        double num;
    } u;
    json_type type;
} json_value;

enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR
};

#define json_init(v) do {(v)->type = JSON_NULL;} while(0)

int json_parse(json_value* v, const char* json);

void json_free(json_value* v);

json_type json_get_type(const json_value* v);

#define json_set_null(v) json_free(v)

int json_get_boolean(const json_value* v);
void json_set_boolean(json_value* v, int b);

double json_get_number(const json_value* v);
void json_set_number(json_value* v, double n);

const char* json_get_string(const json_value* v);
size_t json_get_string_length(const json_value* v);
void json_set_string(json_value* v, const char* s, size_t len);

#endif
