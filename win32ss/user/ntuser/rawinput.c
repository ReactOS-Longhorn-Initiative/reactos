 
#include <win32k.h>
#include <debug.h>

/* it seems like reactos only can handle one input device at a time due to driver limitations? */
extern HANDLE ghKeyboardDevice;
extern HANDLE ghMouseDevice;
BOOLEAN RawInputEnabled = FALSE;
typedef struct _RIDDevice
{
    HANDLE file;
    RID_DEVICE_INFO info;
    DWORD Type; // RIM_TYPEMOUSE or RIM_TYPEKEYBOARD
} RIDDevice;

RIDDevice RIDDevices[2] = {
    { NULL, {0}, 0 }, // Mouse
    { NULL, {0}, 0 }  // Keyboard
};

VOID
APIENTRY
HandleDeviceEnumeration(DWORD type)
{
    static const RID_DEVICE_INFO_KEYBOARD keyboard_info = {0, 0, 1, 12, 3, 101};
    static const RID_DEVICE_INFO_MOUSE mouse_info = {1, 5, 0, FALSE};
    RID_DEVICE_INFO info;
    RtlZeroMemory( &info, sizeof(info) );
    info.cbSize = sizeof(info);
    info.dwType = type;

    switch (type)
    {
        case RIM_TYPEMOUSE:
            info.mouse = mouse_info;
            RIDDevices[0].info = info; // Store mouse info
            RIDDevices[0].file = ghMouseDevice; // Store mouse handle
            RIDDevices[0].Type = RIM_TYPEMOUSE; // Store mouse type

            break;
        case RIM_TYPEKEYBOARD:
            info.keyboard = keyboard_info;
            RIDDevices[1].info = info; // Store keyboard info
            RIDDevices[1].file = ghKeyboardDevice; // Store keyboard handle
            RIDDevices[1].Type = RIM_TYPEKEYBOARD; // Store keyboard type
            break;
        default:
        {
            DPRINT1("Unknown device type %d\n", type);
            __debugbreak();
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return;
        }
    }
}

/* This file is pretty deeply inspired by how wine seems to do it, but wine can't be DIRECTLY used. */
DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    if (cbSizeHeader < sizeof(RAWINPUTHEADER) || !pData || !pcbSize)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    // the return value is the number of RAWINPUT structures written to pData.
    __debugbreak();
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    __debugbreak();
    PRAWINPUT RawInput = (PRAWINPUT)hRawInput;
    ULONG OutputDataSize = sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) - sizeof(*pcbSize);


    DPRINT1("NtUserGetRawInputData: hRawInput %p, uiCommand %u, pData %p, pcbSize %p, cbSizeHeader %u\n",
           hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

    if (!pData)
    {
        *pcbSize = OutputDataSize;
        EngSetLastError(ERROR_SUCCESS);
        return 0; // Return the size needed.
    }
    DPRINT1("Input data size: %d\n", *pcbSize);

    RtlCopyMemory(pData, RawInput, sizeof(PRAWINPUT));
    return RawInput->header.dwSize;
}

RIDDevice
APIENTRY
GetDeviceFromHandle(HANDLE hDevice)
{
    for(int i = 0; i < 2; i++)
    {
        if (RIDDevices[i].file == hDevice)
        {
            return RIDDevices[i];
        }
    }

    return (RIDDevice){ NULL, {0}, 0 }; // Return an empty RIDDevice if not found.
}

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
)
{
    DWORD len, data_len;
    len = data_len = *pcbSize;
    RIDDevice RIDDevicesLoc = GetDeviceFromHandle(hDevice);
    RID_DEVICE_INFO info;
    switch (uiCommand)
    {
    case RIDI_DEVICENAME:
        __debugbreak();
        break;

    case RIDI_DEVICEINFO:
        if ((len = sizeof(info)) <= data_len && pData)
            memcpy( pData, &RIDDevicesLoc.info, len );
        *pcbSize = len;
        break;
    default:
        __debugbreak();
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UINT DeviceCount = 0; // Number of valid RIT Devices.
    if (cbSize < sizeof(RAWINPUTDEVICELIST))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }

    if (!puiNumDevices)
    {
        EngSetLastError(ERROR_NOACCESS);
        return ~0u;
    }

    // Manually enumerate the device list.
    HandleDeviceEnumeration(RIM_TYPEMOUSE);
    HandleDeviceEnumeration(RIM_TYPEKEYBOARD);
 
    /* Hacky parse */
    if (RIDDevices[0].file)
    {
        if (*puiNumDevices < ++DeviceCount || !pRawInputDeviceList) goto skip;
        DeviceCount++;
        pRawInputDeviceList->hDevice = RIDDevices[0].file;
        pRawInputDeviceList->dwType = RIDDevices[0].Type;
        pRawInputDeviceList++;
    }

    if (RIDDevices[1].file)
    {
        if (*puiNumDevices < ++DeviceCount || !pRawInputDeviceList) goto skip;
        DeviceCount++;
        pRawInputDeviceList->hDevice = RIDDevices[1].file;
        pRawInputDeviceList->dwType = RIDDevices[1].Type;
        pRawInputDeviceList++;
    }

skip:
    if (!pRawInputDeviceList)
    {
        *puiNumDevices = DeviceCount;
        EngSetLastError(ERROR_SUCCESS);
        return 0;
    }

    return 0;
}

DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    STUB;
     __debugbreak();
    return 0;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    RawInputEnabled = TRUE;
    return TRUE;
}
