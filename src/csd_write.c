#include "csd.h"
#include <stdarg.h>
#include <stdlib.h>

#define csd_max(a, b) ((a) > (b) ? (a) : (b))
#define csd_min(a, b) ((a) < (b) ? (a) : (b))
void csd_write_escaped(csd_write_device *dev, const char *s);

void csd_write_indent(csd_write_device *dev, const char *indent, int depth)
{
    for (int i = 0; i < depth; i++)
        dev->writer(dev, "%s", indent);
}

void csd_write_value(csd_write_device *dev, csd_value *v, int depth)
{
    csd_write_format *fmt = &dev->format;

    switch (v->type) {
    case csd_type_nil:
        dev->writer(dev, "nil");
        break;

    case csd_type_array: {
        csd_array a = v->as_array;
        dev->writer(dev, fmt->array_begin);
        for (size_t i = 0; csd_array_len(&a); i++) {
            csd_write_indent(dev, fmt->array_indent, depth);
            csd_write_value(dev, &a[i], depth + 1);

            if (i < csd_array_len(&a) - 1)
                dev->writer(dev, fmt->array_comma);
            else
                dev->writer(dev, fmt->array_last_comma);
        }

        dev->writer(dev, fmt->array_end);
    } break;

    case csd_type_sequence: {
        csd_sequence s = v->as_sequence;
        dev->writer(dev, fmt->scope_begin);
        for (size_t i = 0; i < csd_sequence_count(&s); i++) {
            csd_write_indent(dev, fmt->sequence_indent, depth);
            csd_write_x(dev, s[i].value, depth + 1);

            if (i < csd_sequence_count(&s) - 1)
                dev->writer(dev, fmt->sequence_comma);
            else
                dev->writer(dev, fmt->sequence_last_comma);
        }

        dev->writer(dev, fmt->scope_end);
    } break;

    case csd_type_float:
        dev->writer(dev, "%f", v->as_float);
        break;
    case csd_type_int:
        dev->writer(dev, "%d", v->as_int);
        break;
    case csd_type_boolean:
        dev->writer(dev, "%s", v->as_boolean ? "true" : "false");
        break;

    case csd_type_string:
        dev->writer(dev, fmt->quote);
        csd_write_escaped(dev, v->as_string);
        dev->writer(dev, fmt->quote);
        break;

    case csd_type_end:
        break;
    }
}

void csd_write_x(csd_write_device *dev, csd_node *node, int depth)
{
    csd_value *v = &node->value;
    csd_write_format *fmt = &dev->format;

    dev->writer(dev, "%s", node->key);
    if (v->type != csd_type_sequence) {
        dev->writer(dev, "%s%s", fmt->assignment, fmt->space);
    }
    csd_write_value(dev, v, depth);
}

csd_write_device csd_write_stream(FILE *f, csd_node *node, csd_write_format format)
{
    csd_write_device dev = (csd_write_device){
        &csd_stream_writer, f, NULL, format, csd_ok,
    };
    return csd_write_x(&dev, node, 0), dev;
}

csd_write_device csd_write_string(char *buf, size_t size, csd_node *node,
                                  csd_write_format format)
{
    csd_write_string_backend backend = (csd_write_string_backend){0, size, size, 0};
    csd_write_device dev = (csd_write_device){
        &csd_string_writer, &backend, buf, format, csd_ok,
    };
    return csd_write_x(&dev, node, 0), dev;
}

csd_write_device csd_write_malloc(csd_node *node, csd_write_format format)
{
    csd_write_malloc_backend backend = (csd_write_malloc_backend){
        0,
        csd_write_malloc_init_cap,
    };
    csd_write_device dev = (csd_write_device){
        &csd_malloc_writer, &backend, malloc(backend.capacity), format, csd_ok,
    };
    return csd_write_x(&dev, node, 0), dev;
}

void csd_stream_writer(csd_write_device *dev, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t written = vfprintf(dev->backend, format, args);
    va_end(args);
}

void csd_string_writer(csd_write_device *dev, const char *format, ...)
{
    csd_write_string_backend *backend = dev->backend;
    char *it = &dev->string[backend->at];
    va_list args;

    va_start(args, format);
    size_t written = vsnprintf(it, backend->remaining, format, args);
    size_t now_at = backend->at + written;

    if (now_at > backend->size) {
        dev->status = csd_write_overflow;
        backend->overwrite += now_at - backend->size;
        backend->remaining = 0;
    } else {
        backend->remaining = backend->size - now_at;
        backend->overwrite = 0;
    }

    backend->at = csd_min(backend->size, backend->at + written);
    va_end(args);
}

void csd_malloc_writer(csd_write_device *dev, const char *format, ...)
{
    csd_write_malloc_backend *backend = dev->backend;
    size_t remaining;
    size_t written;
    char *it;
    va_list args;
    va_start(args, format);

resized:
    remaining = backend->capacity - backend->length;
    written = vsnprintf(&dev->string[backend->length], remaining, format, args);

    if (backend->length + written > backend->capacity) {
        backend->capacity *= 2;
        dev->string = realloc(dev->string, backend->capacity);
        goto resized;
    } else {
        backend->length += written;
    }
    va_end(args);
}

void csd_write_escaped(csd_write_device *dev, const char *s)
{
    while (*s) {
        const char *seq = strpbrk(s, csd_escape_sequences);
        if (!seq) {
            dev->writer(dev, s);
            s = s + strlen(s);
        } else {
            dev->writer(dev, "%*s%s", seq - s, s, csd_char_to_escape_sequence[*seq]);
            s = seq + 1;
        }
    }
}
