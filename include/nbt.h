/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */


#ifndef __NBT_H__
#define __NBT_H__

#include <stdint.h>
#include <stddef.h> //for size_t

enum NBT_Tags {
    TAG_End_ID = 0,
    TAG_Byte_ID,
    TAG_Short_ID,
    TAG_Int_ID,
    TAG_Long_ID,
    TAG_Float_ID,
    TAG_Double_ID,
    TAG_Byte_Array_ID,
    TAG_String_ID,
    TAG_List_ID,
    TAG_Compound_ID,
    TAG_Int_Array_ID,
    TAG_Long_Array_ID,
    TAGS_NUM,
};

struct NBT_Tag;
union nbt_payload;

typedef struct nbt_list_payload {
    int             children_type;
    int32_t         children_num;
    struct NBT_Tag*  children;

} nbt_list_payload;

typedef struct nbt_coumpound_payload {
    size_t          children_num;
    struct NBT_Tag* children;

} nbt_coumpound_payload;

typedef struct nbt_byte_array_payload {
    int32_t arr_len;
    int8_t* arr;

} nbt_byte_array_payload;

typedef struct nbt_int_array_payload {
    int32_t  arr_len;
    int32_t* arr;

} nbt_int_array_payload;

typedef struct nbt_long_array_payload {
    int32_t  arr_len;
    int64_t* arr;

} nbt_long_array_payload;

typedef union nbt_payload{
    int8_t Byte;
    int16_t Short;
    int32_t Int;
    int64_t Long;
    float Float;
    double Double;
    char *String;

    nbt_byte_array_payload Byte_Array;
    nbt_int_array_payload Int_Array;
    nbt_long_array_payload Long_Array;
    nbt_list_payload List;
    nbt_coumpound_payload Compound;

} nbt_payload;

typedef struct NBT_Tag {
    uint8_t type;
    char *name;
    nbt_payload payload;

} NBT_Tag;

int nbt_uncompress_file(char *file_path, uint8_t **dst);
NBT_Tag *nbt_parse_root(uint8_t *data, int data_len);
void nbt_free_tag(NBT_Tag *tag);
int nbt_tag_set_name(NBT_Tag *tag, const char *name);

uint8_t *nbt_dump_tree(NBT_Tag *tag, int *sz);

const char *nbt_get_error();

void nbt_pretty_print(NBT_Tag *tag);

#define CONVERT_TYPE(type, data) (*(type *)(data))

#endif