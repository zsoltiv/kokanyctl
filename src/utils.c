#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

void ctl_die(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
    exit(1);
}
