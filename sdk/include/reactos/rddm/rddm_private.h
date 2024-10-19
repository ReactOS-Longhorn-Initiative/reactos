/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header file for RDDM Undocumented shared info
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* Create an IO request to fill out the function pointer list */
#define IOCTL_VIDEO_DDI_FUNC_REGISTER \
	CTL_CODE( FILE_DEVICE_VIDEO, 0xF, METHOD_NEITHER, FILE_ANY_ACCESS  )

