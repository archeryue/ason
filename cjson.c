#include <stdlib.h> /* malloc, realloc, free, strtod, NULL */
#include <assert.h> /* assert */
#include <errno.h>  /* errno, ERANGE */
#include <math.h>   /* HUGE_VAL */
#include <string.h> /* memcpy */
#include "cjson.h"

#ifndef JSON_PARSE_STACK_INIT_SIZE
#define JSON_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do { assert(*c->json == ch); c->json++; } while (0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} json_context;

static void* json_context_push(json_context* c, size_t size) {
    void* ret;
    assert(c != NULL && size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = JSON_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1; /* size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* json_context_pop(json_context* c, size_t size) {
    assert(c != NULL && size >= 0 && c->top >= size);
    c->top -= size;
    return c->stack + c->top;
}

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
    v->u.num = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.num == HUGE_VAL || v->u.num == -HUGE_VAL)) {
        return PARSE_NUMBER_TOO_BIG;
    }
    c->json = p;
    v->type = JSON_NUMBER;
    return PARSE_OK;
}

#define PUTC(c, ch) do { *(char*)json_context_push(c, sizeof(char)) = (ch);} while(0)
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static const char* json_parse_hex4(const char* p, unsigned* u) {
    int i;
    *u = 0;
    for (i=0; i<4; i++) {
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9') *u |= ch - '0';
        else if (ch >= 'a' && ch <= 'f') *u |= ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') *u |= ch - 'A' + 10;
        else return NULL;
    }
    return p;
}

static void json_encode_utf8(json_context* c, unsigned u) {
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >>  6) & 0xFF));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

static int json_parse_string(json_context* c, json_value* v) {
    size_t head = c->top, len;
    unsigned u,ul;
    EXPECT(c, '"');
    const char *p = c->json;
    while (1) {
        char ch = *p++;
        switch (ch) {
            case '"':
                len = c->top - head;
                json_set_string(v, (char*)json_context_pop(c, len), len);
                c->json = p;
                return PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\\': PUTC(c, '\\'); break;
                    case '"': PUTC(c, '"'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = json_parse_hex4(p, &u)))
                            STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p != '\\' || *(p+1) != 'u')
                                STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                            p+=2;
                            if (!(p = json_parse_hex4(p, &ul)))
                                STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                            if (ul < 0xDC00 || ul > 0xDFFF)
                                STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (ul - 0xDC00)) + 0x10000;
                        }
                        json_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}

static int json_parse_value(json_context* c, json_value* v) {
    switch(*c->json) {
        case 'n':  return json_parse_null(c, v);
        case 'f':  return json_parse_false(c, v);
        case 't':  return json_parse_true(c, v);
        default :  return json_parse_number(c, v);
        case '"':  return json_parse_string(c, v);
        case '\0': return PARSE_EXPECT_VALUE;
    }
}

int json_parse(json_value* v, const char* json) {
    json_context c;
    int ret;
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    assert(v != NULL);
    json_init(v);
    json_parse_whitespace(&c);
    if ((ret = json_parse_value(&c, v)) == PARSE_OK) {
        json_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = JSON_NULL;
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void json_free(json_value* v) {
    assert(v != NULL);
    if (v->type == JSON_STRING)
        free(v->u.str.s);
    v->type = JSON_NULL;
}

json_type json_get_type(const json_value* v) {
    assert(v != NULL);
    return v->type;
}

int json_get_boolean(const json_value* v) {
    assert(v != NULL && (v->type == JSON_TRUE || v->type == JSON_FALSE));
    return v->type == JSON_TRUE ? 1 : 0;
}

void json_set_boolean(json_value* v, int b) {
    assert(v != NULL);
    json_free(v);
    v->type = b ? JSON_TRUE : JSON_FALSE;
}

double json_get_number(const json_value* v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->u.num;
}

void json_set_number(json_value* v, double n) {
    assert(v != NULL);
    json_free(v);
    v->u.num = n;
    v->type = JSON_NUMBER;
}

const char* json_get_string(const json_value* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.str.s;
}

size_t json_get_string_length(const json_value* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.str.len;
}

void json_set_string(json_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    json_free(v);
    v->u.str.s = (char*)malloc(len+1);
    memcpy(v->u.str.s, s, len);
    v->u.str.s[len] = '\0';
    v->u.str.len = len;
    v->type = JSON_STRING;
}
