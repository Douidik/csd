/*****************************************/
/*                  __       __          */
/*                 /\ \     /\ \         */
/*   ___    ____   \_\ \    \ \ \___     */
/*  /'___\ /',__\  /'_` \    \ \  _ `\   */
/* /\ \__//\__, `\/\ \L\ \  __\ \ \ \ \  */
/* \ \____\/\____/\ \___,_\/\_\\ \_\ \_\ */
/*  \/____/\/___/  \/__,_ /\/_/ \/_/\/_/ */
/*                                       */
/*****************************************/

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef CSD_H
#define CSD_H

#if defined(__GNUC__) || defined(__clang__)
#define csd_noreturn __attribute__((noreturn))
#else
#define csd_noreturn
#endif

#if defined(__GNUC__) || defined(__clang__)
#define csd_printf_like(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define csd_printf_like(fmt, args)
#endif

#define csd_write_malloc_init_cap 512
#define csd_reason_size 512
#define csd_array_sizeof(x) (sizeof(x) / sizeof(x[0]))

static const char *csd_escape_sequences = "\a\b\e\f\n\r\t\v\?\\\"";
static const char *csd_escape_sequence_to_char[255] = {
    ['\a'] = "\\a", ['\b'] = "\\b", ['\e'] = "\\e",  ['\f'] = "\\f",
    ['\n'] = "\\n", ['\r'] = "\\r", ['\t'] = "\\t",  ['\v'] = "\\v",
    ['\?'] = "\\?", ['\\'] = "\\",  ['\"'] = "\\\"",
};
static char csd_char_to_escape_sequence[255] = {
    ['a'] = '\a', ['b'] = '\b', ['e'] = '\e', ['f'] = '\f',  ['n'] = '\n',  ['r'] = '\r',
    ['t'] = '\t', ['v'] = '\v', ['?'] = '\?', ['\\'] = '\\', ['\''] = '\'',
};
static bool csd_is_char_escape_sequence[255] = {
    ['a'] = true, ['b'] = true, ['e'] = true, ['f'] = true,  ['n'] = true,  ['r'] = true,
    ['t'] = true, ['v'] = true, ['?'] = true, ['\\'] = true, ['\"'] = true,
};

typedef enum csd_error
{
    csd_ok = 0,
    csd_file_error,
    csd_scan_error,
    csd_write_overflow,
} csd_error;

typedef enum csd_type
{
    csd_type_nil,
    csd_type_array,
    csd_type_sequence,
    csd_type_float,
    csd_type_int,
    csd_type_boolean,
    csd_type_string,
    csd_type_end,
} csd_type;

#define csd_bit(n) (1 << n)
typedef enum csd_token_type
{
    csd_token_none = csd_bit(0),
    csd_token_id = csd_bit(1),
    csd_token_assign = csd_bit(2),
    csd_token_comma = csd_bit(3),
    csd_token_scope_begin = csd_bit(4),
    csd_token_scope_end = csd_bit(5),
    csd_token_array_begin = csd_bit(6),
    csd_token_array_end = csd_bit(7),
    csd_token_string = csd_bit(8),
    csd_token_float = csd_bit(9),
    csd_token_int = csd_bit(10),
    csd_token_int_hex = csd_bit(11),
    csd_token_int_binary = csd_bit(12),
    csd_token_true = csd_bit(13),
    csd_token_false = csd_bit(14),
    csd_token_comment = csd_bit(15),
    csd_token_eof = csd_bit(16),
    csd_token_type_end = csd_bit(17),
} csd_token_type;

typedef uint32_t csd_token_mask;

typedef struct csd_token
{
    csd_token_type type;
    char *expr;
    size_t size;
    int line;
    int column;
    bool ok;
} csd_token;

typedef struct csd_nil
{
} csd_nil;

typedef struct csd_value *csd_array;
typedef struct csd_node *csd_node_array;
typedef struct csd_bucket
{
    const char *key;
    struct csd_node *value;
} csd_bucket;
typedef csd_bucket *csd_sequence;

typedef struct csd_value
{
    csd_type type;
    union {
        csd_nil as_nil;
        csd_array as_array;
        csd_sequence as_sequence;
        double as_float;
        int64_t as_int;
        bool as_boolean;
        const char *as_string;
    };
} csd_value;

#define csd_vnil ((csd_value){.type = csd_type_nil, .as_nil = (csd_nil){}})
#define csd_varray(v) ((csd_value){.type = csd_type_array, .as_array = v})
#define csd_vsequence(v) ((csd_value){.type = csd_type_sequence, .as_sequence = v})
#define csd_vfloat(v) ((csd_value){.type = csd_type_float, .as_float = v})
#define csd_vint(v) ((csd_value){.type = csd_type_int, .as_int = v})
#define csd_vboolean(v) ((csd_value){.type = csd_type_boolean, .as_boolean = v})
#define csd_vstring(v) ((csd_value){.type = csd_type_string, .as_string = v})
#define csd_vend() ((csd_value){.type = csd_type_end})

typedef struct csd_node
{
    const char *key;
    csd_value value;
} csd_node;

