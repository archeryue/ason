#include <stdlib.h>
#include <assert.h>
#include "cjson.h"

#define EXCEPT(c, ch) do { assert(*c->json == ch); c->json++; } while (0)

typedef struct {
    const char* json;
} json_context;



static void json_parse_whitespace(json_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int json_parse_null(json_context* c, json_value* v) {
    EXCEPT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = JSON_NULL;
    return PARSE_OK;
}

static int json_parse_value(json_context* c, json_value* v) {
    switch(*c->json) {
        case 'n':  return json_parse_null(c, v);
        case '\0': return PARSE_EXCEPT_VALUE;
        default:   return PARSE_INVALID_VALUE;
    }
}

int json_parse(json_value* v, const char* json) {
    json_context c;
    assert(v != NULL);
    c.json = json;
    v->type = JSON_NULL;
    json_parse_whitespace(&c);
    return json_parse_value(&c, v);
}

json_type json_get_type(const json_value* v) {
    assert(v != NULL);
    return v->type;
}
