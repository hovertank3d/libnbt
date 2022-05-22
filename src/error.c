/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "nbterr.h"

/* TODO: thread safe errors */
static nbt_error *nbt_get_error_buffer()
{
    static nbt_error errbuf;
    return &errbuf;
}

void nbt_clear_error()
{
    nbt_get_error_buffer()->error = 0;
}

void nbt_set_error(const char *fmt, ...)
{
    nbt_error *err;
    va_list args;

    err = nbt_get_error_buffer();
    
    va_start(args, fmt);
    vsnprintf(err->message, ERRBUFLEN, fmt, args);
    va_end(args);

    err->error = 1;
}

const char *nbt_get_error()
{
    nbt_error *err = nbt_get_error_buffer();
    return err->error ? err->message : "";
}