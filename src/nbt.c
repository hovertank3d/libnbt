/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */


#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "nbt.h"
#include "nbterr.h"

#define BUFLEN 512

char *nbt_tag_names[] = {
    [TAG_Byte_ID]       = "TAG_Byte",
    [TAG_Short_ID]      = "TAG_Short",
    [TAG_Int_ID]        = "TAG_Int",
    [TAG_Long_ID]       = "TAG_Long",
    [TAG_Float_ID]      = "TAG_Float",
    [TAG_Double_ID]     = "TAG_Double",
    [TAG_Byte_Array_ID] = "TAG_Byte_Array",
    [TAG_String_ID]     = "TAG_String",
    [TAG_List_ID]       = "TAG_List",
    [TAG_Compound_ID]   = "TAG_Compound",
    [TAG_Int_Array_ID]  = "TAG_Int_Array",
    [TAG_Long_Array_ID] = "TAG_Long_Array"
};

static void print_tab(int count) 
{
    for (int i = 0; i < count; i++) {
        printf("    ");
    }
}

static void nbt_pretty_print_(NBT_Tag *tag, int nest_level)
{
    int print_name;

    print_name = !!tag->name; 
    print_tab(nest_level);
    printf("%s(%s%s%s): ",
           nbt_tag_names[tag->type],
           print_name ? "'" : "",
           print_name ? tag->name : "None",
           print_name ? "'" : "");

    switch (tag->type)
    {
    case TAG_String_ID:
        printf("'%s'", tag->payload.String);
        break;
    case TAG_Byte_ID:
        printf("%d", tag->payload.Byte);
        break;
    case TAG_Short_ID:
        printf("%d", tag->payload.Short);
        break;
    case TAG_Int_ID:
        printf("%d", tag->payload.Int);
        break;
    case TAG_Long_ID:
        printf("%ld", tag->payload.Long);
        break;
    case TAG_Float_ID:
        printf("%f", tag->payload.Float);
        break;
    case TAG_Double_ID:
        printf("%f", tag->payload.Double);
        break;
    case TAG_Byte_Array_ID:
        printf("[%d bytes]", tag->payload.Byte_Array.arr_len);
        break;
    case TAG_Int_Array_ID:
        printf("[%d bytes]", tag->payload.Int_Array.arr_len * 4);
        break;
    case TAG_Long_Array_ID:
        printf("[%d bytes]", tag->payload.Long_Array.arr_len * 8);
        break;
    case TAG_Compound_ID:
        printf("%ld entries\n", tag->payload.Compound.children_num);
        print_tab(nest_level);
        printf("{\n");
        for (int i = 0; i < tag->payload.Compound.children_num; i++) {
            nbt_pretty_print_(tag->payload.Compound.children + i, nest_level + 1);
        }
        print_tab(nest_level);
        printf("}");
        break;
    case TAG_List_ID:
        printf("%d %s entries\n", tag->payload.List.children_num, 
            nbt_tag_names[tag->payload.List.children_type]);
        print_tab(nest_level);
        printf("{\n");
        for (int i = 0; i < tag->payload.List.children_num; i++) {
            nbt_pretty_print_(tag->payload.List.children + i, nest_level + 1);
        }
        print_tab(nest_level);
        printf("}");
        break;
    }

    putc('\n', stdout);
}

void nbt_pretty_print(NBT_Tag *tag)
{
    nbt_pretty_print_(tag, 0);
}

static void _nbt_free_tag(NBT_Tag *tag)
{
    if (tag->name) {
        free(tag->name);
    }

    switch (tag->type) {
    case TAG_Compound_ID:
        for(int i = 0; i < tag->payload.Compound.children_num; i++) {
            _nbt_free_tag(tag->payload.Compound.children + i);
        }
        free(tag->payload.Compound.children);
        break;
    case TAG_List_ID:
        for(int i = 0; i < tag->payload.List.children_num; i++) {
            _nbt_free_tag(tag->payload.List.children + i);
        }
        free(tag->payload.List.children);
        break;
    case TAG_String_ID:
        if (tag->payload.String != NULL) {
            free(tag->payload.String);
        }
        break;
    case TAG_Byte_Array_ID:
        if (tag->payload.Byte_Array.arr_len) {
            free(tag->payload.Byte_Array.arr);
        }
        break;
    case TAG_Int_Array_ID:
        if (tag->payload.Int_Array.arr_len) {
            free(tag->payload.Int_Array.arr);
        }
        break;
    case TAG_Long_Array_ID:
        if (tag->payload.Long_Array.arr_len) {
            free(tag->payload.Long_Array.arr);
        }
        break;
    default:
        break;
    }
}

