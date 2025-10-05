/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */


#include "setupapi_private.h"
#include "wine/list.h"
 typedef GUID  DEVPROPGUID, *PDEVPROPGUID;
typedef ULONG DEVPROPID,   *PDEVPROPID;
typedef ULONG DEVPROPTYPE, *PDEVPROPTYPE;

//
// Property data types.
//
#define DEVPROP_TYPE_EMPTY                      0x00000000  // nothing, no property data
#define DEVPROP_TYPE_NULL                       0x00000001  // null property data
#define DEVPROP_TYPE_SBYTE                      0x00000002  // 8-bit signed int (SBYTE)
#define DEVPROP_TYPE_BYTE                       0x00000003  // 8-bit unsigned int (BYTE)
#define DEVPROP_TYPE_INT16                      0x00000004  // 16-bit signed int (SHORT)
#define DEVPROP_TYPE_UINT16                     0x00000005  // 16-bit unsigned int (USHORT)
#define DEVPROP_TYPE_INT32                      0x00000006  // 32-bit signed int (LONG)
#define DEVPROP_TYPE_UINT32                     0x00000007  // 32-bit unsigned int (ULONG)
#define DEVPROP_TYPE_INT64                      0x00000008  // 64-bit signed int (LONG64)
#define DEVPROP_TYPE_UINT64                     0x00000009  // 64-bit unsigned int (ULONG64)
#define DEVPROP_TYPE_FLOAT                      0x0000000A  // 32-bit floating-point (FLOAT)
#define DEVPROP_TYPE_DOUBLE                     0x0000000B  // 64-bit floating-point (DOUBLE)
#define DEVPROP_TYPE_DECIMAL                    0x0000000C  // 128-bit data (DECIMAL)
#define DEVPROP_TYPE_GUID                       0x0000000D  // 128-bit unique identifier (GUID)
#define DEVPROP_TYPE_CURRENCY                   0x0000000E  // 64 bit signed int currency value (CURRENCY)
#define DEVPROP_TYPE_DATE                       0x0000000F  // date (DATE)
#define DEVPROP_TYPE_FILETIME                   0x00000010  // file time (FILETIME)
#define DEVPROP_TYPE_BOOLEAN                    0x00000011  // 8-bit boolean (DEVPROP_BOOLEAN)
#define DEVPROP_TYPE_STRING                     0x00000012  // null-terminated string
#define DEVPROP_TYPE_STRING_LIST (DEVPROP_TYPE_STRING|DEVPROP_TYPEMOD_LIST) // multi-sz string list
#define DEVPROP_TYPE_SECURITY_DESCRIPTOR        0x00000013  // self-relative binary SECURITY_DESCRIPTOR
#define DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING 0x00000014  // security descriptor string (SDDL format)
#define DEVPROP_TYPE_DEVPROPKEY                 0x00000015  // device property key (DEVPROPKEY)
#define DEVPROP_TYPE_DEVPROPTYPE                0x00000016  // device property type (DEVPROPTYPE)
#define DEVPROP_TYPE_BINARY      (DEVPROP_TYPE_BYTE|DEVPROP_TYPEMOD_ARRAY)  // custom binary data
#define DEVPROP_TYPE_ERROR                      0x00000017  // 32-bit Win32 system error code
#define DEVPROP_TYPE_NTSTATUS                   0x00000018  // 32-bit NTSTATUS code
#define DEVPROP_TYPE_STRING_INDIRECT            0x00000019  // string resource (@[path\]<dllname>,-<strId>)
// Max base DEVPROP_TYPE_ and DEVPROP_TYPEMOD_ values.
//
#define MAX_DEVPROP_TYPE                        0x00000019  // max valid DEVPROP_TYPE_ value
#define MAX_DEVPROP_TYPEMOD                     0x00002000  // max valid DEVPROP_TYPEMOD_ value
#define DEVPROP_TYPEMOD_ARRAY                   0x00001000  // array of fixed-sized data elements
#define DEVPROP_TYPEMOD_LIST                    0x00002000  // list of variable-sized data elements

