/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */


#ifndef __NBTERR_H__
#define __NBTERR_H__

#define ERRBUFLEN 512

typedef struct nbt_error {
    int error;
    char message[ERRBUFLEN];
} nbt_error;

void nbt_clear_error();
void nbt_set_error(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#endif