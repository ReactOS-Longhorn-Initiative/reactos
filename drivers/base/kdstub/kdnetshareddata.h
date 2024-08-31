/*++

Copyright (c) Microsoft Corporation

Module Name:

    kdnetshareddata.h

Abstract:

    Defines the public interface for the kdnet data that is shared with its
    extensibility modules.

Author:

    Joe Ballantyne (joeball) 2-18-2012

--*/

#pragma once

#define MAC_ADDRESS_SIZE 6
#define UNDI_DEFAULT_HARDWARE_CONTEXT_SIZE ((512 + 10) * 1024)

#if defined(_AMD64_)
#define MAX_HARDWARE_CONTEXT_SIZE (160*1024*1024)
#else
#define MAX_HARDWARE_CONTEXT_SIZE (16*1024*1024)
#endif

//
// TRANSMIT_ASYNC indicates that the packet should be sent asynchronously
// if possible, so the KDNET extensibility module will not wait for the
// hardware to send the packet before returning from SendTxPacket.  Support
// for this flag is optional, but can significantly improve performance.
//

#define TRANSMIT_ASYNC 0x80000000

//
// TRANSMIT_HANDLE indicates that this handle is for a transmit packet not for
// a receive packet.  Support for this flag is required.
//

#define TRANSMIT_HANDLE 0x40000000

//
// TRANSMIT_LAST indicates that this is the last packet of the current set of
// packets being sent by kdnic, and can be optionally used by the KDNET
// extensibility module to optimize send performance by allowing it to perform
// certain hardware operations for a set of packets instead of on each one.
// This flag is informational, and use of this flag is optional.
//

#define TRANSMIT_LAST 0x20000000

//
// HANDLE_FLAGS is the full set of possible flags that can be set in a handle.
// Use of this value to clear handle flags to obtain the packet index is
// absolutely required so that any newly added flags get properly removed
// from handles.
//

#define HANDLE_FLAGS (TRANSMIT_ASYNC | TRANSMIT_HANDLE | TRANSMIT_LAST)

//
// The following flags are set by the KDNET extensibility module in the Flags
// field of the KDNET_SHARED_DATA structure to indicate support for optional or
// extended functionality.
//

//
// KDX_EXTENDED_INITIAL_CONNECT causes KDNET to repeatedly attempt the initial
// connection to the target machine by sending an offer packet, for up to 24
// seconds, instead of the default which is just a couple of attempts.  This
// gives the EEM driver on the host, time to get assigned an IP address so
// that it can recieve and respond to the offer packet.  Without this flag
// target machines that are connected over EEM will not usually be able to
// properly break in during KdInitSystem when the kernel break at boot flag is
// set.  Both the KDNET over USB extensibility module, and the KDNET over USB3
// support set this flag, as they both are designed to connect to the EEM
// driver.
//
 
#define KDX_EXTENDED_INITIAL_CONNECT 0x1

//
// KDX_FORCE_DHCP_OFF forces KDNET to not use DHCP when acquiring a target IP
// address.  This is is set when the loadoptions string contains EEM in it
// and KDNET over USB support is being used.  This turns off DHCP regardless of
// the state of the BCD dbgsettings dhcp setting.  This prevents boot from
// taking too long when connecting over EEM when EEM is not bridged to a
// network with DHCP support.  Normally EEM will be local link only, so either
// the BCD dbgsettings dhcp should be set to false or this EEM loadoptions
// string should be set.  Otherwise boot will take at least 50-75 seconds
// longer just waiting for DHCP to time out.
//

#define KDX_FORCE_DHCP_OFF 0x2
#define KDX_VALID_FLAGS (KDX_EXTENDED_INITIAL_CONNECT | KDX_FORCE_DHCP_OFF)

typedef struct _KDNET_SHARED_DATA
{
    PVOID Hardware;
    PDEBUG_DEVICE_DESCRIPTOR Device;
    PUCHAR TargetMacAddress;
    ULONG LinkSpeed;
    ULONG LinkDuplex;
    PUCHAR LinkState;
    ULONG SerialBaudRate;
    ULONG Flags;
    UCHAR RestartKdnet;
    UCHAR Reserved[3];
} KDNET_SHARED_DATA, *PKDNET_SHARED_DATA;