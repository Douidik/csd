#include "csd.h"
#include "stb_ds.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#endif

csd_token_mask csd_value_mask = csd_token_string | csd_token_float | csd_token_int |
                                csd_token_true | csd_token_false | csd_token_array_begin;

csd_token_mask csd_item_mask = csd_token_string | csd_token_float | csd_token_int |
                               csd_token_true | csd_token_false | csd_token_scope_begin |
                               csd_token_array_begin;

csd_node *csd_parse_node(csd_document *doc);
csd_sequence csd_parse_sequence(csd_document *doc);
csd_value csd_parse_value(csd_document *doc, csd_token_mask mask);
const char *csd_token_typename(csd_token_type type);
char *csd_get_filename(char *s, FILE *f);

csd_document csd_parse_stream(FILE *f)
{
    char filename[255];
    csd_document doc = {0};
    csd_get_filename(filename, f);
    doc.error = setjmp(doc._throw_env);

    if (doc.error != csd_ok) {
        csd_free(&doc);
        return doc;
    }
    if (ferror(f)) {
        csd_file_throw(&doc, filename, "%s", strerror(errno));
    }

    int64_t at = ftell(f);
    fseek(f, 0, SEEK_END);
    int64_t fsize = ftell(f);
    fseek(f, at, SEEK_SET);

    char *source = malloc(fsize + 1);
    if (fread(source, 1, fsize, f) != fsize) {
        csd_file_throw(&doc, filename, "%s", strerror(errno));
    }

    doc.head = csd_parse_node(&doc);
    return doc;
}

csd_document csd_parse(char *source)
{
    csd_document doc = {0};
    doc.source = source;
    doc._stream = source;
    doc.error = setjmp(doc._throw_env);

    if (doc.error != csd_ok) {
        csd_free(&doc);
        return doc;
    }
    doc.head = csd_parse_node(&doc);
    return doc;
}

csd_token csd_scan_token(csd_document *doc);

csd_token csd_queue_token(csd_document *doc, csd_token token)
{
    doc->_has_queued_token = true;
    doc->_queued_token = token;
    return token;
}

csd_token csd_scan(csd_document *doc, csd_token_mask mask)
{
    csd_token token;

    if (doc->_has_queued_token) {
        doc->_has_queued_token = false;
        token = doc->_queued_token;
    } else {
        do {
            token = csd_scan_token(doc);
        } while (token.type & (csd_token_comment));
    }

    token.ok = token.type & mask;
    return token;
}

csd_token csd_expect(csd_document *doc, csd_token_mask mask)
{
    csd_token token = csd_scan(doc, mask);
    if (!token.ok) {
        const char **types = NULL;
        for (int i = 0; csd_bit(i) != csd_token_type_end; i++) {
            if (csd_bit(i) & mask)
                arrpush(types, csd_token_typename(csd_bit(i)));
        }

        char expected[512] = {0};
        for (int i = 0; i < arrlen(types); i++) {
            sprintf(expected, "%s'%s'", expected, types[i]);
            if (i < arrlen(types) - 1)
                strcat(expected, ", ");
        }

        arrfree(types);
        csd_parse_throw(doc, token, "expected: %s", expected);
    }
    return token;
}

csd_token csd_peek(csd_document *doc, csd_token_mask mask)
{
    return csd_queue_token(doc, csd_scan(doc, mask));
}

csd_token csd_read(csd_document *doc, csd_token_mask mask)
{
    csd_token token = csd_scan(doc, mask);
    if (!token.ok)
        csd_queue_token(doc, token);
    return token;
}

