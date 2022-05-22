/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <cyb3r@nigge.rs> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Mykhailo Chernysh
 * ----------------------------------------------------------------------------
 */


#pragma once

#include <stdio.h>
#include <stdlib.h>

#define __COLOR_RED "31"
#define __COLOR_GREEN "32"
#define __COLOR_YELLOW "33"

#define __LOG_FMT_MACRO(type, color, fmt, args...)                                  \
    {                                                                               \
        fprintf(stderr, "\033[" color ";1m" type ":\t" fmt "\033[37;0m\n", ##args); \
    }

#define FMT_ERR_NOEXIT(fmt, args...)                        \
    {                                                       \
        __LOG_FMT_MACRO("ERROR", __COLOR_RED, fmt, ##args); \
    }
#define FMT_WARN(fmt, args...)                                \
    {                                                         \
        __LOG_FMT_MACRO("WARN", __COLOR_YELLOW, fmt, ##args); \
    }
#define FMT_INFO(fmt, args...)                               \
    {                                                        \
        __LOG_FMT_MACRO("INFO", __COLOR_GREEN, fmt, ##args); \
    }

#define FMT_ERR(code, fmt, args...)  \
    {                                \
        FMT_ERR_NOEXIT(fmt, ##args); \
        exit(code);                  \
    }
#define FMT_ERR_GOTO(label, fmt, args...) \
    {                                     \
        FMT_ERR_NOEXIT(fmt, ##args);      \
        goto label;                       \
    }
#define FMT_ERR_RET(ret, fmt, args...) \
    {                                  \
        FMT_ERR_NOEXIT(fmt, ##args);   \
        return ret;                    \
    }

#define FMT_WARN_GOTO(label, fmt, args...) \
    {                                      \
        FMT_WARN(fmt, ##args);             \
        goto label;                        \
    }

#define FMT_ERR_RET_ASSERT(ret, expr, fmt, args...) \
    {                                               \
        if (!(expr))                                \
        {                                           \
            FMT_ERR_RET(ret, fmt, ##args);          \
        }                                           \
    }

#define FMT_ERR_ASSERT(code, expr, fmt, args...) \
    {                                            \
        if (!(expr))                             \
        {                                        \
            FMT_ERR(code, fmt, ##args);          \
        }                                        \
    }
