#include "csd.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

typedef struct csd_keyword
{
    const char *word;
    csd_token_type type;
} csd_keyword;

const csd_keyword csd_keywords[] = {
    {"'", csd_token_string},    {"\"", csd_token_string},
    {",", csd_token_comma},     {"{", csd_token_scope_begin},
    {"}", csd_token_scope_end}, {"[", csd_token_array_begin},
    {"]", csd_token_array_end}, {":", csd_token_assign},
    {"true", csd_token_true},   {"false", csd_token_false},
};
const size_t csd_keyword_count = csd_array_sizeof(csd_keywords);

void csd_eat_char(csd_document *doc)
{
    if (*doc->_stream != '\n')
        doc->_column++;
    else
        doc->_line++, doc->_column = 0;
    doc->_stream++;
}

csd_token csd_eat(csd_document *doc, csd_token_type type, size_t size)
{
    csd_token token = {0};
    token.line = doc->_line;
    token.column = doc->_column;
    token.type = type;
    token.expr = doc->_stream;
    token.size = size;

    for (size_t i = 0; i < size; i++)
        csd_eat_char(doc);
    return token;
}

csd_token csd_eat_dumb(csd_document *doc)
{
    const char *end = doc->_stream;
    while (*end != '\0' && !isspace(*end))
        end++;
    return csd_eat(doc, csd_token_none, end - doc->_stream);
}

csd_token csd_eat_into(csd_document *doc, csd_token_type type, const char *into)
{
    const char *end = strstr(doc->_stream, into);
    if (!end) {
        end = &doc->source[strlen(doc->source)];
    }
    return csd_eat(doc, type, end - doc->_stream);
}

csd_token csd_eat_keyword(csd_document *doc, csd_keyword keyword)
{
    csd_token token;

    switch (keyword.type) {
    case csd_token_string: {
        csd_eat_char(doc);
        token = csd_eat_into(doc, csd_token_string, keyword.word);

        if (!token.expr[token.size])
            csd_scan_throw(doc, token, "unterminated string", keyword.word);

        char *seq = token.expr;
        while ((seq = strchr(seq, '\\')) != NULL) {
            size_t index = seq - token.expr;
            if (index >= token.size)
                break;
            if (!csd_is_char_escape_sequence[seq[1]])
                csd_scan_throw(doc, token, "unknown escape sequence: '\\%c'", seq[1]);
            seq[0] = csd_char_to_escape_sequence[seq[1]];
            memmove(seq + 1, seq + 2, token.size - index - 1);
            token.size--;
        }
        token.expr[token.size] = '\0';
    } break;

    case csd_token_comment:
        token = csd_eat_into(doc, csd_token_comment, "\n");
        break;

    case csd_token_assign:
    case csd_token_comma:
    case csd_token_scope_begin:
    case csd_token_scope_end:
    case csd_token_array_begin:
    case csd_token_array_end:
    case csd_token_true:
    case csd_token_false:
        token = csd_eat(doc, keyword.type, strlen(keyword.word));
        break;

    default:
        assert(!"unreachable");
    }

    return token;
}

csd_token csd_eat_id(csd_document *doc)
{
    csd_token token;
    static char buf[64];

    const char *it = doc->_stream;
    if (isalpha(*it))
        it++;
    while (isalnum(*it))
        it++;
    /* = csd_eat(doc, csd_token_id, it - doc->_stream); */
}

csd_token csd_eat_number(csd_document *doc)
{
    const char *it = doc->_stream;
    csd_token_type type = csd_token_int;

    if (*it == '-' || *it == '+')
        it++;
    if (strncmp(it, "0x", 2) == 0)
        type = csd_token_int_hex, it = &it[2];
    if (strncmp(it, "0b", 2) == 0)
        type = csd_token_int_binary, it = &it[2];

    while (isdigit(*it))
        it++;
    if (*it == '.') {
        if (type & (csd_token_int_hex | csd_token_int_binary)) {
            csd_scan_throw(doc, csd_eat_dumb(doc),
                           "hex/binary float representation is not supported");
        }

        type = csd_token_float;
        while (isdigit(*it))
            it++;
    }
    return csd_eat(doc, type, it - doc->_stream);
}

csd_token csd_scan_token(csd_document *doc)
{
    if (*doc->_stream == '\0')
        return csd_eat(doc, csd_token_eof, 0);
redo:
    while (isspace(*doc->_stream)) {
        csd_eat_char(doc);
    }

    for (int i = 0; i < csd_keyword_count; i++) {
        csd_keyword keyword = csd_keywords[i];

        if (strncmp(keyword.word, doc->_stream, strlen(keyword.word)) == 0) {
            return csd_eat_keyword(doc, keyword);
        }
    }

    if (isalpha(*doc->_stream)) {
        return csd_eat_id(doc);
    }
    if (*doc->_stream == '-' || *doc->_stream == '+' || isdigit(*doc->_stream)) {
        return csd_eat_number(doc);
    }

    csd_scan_throw(doc, csd_eat_dumb(doc), "unknown token encountered");
}