void nbt_free_tag(NBT_Tag *tag)
{
    _nbt_free_tag(tag);
    free(tag);
}

int nbt_tag_set_name(NBT_Tag *tag, const char *name)
{
    int name_len;
    if (tag->name) {
        free(tag->name);
    }

    name_len = strlen(name) + 1;
    tag->name = malloc(name_len);
    if (tag->name == NULL) {
        nbt_set_error("Out of memory");
        return 1;
    }
    strcpy(tag->name, name);
    return 0;
}

int nbt_compress_file(const char *file_path, uint8_t *data, int size)
{
    FILE *f;
    gzFile file;
    int      fd;


    f = fopen(file_path, "wb");
    if (f == NULL) {
        nbt_set_error("Can't open file: %s", strerror(errno));
        return 1;
    }
    fd = fileno(f);
    file = gzdopen(fd, "wb");
    if (file == NULL) {
        nbt_set_error("Can't gzdopen stdin");
        fclose(f);
        return 1;
    }
    
    if (gzwrite(file, data, size) != size) {
        nbt_set_error("Z_STREAM_ERROR");
        gzclose(file);
        fclose(f);
        return 1;
    }

    gzclose(file);
    fclose(f);
    return 0;
}

uint8_t *nbt_uncompress_file(const char *file_path, int *size)
{
    uint8_t  buf[BUFLEN];
    uint8_t  *dst;
    size_t   dst_offset;
    gzFile   file;
    FILE    *f;
    int      fd;
    int      read_bytes;


    f = fopen(file_path, "rb");
    if (f == NULL) {
        nbt_set_error("Can't open file: %s", strerror(errno));
        return NULL;
    }
    fd = fileno(f);
    file = gzdopen(fd, "rb");
    if (file == NULL) {
        nbt_set_error("Can't gzdopen stdin");
        goto err_gzip;
    }

    dst = NULL;
    dst_offset = 0;
    do {
        read_bytes = gzread(file, buf, BUFLEN);
        if (read_bytes < 0) {
            nbt_set_error("Z_STREAM_ERROR");
            goto err_io;
        }
        dst = realloc(dst, dst_offset + read_bytes);
        if (dst == NULL) {
            nbt_set_error("Out of memory");
            goto err_alloc;
        }
        memcpy(dst+dst_offset, buf, read_bytes);
        dst_offset += read_bytes;
        
    } while(read_bytes > 0);

    *size = dst_offset;

    return dst;

err_io:
    free(dst);
err_alloc:
    gzclose(file);
err_gzip:
    fclose(f);
    return NULL;
}

NBT_Tag *nbt_create_tag(const char *name, uint8_t type, nbt_payload data)
{
    NBT_Tag *tag = malloc(sizeof(NBT_Tag));
    if (tag == NULL) {
        nbt_set_error("Out of memory");
        return NULL;
    }

    if (name) {
        tag->name = malloc(strlen(name) + 1);

        if (tag->name == NULL) {
            nbt_set_error("Out of memory");
            goto err;
        }
        strcpy(tag->name, name);

    } else {
        tag->name = NULL;
    }

    tag->type = TAG_List_ID;
    tag->payload = data;

    return tag;
err:
    free(tag);
    return NULL;
}

NBT_Tag *nbt_create_list(const char *name, int32_t len, uint8_t type, const nbt_payload *children)
{
    nbt_payload payload;
    
    payload.List.children_type = type;
    payload.List.children_num = len;

    if (len == 0) {
        payload.List.children = NULL;
    } else {
        payload.List.children = malloc(len * sizeof(nbt_payload));
        if (payload.List.children == NULL) {
            nbt_set_error("Out of memory");
            return NULL;
        }
        memcpy(payload.List.children, children, len * sizeof(nbt_payload));
    }

    return nbt_create_tag(name, TAG_List_ID, payload);
}

