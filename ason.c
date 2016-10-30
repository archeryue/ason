#include <stdlib.h> /* malloc, realloc, free, strtod, NULL */
#include <assert.h> /* assert */
#include <errno.h>  /* errno, ERANGE */
#include <math.h>   /* HUGE_VAL */
#include <string.h> /* memcpy */
#include "ason.h"

#ifndef ASON_PARSE_STACK_INIT_SIZE
#define ASON_PARSE_STACK_INIT_SIZE 256
#endif

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} ason_context;

static void* ason_context_push(ason_context* c, size_t size) {
    void* ret;
    assert(c != NULL && size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = ASON_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1; /* size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* ason_context_pop(ason_context* c, size_t size) {
    assert(c != NULL && size >= 0 && c->top >= size);
    c->top -= size;
    return c->stack + c->top;
}

#define EXPECT(c, ch) do { assert(*c->json == ch); c->json++; } while (0)

static void ason_parse_whitespace(ason_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int ason_parse_null(ason_context* c, ason_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l') {
        return ASON_PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type = ASON_NULL;
    return ASON_PARSE_OK;
}

static int ason_parse_false(ason_context* c, ason_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e') {
        return ASON_PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type = ASON_FALSE;
    return ASON_PARSE_OK;
}

static int ason_parse_true(ason_context* c, ason_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e') {
        return ASON_PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type = ASON_TRUE;
    return ASON_PARSE_OK;
}

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

static int ason_parse_number(ason_context* c, ason_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return ASON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return ASON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return ASON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.num = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.num == HUGE_VAL || v->u.num == -HUGE_VAL)) {
        return ASON_PARSE_NUMBER_TOO_BIG;
    }
    c->json = p;
    v->type = ASON_NUMBER;
    return ASON_PARSE_OK;
}

#define PUTC(c, ch) do { *(char*)ason_context_push(c, sizeof(char)) = (ch);} while(0)
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static const char* ason_parse_hex4(const char* p, unsigned* u) {
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

static void ason_encode_utf8(ason_context* c, unsigned u) {
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

static int ason_parse_string(ason_context* c, ason_value* v) {
    size_t head = c->top, len;
    unsigned u,ul;
    EXPECT(c, '"');
    const char *p = c->json;
    while (1) {
        char ch = *p++;
        switch (ch) {
            case '"':
                len = c->top - head;
                ason_set_string(v, (char*)ason_context_pop(c, len), len);
                c->json = p;
                return ASON_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\\': PUTC(c, '\\'); break;
                    case '"' : PUTC(c, '"' ); break;
                    case '/' : PUTC(c, '/' ); break;
                    case 'b' : PUTC(c, '\b'); break;
                    case 'f' : PUTC(c, '\f'); break;
                    case 'n' : PUTC(c, '\n'); break;
                    case 'r' : PUTC(c, '\r'); break;
                    case 't' : PUTC(c, '\t'); break;
                    case 'u' :
                        if (!(p = ason_parse_hex4(p, &u)))
                            STRING_ERROR(ASON_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xDC00 && u <= 0xDFFF)
                            STRING_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p != '\\' || *(p+1) != 'u')
                                STRING_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE);
                            p+=2;
                            if (!(p = ason_parse_hex4(p, &ul)))
                                STRING_ERROR(ASON_PARSE_INVALID_UNICODE_HEX);
                            if (ul < 0xDC00 || ul > 0xDFFF)
                                STRING_ERROR(ASON_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (ul - 0xDC00)) + 0x10000;
                        }
                        ason_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(ASON_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(ASON_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(ASON_PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}

static int ason_parse_value(ason_context* c, ason_value* v);

static int ason_parse_array(ason_context* c, ason_value* v) {
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    ason_parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        v->type = ASON_ARRAY;
        v->u.arr.size = size;
        return ASON_PARSE_OK;
    }
    while (1) {
        ason_value e;
        ason_init(&e);
        if ((ret = ason_parse_value(c, &e)) != ASON_PARSE_OK) {
            ason_context_pop(c, size * sizeof(ason_value));
            return ret;
        }
        memcpy(ason_context_push(c, sizeof(ason_value)), &e, sizeof(ason_value));
        size++;
        ason_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            ason_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = ASON_ARRAY;
            v->u.arr.size = size;
            size *= sizeof(ason_value);
            memcpy(v->u.arr.e = (ason_value*)malloc(size), ason_context_pop(c, size), size);
            return ASON_PARSE_OK;
        }
        else {
            ason_context_pop(c, size * sizeof(ason_value));
            return ASON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
        }
    }
}

static int ason_parse_object(ason_context* c, ason_value* v) {
    /* TODO */
    return 0;
}

static int ason_parse_value(ason_context* c, ason_value* v) {
    switch(*c->json) {
        case 'n' : return ason_parse_null(c, v);
        case 'f' : return ason_parse_false(c, v);
        case 't' : return ason_parse_true(c, v);
        default  : return ason_parse_number(c, v);
        case '"' : return ason_parse_string(c, v);
        case '[' : return ason_parse_array(c, v);
        case '{' : return ason_parse_object(c, v);
        case '\0': return ASON_PARSE_EXPECT_VALUE;
    }
}

int ason_parse(ason_value* v, const char* json) {
    ason_context c;
    int ret;
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    assert(v != NULL);
    ason_init(v);
    ason_parse_whitespace(&c);
    if ((ret = ason_parse_value(&c, v)) == ASON_PARSE_OK) {
        ason_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = ASON_NULL;
            ret = ASON_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void ason_free(ason_value* v) {
    size_t i;
    assert(v != NULL);
    switch (v->type) {
        case ASON_STRING:
            free(v->u.str.s);
            break;
        case ASON_ARRAY:
            for (i = 0; i < v->u.arr.size; i++)
                ason_free(&v->u.arr.e[i]);
            if (v->u.arr.size > 0)
                free(v->u.arr.e);
            break;
        default:
            break;
    }
    v->type = ASON_NULL;
}

ason_type ason_get_type(const ason_value* v) {
    assert(v != NULL);
    return v->type;
}

int ason_get_boolean(const ason_value* v) {
    assert(v != NULL && (v->type == ASON_TRUE || v->type == ASON_FALSE));
    return v->type == ASON_TRUE ? 1 : 0;
}

void ason_set_boolean(ason_value* v, int b) {
    assert(v != NULL);
    ason_free(v);
    v->type = b ? ASON_TRUE : ASON_FALSE;
}

double ason_get_number(const ason_value* v) {
    assert(v != NULL && v->type == ASON_NUMBER);
    return v->u.num;
}

void ason_set_number(ason_value* v, double n) {
    assert(v != NULL);
    ason_free(v);
    v->u.num = n;
    v->type = ASON_NUMBER;
}

const char* ason_get_string(const ason_value* v) {
    assert(v != NULL && v->type == ASON_STRING);
    return v->u.str.s;
}

size_t ason_get_string_length(const ason_value* v) {
    assert(v != NULL && v->type == ASON_STRING);
    return v->u.str.len;
}

void ason_set_string(ason_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    ason_free(v);
    v->u.str.s = (char*)malloc(len+1);
    memcpy(v->u.str.s, s, len);
    v->u.str.s[len] = '\0';
    v->u.str.len = len;
    v->type = ASON_STRING;
}

size_t ason_get_array_size(const ason_value* v) {
    assert(v != NULL && v->type == ASON_ARRAY);
    return v->u.arr.size;
}

ason_value* ason_get_array_element(const ason_value* v, size_t index) {
    assert(v != NULL && v->type == ASON_ARRAY);
    assert(index >=0 && index < v->u.arr.size);
    return v->u.arr.e + index;
}
