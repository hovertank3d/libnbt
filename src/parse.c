/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <endian.h>
#include "nbt.h"
#include "nbterr.h"


static int nbt_parse_tag(uint8_t *data, size_t data_size, NBT_Tag *tag);
static int nbt_parse_compound_payload(uint8_t *data, size_t data_size, void **payload);
static int nbt_load_string(uint8_t *data, size_t data_size, char **str);
static int nbt_load_short(uint8_t *data, size_t data_size, int16_t *num);
static int nbt_load_int(uint8_t *data, size_t data_size, int32_t *num);
static int nbt_load_long(uint8_t *data, size_t data_size, int64_t *num);
static int nbt_load_float(uint8_t *data, size_t data_size, float *num);
static int nbt_load_double(uint8_t *data, size_t data_size, double *num);
static int nbt_load_array(uint8_t *data, size_t data_size, int type_size, void **arr, uint32_t *len);
static int nbt_load_payload(uint8_t *data, size_t data_size, int type, nbt_payload *payload);
static int nbt_load_compound_payload(uint8_t *data, size_t data_size, nbt_coumpound_payload *payload);


static int nbt_load_list_payload(uint8_t *data, size_t data_size, nbt_list_payload *payload)
{
    int offset = 0;
    int err;

    if (data_size < 3) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    payload->children_type = data[0];
    offset += 1;
    err = nbt_load_int(data + offset, data_size - offset, &(payload->children_num));
    if (!err) {
        return err;
    }
    offset += err;

    payload->children = malloc(payload->children_num * sizeof(NBT_Tag));
    if (payload->children == NULL) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }

    for (int i = 0; i < payload->children_num; i++) {
        payload->children[i].type = payload->children_type;
        payload->children[i].name = NULL;

        err = nbt_load_payload(data + offset, data_size - offset,
                payload->children_type,
                &(payload->children[i].payload));

        if (!err) {
            goto error;
        }

        offset += err;             
    }

    return offset;
error:
    free(payload->children);
    return err;
}

static int nbt_load_compound_payload(uint8_t *data, size_t data_size, nbt_coumpound_payload *payload)
{
    int offset = 0;
    int err;

    if (data_size < 1) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }

    payload->children_num = 0;
    payload->children = NULL;

    while (data[offset] != 0) {
        payload->children_num++;
        payload->children = realloc(payload->children, sizeof(NBT_Tag) * payload->children_num);
        if (payload->children == NULL) {
            nbt_set_error("Out of memory");
            return 0;
        }
        err = nbt_parse_tag(data + offset, data_size - offset, 
                &payload->children[payload->children_num - 1]);

        if (!err) {
            goto error;
        }
        offset += err;             
    }

    return offset + 1; //1 more for TAG_End
error:
    free(payload->children);
    return err;
}

static int nbt_load_array(uint8_t *data, size_t data_size, int type_size, void **arr, uint32_t *len)
{
    int err;
    int offset = 0;
    int arr_size;

    err = nbt_load_int(data, data_size, len);
    if (!err) {
        return err;
    }
    offset += err;

    arr_size = *len * type_size;
    
    if (data_size - offset < arr_size) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    if (arr_size == 0) {
        *arr = NULL;
        return offset;
    } 
    *arr = malloc(arr_size);
    if (*arr == NULL) {
        nbt_set_error("Out of memory");
        return 0;
    }

    switch (type_size)
    {
    case sizeof(int8_t):
        memcpy(*arr, data + offset, arr_size);
        break;
    case sizeof(int32_t):
        for (int i = 0; i < *len; i++) {
            nbt_load_int(data + offset, data_size - offset, *arr + i * type_size);
        }
        break;
    case sizeof(int64_t):
        for (int i = 0; i < *len; i++) {
            nbt_load_long(data + offset, data_size - offset, *arr + i * type_size);
        }
        break;
    default:
        nbt_set_error("Unknown array type");
        free(*arr);
        return 0;
    }

    return offset + arr_size;
}

static int nbt_load_payload(uint8_t *data, size_t data_size, int type, nbt_payload *payload) {
    switch (type)
    {
    case TAG_Byte_ID:
        if (data_size < 1) {
            nbt_set_error("Unexpected EOF");
            return 0;
        }
        payload->Byte = data[0];
        return 1;
    case TAG_Short_ID:
        return nbt_load_short(data, data_size, &(payload->Short));;
    case TAG_Int_ID:
        return nbt_load_int(data, data_size, &(payload->Int));
    case TAG_Long_ID:
        return nbt_load_long(data, data_size, &(payload->Long));
    case TAG_Float_ID:
        return nbt_load_float(data, data_size, &(payload->Float));
    case TAG_Double_ID:
        return nbt_load_double(data, data_size, &(payload->Double));
    case TAG_String_ID:
        return nbt_load_string(data, data_size, &(payload->String));
    case TAG_Byte_Array_ID:
        return nbt_load_array(data, data_size, 1,
                    (void**)&(payload->Byte_Array.arr),
                    &(payload->Byte_Array.arr_len));
    case TAG_Int_Array_ID:
        return nbt_load_array(data, data_size, 4,
                    (void**)&(payload->Int_Array.arr),
                    &(payload->Int_Array.arr_len));
    case TAG_Long_Array_ID:
        return nbt_load_array(data, data_size, 8,
                    (void**)&(payload->Long_Array.arr),
                    &(payload->Long_Array.arr_len));
    case TAG_Compound_ID:
        return nbt_load_compound_payload(data, data_size,
                    &(payload->Compound));
    case TAG_List_ID:
        return nbt_load_list_payload(data, data_size,
                    &(payload->List));
    default:
        break;
    }
}

