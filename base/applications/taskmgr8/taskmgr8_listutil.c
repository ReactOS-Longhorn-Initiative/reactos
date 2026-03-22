/*
 * PROJECT:     ReactOS — Task Manager 8
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 */

#include "taskmgr8_listutil.h"

#include <commctrl.h>

void
Tm8LvResetContentAndColumns(HWND hLv)
{
    HWND hHdr;
    int n, i;

    if (!hLv)
        return;
    ListView_DeleteAllItems(hLv);
    hHdr = ListView_GetHeader(hLv);
    if (!hHdr)
        return;
    n = Header_GetItemCount(hHdr);
    for (i = n - 1; i >= 0; i--)
        ListView_DeleteColumn(hLv, i);
}
