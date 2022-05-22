/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */

/*
 * NBT to JSON converting example
 */


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "nbt.h"
#include "logger.h"

void tab(FILE *f, int level)
{
    for(int i = 0; i < level; i++) {
        fprintf(f, "    ");
    }
}

void nbt2json(FILE *f, NBT_Tag *tag, int nest_level)
{
    char *coma;
    coma = nest_level?",":"";
    tab(f, nest_level);
    if (tag->name && nest_level) {
        fprintf(f, "\"%s\": ", tag->name);
    }
    switch (tag->type) {
    case TAG_Compound_ID:
        fprintf(f, "{\n");
        for (int i = 0; i < tag->payload.Compound.children_num; i++) {
            nbt2json(f, tag->payload.Compound.children + i, nest_level+1);
        }
        tab(f, nest_level);
        fprintf(f, "}%s\n", coma);
        break;

    case TAG_List_ID:
        fprintf(f, "[\n");
        for (int i = 0; i < tag->payload.List.children_num; i++) {
            nbt2json(f, tag->payload.List.children + i, nest_level+1);
        }
        tab(f, nest_level);
        fprintf(f, "]%s\n", coma);
        break;

    case TAG_String_ID:
        fprintf(f, "\"%s\"%s\n", tag->payload.String, coma);
        break;

    case TAG_Byte_ID:
    case TAG_Short_ID:
    case TAG_Int_ID:
    case TAG_Long_ID:
        fprintf(f, "%ld%s\n", *(int64_t *)&tag->payload, coma);
        break;

    case TAG_Float_ID:
    case TAG_Double_ID:
        fprintf(f, "%f%s\n", *(double *)&tag->payload, coma);
        break;

    case TAG_Byte_Array_ID:
        fprintf(f, "[\n");
        for (int i = 0; i < tag->payload.Byte_Array.arr_len; i++) {
            tab(f, nest_level+1);
            fprintf(f, "%d,\n", tag->payload.Byte_Array.arr[i]);
        }
        tab(f, nest_level);
        fprintf(f, "]%s\n", coma);
        break;
    case TAG_Int_Array_ID:
        fprintf(f, "[\n");
        for (int i = 0; i < tag->payload.Int_Array.arr_len; i++) {
            tab(f, nest_level+1);
            fprintf(f, "%d,\n", tag->payload.Int_Array.arr[i]);
        }
        tab(f, nest_level);
        fprintf(f, "]%s\n", coma);
        break;
    case TAG_Long_Array_ID:
        fprintf(f, "[\n");
        for (int i = 0; i < tag->payload.Long_Array.arr_len; i++) {
            tab(f, nest_level+1);
            fprintf(f, "%ld,\n", tag->payload.Long_Array.arr[i]);
        }
        tab(f, nest_level);
        fprintf(f, "]%s\n", coma);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv)
{
    NBT_Tag *root;
    FILE *out_file;
    uint8_t *uncompressed;
    size_t   size;
    char    *in_file_path;
    char    *out_file_path;
    
    if (argc != 3) {
        printf("requires 2 arguments!\n");
        return 1;
    }
    in_file_path = argv[1];
    out_file_path = argv[2];

    size = nbt_uncompress_file(in_file_path, &uncompressed);
    if (size <= 0) {
        FMT_ERR(1, "zlib error: %s", nbt_get_error());
    }

    root = nbt_parse_root(uncompressed, size);
    if (root == NULL) {
        FMT_ERR(1, "error parsing nbt: %s", nbt_get_error());
    }
    
    
    out_file = fopen(out_file_path, "w");
    if (out_file == NULL) {
        FMT_ERR(1, "error reading file: %s", strerror(errno));
    }
    nbt2json(out_file, root, 0);
    fclose(out_file);
}