typedef struct csd_document
{
    char *source;
    csd_node *head;
    csd_node_array nodes;

    char *_strbuf;
    char *_stream;
    int _line;
    int _column;

    csd_token _queued_token;
    bool _has_queued_token;

    csd_error error;
    char reason[csd_reason_size];
    jmp_buf _throw_env;
} csd_document;

typedef struct csd_write_format
{
    const char *sequence_indent;
    const char *array_indent;
    const char *assignment;
    const char *space;
    const char *sequence_comma;
    const char *sequence_last_comma;
    const char *array_comma;
    const char *array_last_comma;
    const char *quote;
    const char *scope_begin;
    const char *scope_end;
    const char *array_begin;
    const char *array_end;
} csd_write_format;

static const csd_write_format csd_format_standard = (csd_write_format){
    .sequence_indent = "\t",
    .array_indent = "",
    .assignment = ":",
    .space = " ",
    .sequence_comma = ",\n",
    .sequence_last_comma = ",\n",
    .array_comma = ", ",
    .array_last_comma = "",
    .quote = "\"",
    .scope_begin = "{",
    .scope_end = "}",
    .array_begin = "[",
    .array_end = "]",
};

typedef struct csd_write_device csd_write_device;
typedef void (*csd_writer)(csd_write_device *dev, const char *format, ...)
    csd_printf_like(2, 3);

struct csd_write_device
{
    csd_writer writer;
    void *backend;
    char *string;
    csd_write_format format;
    csd_error status;
};

typedef struct csd_write_string_backend
{
    size_t at;
    size_t size;
    size_t remaining;
    size_t overwrite;
} csd_write_string_backend;

typedef struct csd_write_malloc_backend
{
    size_t length;
    size_t capacity;
} csd_write_malloc_backend;

csd_node *csd_new_nil(csd_document *doc, const char *name);
csd_node *csd_new_array(csd_document *doc, const char *name, csd_array v);
csd_node *csd_new_sequence(csd_document *doc, const char *name, csd_sequence v);
csd_node *csd_new_float(csd_document *doc, const char *name, double v);
csd_node *csd_new_int(csd_document *doc, const char *name, int64_t v);
csd_node *csd_new_boolean(csd_document *doc, const char *name, bool v);
csd_node *csd_new_string(csd_document *doc, const char *name, const char *v);

csd_node *csd_sequence_push(csd_sequence *sequence, csd_node *n);
void csd_sequence_remove(csd_sequence *sequence, const char *key);
csd_node *csd_sequence_get(csd_sequence *sequence, const char *key);
size_t csd_sequence_count(csd_sequence *sequence);

#define csd_insert(node, n) csd_sequence_push(&(node)->value.as_sequence, n)
#define csd_remove(node, key) csd_sequence_remove(&(node)->value.as_sequence, key)
#define csd_at(node, key) csd_sequence_get(&(node)->value.as_sequence, key)
#define csd_count(node) csd_sequence_count(&(node)->value.as_sequence)

static const csd_node _csd_end_sentinel = (csd_node){
    .key = "_csd_end_sentinel",
    .value = {csd_type_end},
};

csd_node *csd_make_sequence_x(csd_document *doc, const char *name, ...);
#define csd_make_sequence(doc, name, ...) \
    csd_make_sequence_x(doc, name, __VA_ARGS__ __VA_OPT__(, ) & _csd_end_sentinel)

typedef void (*csd_neq_cb)(csd_node *a, csd_node *b, void *);

bool csd_eq_x(csd_node *a, csd_node *b, csd_neq_cb neq_cb, void *data);
bool csd_eq(csd_node *a, csd_node *b);

csd_value *csd_array_push(csd_array *array, csd_value v);
size_t csd_array_len(csd_array *array);

#define csd_push(node, v) csd_array_push(&(node)->value.as_array, v)
#define csd_len(node) csd_array_len(&(node)->value.as_array)

void csd_free(csd_document *doc);
void csd_free_node(csd_node *node);
void csd_free_value(csd_value *value);

csd_document csd_parse(char *source);
csd_document csd_parse_stream(FILE *f);

void csd_write_x(csd_write_device *dev, csd_node *node, int depth);
csd_write_device csd_write_stream(FILE *f, csd_node *node, csd_write_format format);
csd_write_device csd_write_string(char *buf, size_t size, csd_node *node,
                                  csd_write_format format);
csd_write_device csd_write_malloc(csd_node *node, csd_write_format format);

void csd_stream_writer(csd_write_device *dev, const char *format, ...)
    csd_printf_like(2, 3);
void csd_string_writer(csd_write_device *dev, const char *format, ...)
    csd_printf_like(2, 3);
void csd_malloc_writer(csd_write_device *dev, const char *format, ...)
    csd_printf_like(2, 3);

void csd_throw(csd_document *doc) csd_noreturn;
void csd_scan_throw(csd_document *doc, csd_token token, const char *format,
                    ...) csd_noreturn;
void csd_parse_throw(csd_document *doc, csd_token token, const char *format,
                     ...) csd_noreturn;
void csd_file_throw(csd_document *doc, const char *filepath, const char *format,
                    ...) csd_noreturn;

#endif
