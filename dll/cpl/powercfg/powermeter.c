/*
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powermeter.c
 * PURPOSE:         hibernate tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include "powercfg.h"

static int SelectedBattery = 0;
static HWND hwndDlgDetail = 0;

static
VOID
PowerMeterDetail_UpdateStats(HWND hwndDlg)
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA InfoData;
    DWORD dwIndex;
    DWORD dwSize;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W InterfaceData;
    HANDLE hDevice;
    DWORD dwWait;
    DWORD dwReceived;
    BATTERY_QUERY_INFORMATION bqi = {0};
    WCHAR BatteryName[200];
    WCHAR Buffer[200];

    hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupDiGetClassDevsW failed with %x\n", GetLastError());
        return;
    }

    InfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dwIndex = SelectedBattery;
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, 0, &GUID_DEVCLASS_BATTERY, dwIndex, &InfoData))
    {
        DPRINT1("SetupDiEnumDeviceInterfaces failed with %x\n", GetLastError());
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return;
    }

    dwSize = 0;
    InterfaceData = NULL;
    if (!SetupDiGetInterfaceDeviceDetailW(hDevInfo, &InfoData, InterfaceData, dwSize, &dwSize, NULL))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            DPRINT1("SetupDiGetInterfaceDeviceDetailW failed with %x\n", GetLastError());
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return;
        }
    }
    InterfaceData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!InterfaceData)
    {
        DPRINT1("HeapAlloc failed with %x\n", GetLastError());
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return;
    }
    InterfaceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if (!SetupDiGetInterfaceDeviceDetailW(hDevInfo, &InfoData, InterfaceData, dwSize, &dwSize, NULL))
    {
        DPRINT1("SetupDiGetInterfaceDeviceDetailW failed with %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, InterfaceData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return;
    }

    DPRINT1("Opening file %S\n", InterfaceData->DevicePath);
    hDevice = CreateFileW(
        InterfaceData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        DPRINT1("CreateFileW failed with %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, InterfaceData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return;
    }

    dwWait = 0;
    if (DeviceIoControl(
            hDevice, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(DWORD), &bqi.BatteryTag, sizeof(bqi.BatteryTag),
            &dwReceived,
            NULL))
    {
        BATTERY_INFORMATION bi = {0};
        BATTERY_WAIT_STATUS bws = {0};

        bqi.InformationLevel = BatteryDeviceName;
        if (DeviceIoControl(
                hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), BatteryName, sizeof(BatteryName),
                &dwReceived,
                NULL))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYNAME, BatteryName);
        }

        bqi.InformationLevel = BatteryUniqueID;
        if (DeviceIoControl(
                hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), BatteryName, sizeof(BatteryName),
                &dwReceived, NULL))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYUNIQUEID, BatteryName);
        }

        bqi.InformationLevel = BatteryInformation;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi, sizeof(bi), &dwReceived, NULL))
        {
            CHAR Chem[5];
            memset(Chem, 0x0, sizeof(Chem));
            RtlCopyMemory(Chem, bi.Chemistry, sizeof(bi.Chemistry));
            SetDlgItemTextA(hwndDlg, IDC_BATTERYCHEMISTRY, Chem);
        }

        bws.BatteryTag = bqi.BatteryTag;
        BATTERY_STATUS bs;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws), &bs, sizeof(bs), &dwReceived, NULL))
        {
            WCHAR Status[200];
            Status[0] = UNICODE_NULL;
            if (bs.PowerState & BATTERY_POWER_ON_LINE)
            {
                if (LoadString(hApplet, IDS_ONLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
                {
                    wcscpy(Status, Buffer);
                }
            }
            if (bs.PowerState & BATTERY_CHARGING)
            {
                if (LoadString(hApplet, IDS_CHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
                {
                    if (Status[0] != UNICODE_NULL)
                    {
                        wcscat(Status, L", ");
                    }
                    wcscat(Status, Buffer);
                }
            }
            if (bs.PowerState & BATTERY_DISCHARGING)
            {
                if (LoadString(hApplet, IDS_DISCHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
                {
                    if (Status[0] != UNICODE_NULL)
                    {
                        wcscat(Status, L", ");
                    }
                    wcscat(Status, Buffer);
                }
            }
            // TODO BATTERY_CRITICAL
            SetDlgItemTextW(hwndDlg, IDC_BATTERYPOWERSTATE, Status);
        }

        bqi.InformationLevel = BatteryManufactureName;
        if (DeviceIoControl(
                hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), BatteryName, sizeof(BatteryName),
                &dwReceived, NULL))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYMANUFACTURER, BatteryName);
        }
    }
    HeapFree(GetProcessHeap(), 0, InterfaceData);
    SetupDiDestroyDeviceInfoList(hDevInfo);
}


static
VOID
PowerMeterDetail_InitDialog(HWND hwndDlg)
{
    WCHAR FormatBuffer[200];
    WCHAR Buffer[200];

    if (LoadString(hApplet, IDS_DETAILEDBATTERY, FormatBuffer, sizeof(Buffer) / sizeof(WCHAR)))
    {
        wsprintf(Buffer, FormatBuffer, SelectedBattery);
        SetWindowTextW(hwndDlg, Buffer);
    }   
    PowerMeterDetail_UpdateStats(hwndDlg);
}

INT_PTR
CALLBACK
PowerMeterDetailDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PowerMeterDetail_InitDialog(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_REFRESH)
            {
                PowerMeterDetail_InitDialog(hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDOK)
            {
                DestroyWindow(hwndDlgDetail);
                hwndDlgDetail = 0;
                return TRUE;
            }

    }
    return FALSE;
}


static
VOID
PowerMeter_InitDialog(HWND hwndDlg)
{
    SYSTEM_POWER_STATUS sps;
    WCHAR Buffer[200];

    if (GetSystemPowerStatus(&sps))
    {
        if (sps.ACLineStatus)
        {
            if (LoadString(hApplet, IDS_ONLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
            {
                SetDlgItemTextW(hwndDlg, IDC_POWERSTATUS, Buffer);
            }          
        }
        else
        {
            if (LoadString(hApplet, IDS_OFFLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
            {
                SetDlgItemTextW(hwndDlg, IDC_POWERSTATUS, Buffer);
            }
        }
    }
    if (sps.BatteryFlag & 8)
    {
        if (LoadString(hApplet, IDS_CHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING, Buffer);
        }
    }
    wsprintf(Buffer, L"%d", sps.BatteryLifePercent);
    SetDlgItemTextW(hwndDlg, IDC_BATTERYPERCENT, Buffer);
}

/* Property page dialog callback */
INT_PTR
CALLBACK
PowerMeterDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PowerMeter_InitDialog(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDI_BATTERYDETAIL)
            {
                if (!IsWindow(hwndDlgDetail))
                {
                    hwndDlgDetail =
                        CreateDialog(hApplet, MAKEINTRESOURCE(IDD_POWERMETERDETAILS), hwndDlg, PowerMeterDetailDlgProc);
                    ShowWindow(hwndDlgDetail, SW_SHOW); 
                }

            }
            break;
    }
    return FALSE;
}
