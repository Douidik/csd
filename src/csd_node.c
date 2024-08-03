#include "csd.h"
#include <stdarg.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void csd_free(csd_document *doc)
{
    for (size_t i = 0; i < arrlen(doc->nodes); i++) {
        csd_free_node(&doc->nodes[i]);
    }
    arrfree(doc->nodes);
    free(doc->source);
}

void csd_free_node(csd_node *node)
{
    csd_free_value(&node->value);
}

void csd_free_value(csd_value *value)
{
    switch (value->type) {
    case csd_type_nil:
    case csd_type_float:
    case csd_type_int:
    case csd_type_string:
    case csd_type_boolean:
    case csd_type_end:
        break;
    case csd_type_array:
        arrfree(value->as_sequence);
        break;
    case csd_type_sequence:
        hmfree(value->as_array);
        break;
    }
}

csd_node *csd_doc_push(csd_document *doc, csd_node node)
{
    arrpush(doc->nodes, node);
    return &arrlast(doc->nodes);
}

csd_node *csd_new_nil(csd_document *doc, const char *name)
{
    return csd_doc_push(doc, (csd_node){name, csd_vnil});
}
csd_node *csd_new_array(csd_document *doc, const char *name, csd_array v)
{
    return csd_doc_push(doc, (csd_node){name, csd_varray(v)});
}
csd_node *csd_new_sequence(csd_document *doc, const char *name, csd_sequence v)
{
    return csd_doc_push(doc, (csd_node){name, csd_vsequence(v)});
}
csd_node *csd_new_float(csd_document *doc, const char *name, double v)
{
    return csd_doc_push(doc, (csd_node){name, csd_vfloat(v)});
}
csd_node *csd_new_int(csd_document *doc, const char *name, int64_t v)
{
    return csd_doc_push(doc, (csd_node){name, csd_vint(v)});
}
csd_node *csd_new_boolean(csd_document *doc, const char *name, bool v)
{
    return csd_doc_push(doc, (csd_node){name, csd_vboolean(v)});
}
csd_node *csd_new_string(csd_document *doc, const char *name, const char *v)
{
    return csd_doc_push(doc, (csd_node){name, csd_vstring(v)});
}

csd_node *csd_sequence_push(csd_sequence *sequence, csd_node *n)
{
    csd_bucket bucket = (csd_bucket){n->key, n};
    return hmputs(*sequence, bucket), n;
}

void csd_sequence_remove(csd_sequence *sequence, const char *key)
{
    hmdel(*sequence, key);
}

csd_node *csd_sequence_get(csd_sequence *sequence, const char *key)
{
    csd_bucket *bucket = hmgetp(*sequence, key);
    if (!bucket)
        return NULL;
    return bucket->value;
}

size_t csd_sequence_count(csd_sequence *sequence)
{
    return hmlen(sequence);
}

csd_node *csd_make_sequence_x(csd_document *doc, const char *name, ...)
{
    csd_node *node;
    csd_node *child;
    va_list list;
    va_start(list, name);

    node = csd_new_sequence(doc, name, NULL);
    while ((child = va_arg(list, csd_node *))->value.type != csd_type_end) {
        csd_insert(node, child);
    }
    
    va_end(list);
    return node;
}

csd_value *csd_array_push(csd_array *array, csd_value v)
{
    arrpush(*array, v);
    return &arrlast(*array);
}

size_t csd_array_len(csd_array *array)
{
    return arrlen(array);
}

void csd_neq_none(csd_node *a, csd_node *b, void *data)
{
    (void)a;
    (void)b;
    (void)data;
}

#define return_neq return neq_cb(a, b, data), false
bool csd_value_eq(csd_node *a, csd_node *b, csd_value *va, csd_value *vb,
                  csd_neq_cb neq_cb, void *data)
{
    if (va->type != vb->type)
        return_neq;

    switch (va->type) {
    case csd_type_nil:
    case csd_type_end:
        break;

    case csd_type_array: {
        csd_array via = va->as_array;
        csd_array vib = vb->as_array;

        if (csd_len(a) != csd_len(b))
            return_neq;
        for (size_t i = 0; i < csd_len(a); i++) {
            if (!csd_value_eq(a, b, &via[i], &vib[i], neq_cb, data))
                return_neq;
        }
    } break;

    case csd_type_sequence: {
        csd_sequence via = va->as_sequence;
        csd_sequence vib = vb->as_sequence;

        if (csd_count(a) != csd_count(b))
            return_neq;
        for (size_t i = 0; i < csd_count(a); i++) {
            if (!csd_eq_x((via[i].value), (vib[i].value), neq_cb, data))
                return_neq;
        }
        break;
    }

    case csd_type_float:
        if (va->as_float != vb->as_float)
            return_neq;
        break;
    case csd_type_int:
        if (va->as_int != vb->as_int)
            return_neq;
        break;
    case csd_type_boolean:
        if (va->as_boolean != vb->as_boolean)
            return_neq;
        break;
    case csd_type_string:
        if (strcmp(va->as_string, vb->as_string) != 0)
            return_neq;
        break;
    }
    return true;
}

bool csd_eq_x(csd_node *a, csd_node *b, csd_neq_cb neq_cb, void *data)
{
    if (!neq_cb)
        neq_cb = &csd_neq_none;

    if ((a != NULL) ^ (b != NULL))
        return_neq;
    if (strcmp(a->key, b->key) != 0)
        return_neq;
    if (!csd_value_eq(a, b, &a->value, &b->value, neq_cb, data))
        return_neq;
    return true;
}

bool csd_eq(csd_node *a, csd_node *b)
{
    return csd_eq_x(a, b, csd_neq_none, NULL);
}