static int nbt_parse_tag(uint8_t *data, size_t data_size, NBT_Tag *tag)
{
    size_t offset = 0;
    int err;

    if (data_size <= 0) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }

    if ((tag->type = data[0]) >= TAGS_NUM) {
        nbt_set_error("Incorrect TAG ID: %02x", data[0]);
        return 0;
    }
    if (tag->type == 0) {
        nbt_set_error("Incorrect TAG ID");
        return 0;
    }
    offset += 1;

    err = nbt_load_string(data + offset, data_size - offset, &(tag->name));
    if (!err) {
        return err;
    }
    offset += err;
    
    err = nbt_load_payload(data + offset, data_size - offset, data[0], &(tag->payload));
    if (!err) {
        return err;
    }
    offset += err;

    return offset;
}

NBT_Tag *nbt_parse_root(uint8_t *data, int data_len)
{
    NBT_Tag *root;
    root = malloc(sizeof(NBT_Tag));
    if (root == NULL) {
        nbt_set_error("Out of memory");
        return NULL;
    }
    if (!nbt_parse_tag(data, data_len, root)) {
        return NULL;
    }
    return root;
}

static int nbt_load_string(uint8_t *data, size_t data_size, char **str)
{
    int16_t string_len;
    int offset = 0;
    int err;
    
    err = nbt_load_short(data, data_size, &string_len);
    if (!err) {
        return err;
    }
    offset += err;
    
    if (string_len == 0) {
        *str = NULL;
        return offset;    
    }

    if (data_size - offset < string_len) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    
    *str = malloc(string_len+1); // one more for \0
    if (*str == NULL) {
        nbt_set_error("Out of memory");
        return 0;
    }

    memcpy(*str, data + offset, string_len);
    (*str)[string_len] = 0;

    offset += string_len;
    return offset;      
}

static int little_endian()
{
    uint16_t t = 0x0001;
    char c[2];
    memcpy(c, &t, sizeof t);
    return c[0];
}


static int16_t nbt_swap_short(int16_t num)
{
    if (!little_endian()) return num; 
    return __builtin_bswap16(num);
}

static int32_t nbt_swap_int(int32_t num)
{
    if (!little_endian()) return num; 
    return __builtin_bswap32(num);
}

static int64_t nbt_swap_long(int64_t num)
{
    if (!little_endian()) return num; 
    return __builtin_bswap64(num);
}


static float nbt_swap_float(float num)
{
    int32_t temp;
    if (!little_endian()) return *((float *)(&num));

    temp = __builtin_bswap32(*(int32_t *)&num);
    return *(float *)&temp;
}

static double nbt_swap_double(double num)
{
    int64_t temp;
    if (!little_endian()) return *((double *)(&num));

    temp = __builtin_bswap64(*(int64_t *)&num);
    return *(double *)&temp;
}

static int nbt_load_short(uint8_t *data, size_t data_size, int16_t *num)
{
    if (data_size < 2) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    *num = nbt_swap_short(*(int16_t *)data);
    return 2;
}

static int nbt_load_int(uint8_t *data, size_t data_size, int32_t *num)
{
    if (data_size < 4) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    *num = nbt_swap_int(*(int32_t *)data);
    return 4;
}

static int nbt_load_long(uint8_t *data, size_t data_size, int64_t *num)
{
    if (data_size < 8) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    *num = nbt_swap_long(*(int64_t *)data);
    return 8;
}

static int nbt_load_float(uint8_t *data, size_t data_size, float *num)
{
    if (data_size < 4) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    *num = nbt_swap_float(*(float*)data);
    return 4;
}

static int nbt_load_double(uint8_t *data, size_t data_size, double *num)
{
    if (data_size < 8) {
        nbt_set_error("Unexpected EOF");
        return 0;
    }
    *num = nbt_swap_double(*(double*)data);
    return 8;
}

