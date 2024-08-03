#include "csd.h"
#include <stdarg.h>
#include <string.h>

void csd_vprintf(char *s, size_t size, const char *format, va_list args)
{
    size_t remain = strlen(s) - size;
    vsnprintf(s + strlen(s), remain, format, args);
}

void csd_printf(char *s, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    csd_vprintf(s, size, format, args);
    va_end(args);
}

void csd_source_error(csd_document *doc, csd_token token, const char *format,
                      va_list args)
{
    csd_printf(doc->reason, csd_reason_size, "(%d:%d) ", token.line, token.column);
    csd_vprintf(doc->reason, csd_reason_size, format, args);
}

csd_noreturn void csd_throw(csd_document *doc)
{
    longjmp(doc->_throw_env, doc->error);
}

csd_noreturn void csd_file_throw(csd_document *doc, const char *filepath,
                                 const char *format, ...)
{
    va_list args;
    va_start(args, format);
    doc->error = csd_file_error;
    csd_printf(doc->reason, csd_reason_size, "file '%s': ", filepath);
    csd_vprintf(doc->reason, csd_reason_size, format, args);
    va_end(args);
    csd_throw(doc);
}

csd_noreturn void csd_scan_throw(csd_document *doc, csd_token token, const char *format,
                                 ...)
{
    va_list args;
    va_start(args, format);
    doc->error = csd_scan_error;
    csd_source_error(doc, token, format, args);
    va_end(args);
    csd_throw(doc);
}

csd_noreturn void csd_parse_throw(csd_document *doc, csd_token token, const char *format,
                                  ...)
{
    va_list args;
    va_start(args, format);
    doc->error = csd_scan_error;
    csd_source_error(doc, token, format, args);
    va_end(args);
    csd_throw(doc);
}