NBT_Tag *nbt_create_compound(const char *name, int32_t len, const NBT_Tag *children)
{
    nbt_payload payload;
    
    payload.Compound.children_num = len;

    if (len == 0) {
        payload.Compound.children = NULL;
    } else {
        payload.Compound.children = malloc(len * sizeof(nbt_payload));
        if (payload.Compound.children == NULL) {
            nbt_set_error("Out of memory");
            return NULL;
        }

        memcpy(payload.Compound.children, children, len * sizeof(nbt_payload));
    }

    return nbt_create_tag(name, TAG_Compound_ID, payload);
}

NBT_Tag *nbt_create_array(const char *name, const void *data, int size, int count)
{
    nbt_payload payload;
    void       *arr;
    int         type;

    if (size != 1 && size != 4 && size != 8) {
        nbt_set_error("Unknown array type");
        return NULL;
    }

    arr = malloc(size*count);
    if (arr == NULL) {
        nbt_set_error("Out of memoty");
        return NULL;
    }
    memcpy(arr, data, size*count);

    switch (size) {
    case 1:
        payload.Byte_Array.arr_len = count;
        payload.Byte_Array.arr = arr;
        type = TAG_Byte_Array_ID;
        break;
    case 4:
        payload.Int_Array.arr_len = count;
        payload.Int_Array.arr = arr;
        type = TAG_Int_Array_ID;
        break;
    case 8:
        payload.Long_Array.arr_len = count;
        payload.Long_Array.arr = arr;
        type = TAG_Long_Array_ID;
        break;
    }

    return nbt_create_tag(name, type, payload);
}

NBT_Tag *nbt_create_string(const char *name, const char *data)
{
    nbt_payload payload;

    payload.String = malloc(strlen(data) + 1);
    if (payload.String == NULL) {
        nbt_set_error("Out of memory");
        return NULL;
    }
    strcpy(payload.String, data);

    return nbt_create_tag(name, TAG_String_ID, payload);
}

NBT_Tag *nbt_create_byte(const char *name, int8_t data)
{
    nbt_payload payload;
    payload.Byte = data;
    return nbt_create_tag(name, TAG_Byte_ID, payload);
}

NBT_Tag *nbt_create_short(const char *name, int16_t data)
{
    nbt_payload payload;
    payload.Short = data;
    return nbt_create_tag(name, TAG_Short_ID, payload);
}

NBT_Tag *nbt_create_int(const char *name, int32_t data)
{
    nbt_payload payload;
    payload.Int = data;
    return nbt_create_tag(name, TAG_Int_ID, payload);
}

NBT_Tag *nbt_create_long(const char *name, int64_t data)
{
    nbt_payload payload;
    payload.Long = data;
    return nbt_create_tag(name, TAG_Long_ID, payload);
}

NBT_Tag *nbt_create_float(const char *name, float data)
{
    nbt_payload payload;
    payload.Float = data;
    return nbt_create_tag(name, TAG_Float_ID, payload);
}

NBT_Tag *nbt_create_double(const char *name, double data)
{
    nbt_payload payload;
    payload.Double = data;
    return nbt_create_tag(name, TAG_Double_ID, payload);
}

int nbt_insert_tag(NBT_Tag *parent, NBT_Tag *tag)
{
    if (parent->type != TAG_Compound_ID && parent->type != TAG_List_ID) {
        nbt_set_error("%s can't have children", nbt_tag_names[parent->type]);
        return 1;
    }

    if (parent->type == TAG_Compound_ID) {

        nbt_coumpound_payload *p = &parent->payload.Compound;
        p->children_num++;
        p->children = realloc(p->children, p->children_num * sizeof(NBT_Tag));
        p->children[p->children_num-1] = *tag;
    } else if (parent->type == TAG_List_ID) {

        nbt_list_payload *p = &parent->payload.List;
        if (p->children_type != tag->type) {
            nbt_set_error("List have other type");
            return 1;
        }

        if (tag->name) {
            free(tag->name);
        }

        p->children_num++;
        p->children = realloc(p->children, p->children_num * sizeof(NBT_Tag));
        p->children[p->children_num-1] = *tag;
    }
    
    free(tag);
    
    return 0;
}