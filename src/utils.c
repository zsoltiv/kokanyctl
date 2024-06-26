/*
 * copyright (c) 2023 Zsolt Vadasz
 *
 * This file is part of kokanyctl.
 *
 * kokanyctl is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * kokanyctl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kokanyctl. If not, see <https://www.gnu.org/licenses/>. 
*/

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
