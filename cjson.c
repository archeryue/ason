#include <stdlib.h> /* strtod, NULL */
#include <assert.h> /* assert */
#include <errno.h>  /* errno, ERANGE */
#include <math.h>   /* HUGE_VAL */
#include "cjson.h"

#define EXPECT(c, ch) do { assert(*c->json == ch); c->json++; } while (0)

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
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type = JSON_NULL;
    return PARSE_OK;
}

static int json_parse_false(json_context* c, json_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type = JSON_FALSE;
    return PARSE_OK;
}

static int json_parse_true(json_context* c, json_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type = JSON_TRUE;
    return PARSE_OK;
}

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

static int json_parse_number(json_context* c, json_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) {
        return PARSE_NUMBER_TOO_BIG;
    }
    c->json = p;
    v->type = JSON_NUMBER;
    return PARSE_OK;
}

static int json_parse_value(json_context* c, json_value* v) {
    switch(*c->json) {
        case 'n':  return json_parse_null(c, v);
        case 'f':  return json_parse_false(c, v);
        case 't':  return json_parse_true(c, v);
        case '\0': return PARSE_EXPECT_VALUE;
        default:   return json_parse_number(c, v);
    }
}

int json_parse(json_value* v, const char* json) {
    json_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = JSON_NULL;
    json_parse_whitespace(&c);
    if ((ret = json_parse_value(&c, v)) == PARSE_OK) {
        json_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = JSON_NULL;
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

json_type json_get_type(const json_value* v) {
    assert(v != NULL);
    return v->type;
}

double json_get_number(const json_value* v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->n;
}
