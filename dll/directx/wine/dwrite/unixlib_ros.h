/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#ifdef __REACTOS__
#include "wine/winternl.h"
#else
#include "winternl.h"
#endif
#include "dwrite.h"
#include "dwrite_private.h"
#ifndef __REACTOS__
#include "wine/unixlib.h"
#endif

extern NTSTATUS process_attach(void *args);
extern NTSTATUS process_detach(void *args);
extern NTSTATUS create_font_object(void *args);
extern NTSTATUS release_font_object(void *args);
extern NTSTATUS get_glyph_outline(void *args);
extern NTSTATUS get_glyph_count(void *args);
extern NTSTATUS get_glyph_advance(void *args);
extern NTSTATUS get_glyph_bbox(void *args);
extern NTSTATUS get_glyph_bitmap(void *args);
extern NTSTATUS get_design_glyph_metrics(void *args);

#undef UNIX_CALL
#define UNIX_CALL(name, arg) name(arg)