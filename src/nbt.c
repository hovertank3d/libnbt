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
        printf("%ld %s entries\n", tag->payload.List.children_num, 
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
        return 0;
    }
    strcpy(tag->name, name);
    return 1;
}

int nbt_uncompress_file(char *file_path, uint8_t **dst)
{
    uint8_t  buf[BUFLEN];
    size_t   dst_offset;
    gzFile   file;
    FILE    *f;
    int      fd;
    int      read_bytes;

    f = fopen(file_path, "rb");
    if (f == NULL) {
        nbt_set_error("Can't open file: %s", strerror(errno));
    }
    fd = fileno(f);
    file = gzdopen(fd, "rb");
    if (file == NULL) {
        nbt_set_error("Can't gzdopen stdin");
    }

    *dst = NULL;
    dst_offset = 0;
    do {
        read_bytes = gzread(file, buf, BUFLEN);
        if (read_bytes < 0) {
            return Z_STREAM_ERROR;
        }
        *dst = realloc(*dst, dst_offset + read_bytes);
        if (*dst == NULL) {
            nbt_set_error("Out of memory");
        }
        memcpy((*dst)+dst_offset, buf, read_bytes);
        dst_offset += read_bytes;
        
    } while(read_bytes > 0);
    return dst_offset;
}