csd_node *csd_parse_node(csd_document *doc)
{
    csd_node *node;
    csd_token token1;
    csd_token token2;
    const char *key;

    token1 = csd_expect(doc, csd_token_id | csd_token_scope_begin | csd_token_eof);

    switch (token1.type) {
    case csd_token_scope_begin:
        key = "";
        token2 = token1;
        break;
    case csd_token_id:
        key = token1.expr;
        token2 = csd_expect(doc, csd_token_assign | csd_token_scope_begin);
        break;
    case csd_token_eof:
        return NULL;
    default:
        assert(!"unreachable");
    }

    switch (token2.type) {
    case csd_token_scope_begin:
        node = csd_new_sequence(doc, key, NULL);
        node->value.as_sequence = csd_parse_sequence(doc);
        break;
    case csd_token_assign:
        node = csd_new_nil(doc, key);
        node->value = csd_parse_value(doc, csd_value_mask);
        break;
    default:
        assert(!"unreachable");
    }

    return node;
}

csd_sequence csd_parse_sequence(csd_document *doc)
{
    csd_sequence sequence = NULL;

    while (1) {
        if (csd_read(doc, csd_token_scope_end).ok)
            break;
        csd_sequence_push(&sequence, csd_parse_node(doc));
        if (csd_expect(doc, csd_token_scope_end | csd_token_comma).type &
            csd_token_scope_end)
            break;
    }
    return sequence;
}

csd_array csd_parse_array(csd_document *doc)
{
    csd_array array = NULL;

    while (1) {
        if (csd_read(doc, csd_token_array_end).ok)
            break;
        csd_array_push(&array, csd_parse_value(doc, csd_item_mask));
        if (csd_expect(doc, csd_token_array_end | csd_token_comma).type &
            csd_token_array_end)
            break;
    }
    return array;
}

csd_value csd_parse_value(csd_document *doc, csd_token_mask mask)
{
    csd_token vtoken = csd_expect(doc, mask);

    switch (vtoken.type) {
    case csd_token_string:
        return csd_vstring(vtoken.expr);

    case csd_token_float: {
        double v = strtod(vtoken.expr, NULL);
        if (errno != 0)
            csd_parse_throw(doc, vtoken, "cannot parse float: %s", strerror(errno));
        return csd_vfloat(v);
    }

    case csd_token_int: {
        int64_t v = strtol(vtoken.expr, NULL, 0);
        if (errno != 0)
            csd_parse_throw(doc, vtoken, "cannot parse int: %s", strerror(errno));
        return csd_vint(v);
    }

    case csd_token_true:
        return csd_vboolean(true);
    case csd_token_false:
        return csd_vboolean(false);

    case csd_token_scope_begin:
        return csd_vsequence(csd_parse_sequence(doc));
    case csd_token_array_begin:
        return csd_varray(csd_parse_array(doc));

    default:
        assert(!"unreachable");
    }
}

char *csd_get_filename(char *s, FILE *f)
{
    if (!f)
        goto error;
#ifdef __linux__
    int fd = fileno(f);
    char fd_path[255];

    sprintf(fd_path, "/proc/self/fd/%d", fd);
    int size = readlink(fd_path, s, 255);
    if (size < 0)
        goto error;
    s[size] = '\0';
    char *filename = strrchr(s, '/');
    if (filename != NULL) {
        strcpy(s, filename + 1);
    }
    return s;
#endif

error:
    sprintf(s, "nofilename");
    return s;
}

const char *csd_token_typename(csd_token_type type)
{
    switch (type) {
    case csd_token_none:
        return "none";
    case csd_token_id:
        return "identifier";
    case csd_token_assign:
        return ":";
    case csd_token_comma:
        return ",";
    case csd_token_scope_begin:
        return "{";
    case csd_token_scope_end:
        return "}";
    case csd_token_array_begin:
        return "[";
    case csd_token_array_end:
        return "]";
    case csd_token_string:
        return "string";
    case csd_token_float:
        return "float";
    case csd_token_int:
        return "int";
    case csd_token_true:
        return "true";
    case csd_token_false:
        return "false";
    case csd_token_comment:
        return "comment";
    case csd_token_eof:
        return "eof";
    default:
        assert(!"unreachable");
    }
}
