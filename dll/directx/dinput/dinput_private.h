
#ifndef __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"
#include "dinputd.h"
#include "wine/list.h"

extern HINSTANCE DINPUT_instance;

/* Implementation specification */
typedef struct IDirectInputImpl IDirectInputImpl;
struct IDirectInputImpl
{
    IDirectInput7A              IDirectInput7A_iface;
    IDirectInput7W              IDirectInput7W_iface;
    IDirectInput8A              IDirectInput8A_iface;
    IDirectInput8W              IDirectInput8W_iface;
    IDirectInputJoyConfig8      IDirectInputJoyConfig8_iface;

    LONG                        ref;

    BOOL                        initialized;
    CRITICAL_SECTION            crit;
    struct list                 entry;          /* entry into list of all IDirectInputs */

    DWORD                       evsequence;     /* unique sequence number for events */
    DWORD                       dwVersion;      /* direct input version number */
    struct list                 devices_list;   /* list of all created dinput devices */
    struct list                 device_players; /* device instance guid to player name */
};

/* Function called by all devices that Wine supports */
struct dinput_device {
    const char *name;
    HRESULT (*enum_deviceA)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id);
    HRESULT (*enum_deviceW)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id);
    HRESULT (*create_device)(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode);
};

struct DevicePlayer {
    GUID instance_guid;
    WCHAR username[MAX_PATH];
    struct list entry;
};

extern const struct dinput_device mouse_device DECLSPEC_HIDDEN;
extern const struct dinput_device keyboard_device DECLSPEC_HIDDEN;
extern const struct dinput_device joystick_linux_device DECLSPEC_HIDDEN;
extern const struct dinput_device joystick_linuxinput_device DECLSPEC_HIDDEN;
extern const struct dinput_device joystick_osx_device DECLSPEC_HIDDEN;

extern void check_dinput_hooks(LPDIRECTINPUTDEVICE8W, BOOL) DECLSPEC_HIDDEN;
extern void check_dinput_events(void) DECLSPEC_HIDDEN;
typedef int (*DI_EVENT_PROC)(LPDIRECTINPUTDEVICE8A, WPARAM, LPARAM);

extern void _copy_diactionformatAtoW(LPDIACTIONFORMATW, LPDIACTIONFORMATA) DECLSPEC_HIDDEN;
extern void _copy_diactionformatWtoA(LPDIACTIONFORMATA, LPDIACTIONFORMATW) DECLSPEC_HIDDEN;

extern HRESULT _configure_devices(IDirectInput8W *iface, LPDICONFIGUREDEVICESCALLBACK lpdiCallback, LPDICONFIGUREDEVICESPARAMSW lpdiCDParams, DWORD dwFlags, LPVOID pvRefData) DECLSPEC_HIDDEN;

extern WCHAR* get_mapping_path(const WCHAR *device, const WCHAR *username) DECLSPEC_HIDDEN;
extern DWORD get_device_type(DWORD version, BOOL is_joystick) DECLSPEC_HIDDEN;

#define IS_DIPROP(x)    (((ULONG_PTR)(x) >> 16) == 0)

#define DIKEYBOARD_MASK    0x81000000
#define DIMOUSE_MASK       0x82000000
#define DIGENRE_ANY        0xFF000000

#endif /* __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H */
