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
    double n;
    json_type type;
} json_value;

enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG
};

int json_parse(json_value* v, const char* json);

json_type json_get_type(const json_value* v);

double json_get_number(const json_value* v);

#endif
