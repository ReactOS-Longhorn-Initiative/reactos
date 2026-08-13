/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Desktop composition registration entry points
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The four exports dwm.exe uses to attach itself to win32k. Verified against
 * Vista SP1's user32.dll, where each is a bare syscall stub -- there is no
 * user-mode logic in any of them, so these are thin by design, not by omission:
 *
 *   RegisterSessionPort     syscall 0x1270, 1 arg   (b8 70 12 .. c2 04 00)
 *   UnregisterSessionPort   syscall 0x1271, 0 args  (b8 71 12 .. c3)
 *   DwmStartRedirection     syscall 0x1273, 1 arg   (b8 73 12 .. c2 04 00)
 *   DwmStopRedirection      syscall 0x1274, 0 args  (b8 74 12 .. c3)
 *
 * Vista's dwm.exe calls them as RegisterSessionPort(m_hPort) -- the LPC port
 * HANDLE, not its name (dwm.exe.c:9451) -- and
 * DwmStartRedirection(!fStructuralMode) (dwm.exe.c:5421).
 */

#include <user32.h>

/*
 * @implemented
 *
 * Hand win32k the DWM session port. win32k references the handle and posts
 * redirection commands to it; see win32ss/user/ntuser/dwm.c.
 */
BOOL WINAPI
RegisterSessionPort(HANDLE hPort)
{
    return NtUserRegisterSessionPort(hPort);
}

/*
 * @implemented
 */
BOOL WINAPI
UnregisterSessionPort(VOID)
{
    return NtUserUnregisterSessionPort();
}

/*
 * @implemented
 *
 * fRedirectContent is Vista's !fStructuralMode. FALSE selects structural mode:
 * sprite geometry only, no window content. That is all win32k can currently
 * produce -- redirection surfaces do not exist yet -- so TRUE is accepted and
 * recorded but behaves the same.
 */
BOOL WINAPI
DwmStartRedirection(BOOL fRedirectContent)
{
    return NtUserDwmStartRedirection(fRedirectContent);
}

/*
 * @implemented
 */
BOOL WINAPI
DwmStopRedirection(VOID)
{
    return NtUserDwmStopRedirection();
}

/* EOF */