//
// Bitmasks for extracting DEVPROP_TYPE_ and DEVPROP_TYPEMOD_ values.
//
#define DEVPROP_MASK_TYPE                       0x00000FFF  // range for base DEVPROP_TYPE_ values
#define DEVPROP_MASK_TYPEMOD                    0x0000F000  // mask for DEVPROP_TYPEMOD_ type modifiers

struct device
{
    struct DeviceInfoSet *set;
    HKEY                  key;
    BOOL                  phantom;
    WCHAR                *instanceId;
    struct list           interfaces;
    GUID                  class;
    DEVINST               devnode;
    struct list           entry;
    BOOL                  removed;
    SP_DEVINSTALL_PARAMS_W params;
    struct driver        *drivers;
    unsigned int          driver_count;
    struct driver        *selected_driver;
};

static struct device **devnode_table;
static unsigned int devnode_table_size;
 
static void SETUPDI_GuidToString(const GUID *guid, LPWSTR guidStr)
{
    static const WCHAR fmt[] = {'{','%','0','8','X','-','%','0','4','X','-',
        '%','0','4','X','-','%','0','2','X','%','0','2','X','-','%','0','2',
        'X','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X','%',
        '0','2','X','}',0};

    swprintf(
	guidStr,
	fmt, guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static BOOL SETUPDI_StringToGuid(LPCWSTR guidStr, GUID *guid)
{
    /* Parse GUID in format: {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx} */
    return swscanf(guidStr, L"{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                   &guid->Data1, &guid->Data2, &guid->Data3,
                   &guid->Data4[0], &guid->Data4[1], &guid->Data4[2], &guid->Data4[3],
                   &guid->Data4[4], &guid->Data4[5], &guid->Data4[6], &guid->Data4[7]) == 11;
}

static struct device *get_devnode_device(DEVINST devnode)
{
    if (devnode < devnode_table_size)
        return devnode_table[devnode];

    WARN("device node %u not found\n", devnode);
    return NULL;
}

static LSTATUS get_device_property(struct device *device,
                                    const DEVPROPKEY *prop_key,
                                    DEVPROPTYPE *prop_type,
                                     BYTE *prop_buff,
                                     DWORD prop_buff_size,
                                     DWORD *required_size,
                                     DWORD flags)
{
    WCHAR key_path[55] = L"Properties\\";
    HKEY hkey;
    DWORD value_type;
    DWORD value_size = 0;
    LSTATUS ls;

    if (!prop_key)
        return ERROR_INVALID_DATA;

    if (!prop_type || (!prop_buff && prop_buff_size))
        return ERROR_INVALID_USER_BUFFER;

    if (flags)
        return ERROR_INVALID_FLAGS;

    SETUPDI_GuidToString(&prop_key->fmtid, key_path + 11);
    swprintf(key_path + 49,L"\\%04X", prop_key->pid);

    ls = RegOpenKeyExW(device->key, key_path, 0, KEY_QUERY_VALUE, &hkey);
    if (!ls)
    {
        value_size = prop_buff_size;
        ls = RegQueryValueExW(hkey, NULL, NULL, &value_type, prop_buff, &value_size);
        RegCloseKey(hkey);
    }

    switch (ls)
    {
    case NO_ERROR:
    case ERROR_MORE_DATA:
        *prop_type = 0xffff & value_type;
        ls = (ls == ERROR_MORE_DATA || !prop_buff) ? ERROR_INSUFFICIENT_BUFFER : NO_ERROR;
        break;
    case ERROR_FILE_NOT_FOUND:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        ls = ERROR_NOT_FOUND;
        break;
    default:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        FIXME("Unhandled error %#x\n", ls);
        break;
    }

    if (required_size)
        *required_size = value_size;

    return ls;
}

static struct DeviceInfoSet *get_device_set(HDEVINFO devinfo)
{
    struct DeviceInfoSet *set = devinfo;

    if (!devinfo || devinfo == INVALID_HANDLE_VALUE || set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    return set;
}

static struct device *get_device(HDEVINFO devinfo, const SP_DEVINFO_DATA *data)
{
    struct DeviceInfoSet *set;
    struct device *device;

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!data || data->cbSize != sizeof(*data) || !data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    device = (struct device *)data->Reserved;

    if (device->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (device->removed)
    {
        SetLastError(ERROR_NO_SUCH_DEVINST);
        return NULL;
    }

    return device;
}

static BOOL is_valid_property_type(DEVPROPTYPE prop_type)
{
    DWORD type = prop_type & DEVPROP_MASK_TYPE;
    DWORD typemod = prop_type & DEVPROP_MASK_TYPEMOD;

    if (type > MAX_DEVPROP_TYPE)
        return FALSE;
    if (typemod > MAX_DEVPROP_TYPEMOD)
        return FALSE;

    if (typemod == DEVPROP_TYPEMOD_ARRAY
        && (type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL || type == DEVPROP_TYPE_STRING
            || type == DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING))
        return FALSE;

    if (typemod == DEVPROP_TYPEMOD_LIST
        && !(type == DEVPROP_TYPE_STRING || type == DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING))
        return FALSE;

    return TRUE;
}

/***********************************************************************
 *              CM_Get_DevNode_Property_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Property_ExW(DEVINST devnode, const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type,
    BYTE *prop_buff, ULONG *prop_buff_size, ULONG flags, HMACHINE machine)
{
    struct device *device = get_devnode_device(devnode);
    LSTATUS ls;

    TRACE("%u, %p, %p, %p, %p, %#x, %p\n", devnode, prop_key, prop_type, prop_buff, prop_buff_size,
          flags, machine);

    if (machine)
        return CR_MACHINE_UNAVAILABLE;

    if (!device)
        return CR_NO_SUCH_DEVINST;

    if (!prop_buff_size)
        return CR_INVALID_POINTER;

    ls = get_device_property(device, prop_key, prop_type, prop_buff, *prop_buff_size, prop_buff_size, flags);
    switch (ls)
    {
    case NO_ERROR:
        return CR_SUCCESS;
    case ERROR_INVALID_DATA:
        return CR_INVALID_DATA;
    case ERROR_INVALID_USER_BUFFER:
        return CR_INVALID_POINTER;
    case ERROR_INVALID_FLAGS:
        return CR_INVALID_FLAG;
    case ERROR_INSUFFICIENT_BUFFER:
        return CR_BUFFER_SMALL;
    case ERROR_NOT_FOUND:
        return CR_NO_SUCH_VALUE;
    }
    return CR_FAILURE;
}

/***********************************************************************
 *              CM_Get_DevNode_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_PropertyW(DEVINST dev, const DEVPROPKEY *key, DEVPROPTYPE *type,
    PVOID buf, PULONG len, ULONG flags)
{
    return CM_Get_DevNode_Property_ExW(dev, key, type, buf, len, flags, NULL);
}

BOOL WINAPI SetupDiSetDevicePropertyW(HDEVINFO devinfo, PSP_DEVINFO_DATA device_data, const DEVPROPKEY *key,
                                      DEVPROPTYPE type, const BYTE *buffer, DWORD size, DWORD flags)
{
    static const WCHAR propertiesW[] = {'P', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's', 0};
    static const WCHAR formatW[] = {'\\', '%', '0', '4', 'X', 0};
    struct device *device;
    HKEY properties_hkey, property_hkey;
    WCHAR property_hkey_path[44];
    LSTATUS ls;

    TRACE("%p %p %p %#x %p %d %#x\n", devinfo, device_data, key, type, buffer, size, flags);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!key || !is_valid_property_type(type)
        || (buffer && !size && !(type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL))
        || (buffer && size && (type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL)))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (size && !buffer)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ls = RegCreateKeyExW(device->key, propertiesW, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &properties_hkey, NULL);
    if (ls)
    {
        SetLastError(ls);
        return FALSE;
    }

    SETUPDI_GuidToString(&key->fmtid, property_hkey_path);
    swprintf(property_hkey_path + 38, formatW, key->pid);

    if (type == DEVPROP_TYPE_EMPTY)
    {
        ls = RegDeleteKeyW(properties_hkey, property_hkey_path);
        RegCloseKey(properties_hkey);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else if (type == DEVPROP_TYPE_NULL)
    {
        if (!(ls = RegOpenKeyW(properties_hkey, property_hkey_path, &property_hkey)))
        {
            ls = RegDeleteValueW(property_hkey, NULL);
            RegCloseKey(property_hkey);
        }

        RegCloseKey(properties_hkey);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else
    {
        if (!(ls = RegCreateKeyExW(properties_hkey, property_hkey_path, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                                  &property_hkey, NULL)))
        {
            ls = RegSetValueExW(property_hkey, NULL, 0, 0xffff0000 | (0xffff & type), buffer, size);
            RegCloseKey(property_hkey);
        }

        RegCloseKey(properties_hkey);
        SetLastError(ls);
        return !ls;
    }
}

/***********************************************************************
 *              SetupDiGetDevicePropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDevicePropertyW(HDEVINFO devinfo, PSP_DEVINFO_DATA device_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type, BYTE *prop_buff,
                DWORD prop_buff_size, DWORD *required_size, DWORD flags)
{
    struct device *device;
    LSTATUS ls;

    TRACE("%p, %p, %p, %p, %p, %d, %p, %#x\n", devinfo, device_data, prop_key, prop_type, prop_buff, prop_buff_size,
          required_size, flags);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    ls = get_device_property(device, prop_key, prop_type, prop_buff, prop_buff_size, required_size, flags);

    SetLastError(ls);
    return !ls;
}

/***********************************************************************
 *              SetupDiGetDeviceInterfacePropertyKeys (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfacePropertyKeys(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA interface_data,
                DEVPROPKEY *prop_key_array, DWORD prop_key_count, DWORD *required_count, DWORD flags)
{
    struct DeviceInfoSet *set;
    HKEY interface_key = NULL, properties_key = NULL;
    WCHAR key_path[55] = L"Properties";
    DWORD i, max_subkey_len, subkey_count;
    LSTATUS ls = ERROR_SUCCESS;
    BOOL ret = FALSE;

    TRACE("%p, %p, %p, %d, %p, %#x\n", devinfo, interface_data, prop_key_array, prop_key_count,
          required_count, flags);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!interface_data || interface_data->cbSize != sizeof(*interface_data) || !interface_data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    /* Open the device interface registry key */
    interface_key = SetupDiOpenDeviceInterfaceRegKey(devinfo, interface_data, 0, KEY_READ);
    if (interface_key == INVALID_HANDLE_VALUE)
        return FALSE;

    /* Open the Properties subkey */
    ls = RegOpenKeyExW(interface_key, key_path, 0, KEY_ENUMERATE_SUB_KEYS, &properties_key);
    if (ls == ERROR_FILE_NOT_FOUND)
    {
        /* No properties, return success with count 0 */
        if (required_count)
            *required_count = 0;
        ret = TRUE;
        goto cleanup;
    }
    if (ls != ERROR_SUCCESS)
    {
        SetLastError(ls);
        goto cleanup;
    }

    /* Get information about the properties subkeys */
    ls = RegQueryInfoKeyW(properties_key, NULL, NULL, NULL, &subkey_count, &max_subkey_len,
                         NULL, NULL, NULL, NULL, NULL, NULL);
    if (ls != ERROR_SUCCESS)
    {
        SetLastError(ls);
        goto cleanup;
    }

    if (required_count)
        *required_count = subkey_count;

    if (!prop_key_array || !prop_key_count)
    {
        ret = TRUE;
        goto cleanup;
    }

    if (prop_key_count < subkey_count)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto cleanup;
    }

    /* Enumerate the property keys */
    for (i = 0; i < subkey_count; i++)
    {
        WCHAR subkey_name[MAX_GUID_STRING_LEN + 5]; /* GUID + \\ + 4 hex digits + null */
        DWORD name_len = sizeof(subkey_name) / sizeof(WCHAR);
        WCHAR guid_str[MAX_GUID_STRING_LEN];
        GUID guid;
        DWORD pid;

        ls = RegEnumKeyExW(properties_key, i, subkey_name, &name_len, NULL, NULL, NULL, NULL);
        if (ls != ERROR_SUCCESS)
            break;

        /* Parse the subkey name as GUID\PID format */
        if (swscanf(subkey_name, L"%36[^\\]\\%04X", guid_str, &pid) != 2)
        {
            WARN("Invalid property key format: %s\n", debugstr_w(subkey_name));
            continue;
        }

        /* Convert GUID string back to GUID */
        if (!SETUPDI_StringToGuid(guid_str, &guid))
        {
            WARN("Failed to parse GUID: %s\n", debugstr_w(guid_str));
            continue;
        }

        prop_key_array[i].fmtid = guid;
        prop_key_array[i].pid = pid;
    }

    if (ls == ERROR_SUCCESS)
        ret = TRUE;
    else
        SetLastError(ls);

cleanup:
    if (properties_key)
        RegCloseKey(properties_key);
    if (interface_key)
        RegCloseKey(interface_key);

    return ret;
}

/***********************************************************************
 *              SetupDiGetDeviceInterfacePropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfacePropertyW(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA interface_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type, BYTE *prop_buff,
                DWORD prop_buff_size, DWORD *required_size, DWORD flags)
{
    struct DeviceInfoSet *set;
    HKEY interface_key = NULL, property_key = NULL;
    WCHAR key_path[55] = L"Properties\\";
    DWORD value_type, value_size = 0;
    LSTATUS ls;

    TRACE("%p, %p, %p, %p, %p, %d, %p, %#x\n", devinfo, interface_data, prop_key, prop_type, prop_buff,
          prop_buff_size, required_size, flags);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!interface_data || interface_data->cbSize != sizeof(*interface_data) || !interface_data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!prop_key)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (!prop_type || (!prop_buff && prop_buff_size))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    /* Open the device interface registry key */
    interface_key = SetupDiOpenDeviceInterfaceRegKey(devinfo, interface_data, 0, KEY_READ);
    if (interface_key == INVALID_HANDLE_VALUE)
        return FALSE;

    /* Build the property key path */
    SETUPDI_GuidToString(&prop_key->fmtid, key_path + 11);
    swprintf(key_path + 49, L"\\%04X", prop_key->pid);

    /* Open the property key */
    ls = RegOpenKeyExW(interface_key, key_path, 0, KEY_QUERY_VALUE, &property_key);
    if (ls != ERROR_SUCCESS)
    {
        if (ls == ERROR_FILE_NOT_FOUND)
        {
            *prop_type = DEVPROP_TYPE_EMPTY;
            if (required_size)
                *required_size = 0;
            SetLastError(ERROR_NOT_FOUND);
            RegCloseKey(interface_key);
            return FALSE;
        }
        SetLastError(ls);
        RegCloseKey(interface_key);
        return FALSE;
    }

    /* Query the property value */
    value_size = prop_buff_size;
    ls = RegQueryValueExW(property_key, NULL, NULL, &value_type, prop_buff, &value_size);

    switch (ls)
    {
    case NO_ERROR:
    case ERROR_MORE_DATA:
        *prop_type = 0xffff & value_type;
        ls = (ls == ERROR_MORE_DATA || !prop_buff) ? ERROR_INSUFFICIENT_BUFFER : NO_ERROR;
        break;
    case ERROR_FILE_NOT_FOUND:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        ls = ERROR_NOT_FOUND;
        break;
    default:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        FIXME("Unhandled error %#x\n", ls);
        break;
    }

    if (required_size)
        *required_size = value_size;

    RegCloseKey(property_key);
    RegCloseKey(interface_key);

    SetLastError(ls);
    return !ls;
}

/***********************************************************************
 *              SetupDiSetDeviceInterfacePropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInterfacePropertyW(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA interface_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE prop_type, const BYTE *prop_buff,
                DWORD prop_buff_size, DWORD flags)
{
    static const WCHAR propertiesW[] = {'P', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's', 0};
    static const WCHAR formatW[] = {'\\', '%', '0', '4', 'X', 0};
    struct DeviceInfoSet *set;
    HKEY interface_key = NULL, properties_key = NULL, property_key = NULL;
    WCHAR property_key_path[44];
    LSTATUS ls;

    TRACE("%p, %p, %p, %#x, %p, %d, %#x\n", devinfo, interface_data, prop_key, prop_type, prop_buff,
          prop_buff_size, flags);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!interface_data || interface_data->cbSize != sizeof(*interface_data) || !interface_data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!prop_key || !is_valid_property_type(prop_type)
        || (prop_buff && !prop_buff_size && !(prop_type == DEVPROP_TYPE_EMPTY || prop_type == DEVPROP_TYPE_NULL))
        || (prop_buff && prop_buff_size && (prop_type == DEVPROP_TYPE_EMPTY || prop_type == DEVPROP_TYPE_NULL)))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (prop_buff_size && !prop_buff)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    /* Open the device interface registry key */
    interface_key = SetupDiOpenDeviceInterfaceRegKey(devinfo, interface_data, 0, KEY_READ | KEY_WRITE);
    if (interface_key == INVALID_HANDLE_VALUE)
        return FALSE;

    /* Create/open the Properties key */
    ls = RegCreateKeyExW(interface_key, propertiesW, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &properties_key, NULL);
    if (ls)
    {
        SetLastError(ls);
        RegCloseKey(interface_key);
        return FALSE;
    }

    /* Build the property key path */
    SETUPDI_GuidToString(&prop_key->fmtid, property_key_path);
    swprintf(property_key_path + 38, formatW, prop_key->pid);

    if (prop_type == DEVPROP_TYPE_EMPTY)
    {
        /* Delete the property */
        ls = RegDeleteKeyW(properties_key, property_key_path);
        RegCloseKey(properties_key);
        RegCloseKey(interface_key);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else if (prop_type == DEVPROP_TYPE_NULL)
    {
        /* Set to NULL (delete the value) */
        if (!(ls = RegCreateKeyExW(properties_key, property_key_path, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                                  &property_key, NULL)))
        {
            ls = RegDeleteValueW(property_key, NULL);
            RegCloseKey(property_key);
        }

        RegCloseKey(properties_key);
        RegCloseKey(interface_key);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else
    {
        /* Set the property value */
        if (!(ls = RegCreateKeyExW(properties_key, property_key_path, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                                  &property_key, NULL)))
        {
            ls = RegSetValueExW(property_key, NULL, 0, 0xffff0000 | (0xffff & prop_type), prop_buff, prop_buff_size);
            RegCloseKey(property_key);
        }

        RegCloseKey(properties_key);
        RegCloseKey(interface_key);
        SetLastError(ls);
        return !ls;
    }
}

/***********************************************************************
 *              SetupDiGetDeviceInterfacePropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfacePropertyA(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA interface_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type, PBYTE prop_buff,
                DWORD prop_buff_size, PDWORD required_size, DWORD flags)
{
    FIXME("ANSI version not implemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiSetDeviceInterfacePropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInterfacePropertyA(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA interface_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE prop_type, const PBYTE prop_buff,
                DWORD prop_buff_size, DWORD flags)
{
    FIXME("ANSI version not implemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