int nbt_dump_tag(NBT_Tag *tag, uint8_t *buffer, int only_payload)
{

    int16_t name_len;
    uint8_t *buf = buffer;

    if (!only_payload) {
        *buf = tag->type;
        buf += 1;

        if (tag->name) {
            name_len = strlen(tag->name);
            *(int16_t *)buf = nbt_swap_short(name_len);
            buf += 2;
            memcpy(buf, tag->name, name_len);
            buf += name_len;
        } else {
            *(int16_t *)buf = 0;
            buf += 2;
        }
    }

    switch (tag->type) {
    case TAG_Byte_ID:
        *buf = tag->payload.Byte;
        buf += 1;
        break;
    case TAG_Short_ID:
        *(int16_t *)buf = nbt_swap_short(tag->payload.Short);
        buf += 2;
        break;
    case TAG_Int_ID:
        *(int32_t *)buf = nbt_swap_int(tag->payload.Int);
        buf += 4;
        break;
    case TAG_Long_ID:
        *(int32_t *)buf = nbt_swap_long(tag->payload.Long);
        buf += 8;
        break;
    case TAG_Float_ID:
        *(float *)buf = nbt_swap_float(tag->payload.Float);
        buf += 4;
        break;
    case TAG_Double_ID:
        *(double *)buf = nbt_swap_double(tag->payload.Double);
        buf += 8;
        break;
    case TAG_Byte_Array_ID:
        *(int32_t *)buf = nbt_swap_int(tag->payload.Byte_Array.arr_len);
        buf += 4;
        memcpy(buf, tag->payload.Byte_Array.arr, tag->payload.Byte_Array.arr_len);
        buf += tag->payload.Byte_Array.arr_len;
        break;
    case TAG_Int_Array_ID:
        *(int32_t *)buf = nbt_swap_int(tag->payload.Byte_Array.arr_len);
        buf += 4;
        for(int i = 0; i < tag->payload.Int_Array.arr_len; i++) {
            *(int32_t *)buf = nbt_swap_int(tag->payload.Int_Array.arr[i]);
            buf += 4;
        }
        break;
    case TAG_Long_Array_ID:
        *(int32_t *)buf = nbt_swap_int(tag->payload.Byte_Array.arr_len);
        buf += 4;
        for(int i = 0; i < tag->payload.Long_Array.arr_len; i++) {
            *(int64_t *)buf = nbt_swap_long(tag->payload.Long_Array.arr[i]);
            buf += 8;
        }
        break;
    case TAG_String_ID:
        *(int16_t *)buf = nbt_swap_short(strlen(tag->payload.String));
        buf += 2;
        memcpy(buf, tag->payload.String, strlen(tag->payload.String));
        buf += strlen(tag->payload.String);
        break;
    case TAG_Compound_ID:
        for (int i = 0; i < tag->payload.Compound.children_num; i++) {
            buf += nbt_dump_tag(tag->payload.Compound.children + i, buf, 0);
        }
        *buf = 0;
        buf++;
        break;
    case TAG_List_ID:
        *buf = tag->payload.List.children_type;
        buf++;
        *(int32_t *)buf = nbt_swap_int(tag->payload.List.children_num);
        buf += 4;
        for (int i = 0; i < tag->payload.List.children_num; i++) {
            buf += nbt_dump_tag(tag->payload.List.children + i, buf, 1);
        }
        break;
    default:
        break;
    }

    return buf - buffer;
}

int nbt_calculate_tag_size(NBT_Tag *tag, int only_payload)
{
    int size = 0;
    
    if (!only_payload) {
        size += 1; // type id byte
        size += 2; // name len
        if (tag->name) {
            size += strlen(tag->name);
        }
    }

    switch (tag->type) {
    case TAG_String_ID:
        size += 2;
        if (tag->payload.String) {
            size += strlen(tag->payload.String);
        }
        break;

    case TAG_Byte_Array_ID:
        size += 4;
        size += tag->payload.Byte_Array.arr_len;
        break;
    case TAG_Int_Array_ID:
        size += 4;
        size += tag->payload.Int_Array.arr_len * 4;
        break;
    case TAG_Long_Array_ID:
        size += 4;
        size += tag->payload.Long_Array.arr_len * 8;
        break;
    case TAG_Byte_ID:
        size += 1;
        break;
    case TAG_Short_ID:
        size += 2;
        break;
    case TAG_Int_ID:
        size += 4;
        break;
    case TAG_Long_ID:
        size += 8;
        break;
    case TAG_Float_ID:
        size += 4;
        break;
    case TAG_Double_ID:
        size += 8;
        break;
    case TAG_Compound_ID:
        for (int i = 0; i < tag->payload.Compound.children_num; i++) {
            size += nbt_calculate_tag_size(tag->payload.Compound.children + i, 0);
        }
        size += 1; // TAG_End
        break;
    case TAG_List_ID:
        size += 1; // type id
        size += 4; // len
        for (int i = 0; i < tag->payload.List.children_num; i++) {
            size += nbt_calculate_tag_size(tag->payload.List.children + i, 1);
        }
        break;
    }
    return size;
}

uint8_t *nbt_dump_tree(NBT_Tag *tag, int *sz) {
    uint8_t *buf;
    size_t buf_size;

    buf_size = nbt_calculate_tag_size(tag, 0); 
    buf = malloc(buf_size);
    *sz = nbt_dump_tag(tag, buf, 0);
    return buf;
}