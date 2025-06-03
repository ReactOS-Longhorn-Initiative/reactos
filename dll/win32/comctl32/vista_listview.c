
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"

#include "wine/debug.h"

#if 0 
HRESULT IListView_GetImageList (INT, HIMAGELIST *);
HRESULT IListView_SetImageList (INT, HIMAGELIST, HIMAGELIST *);
HRESULT IListView_GetBackgroundColor (COLORREF *);
HRESULT IListView_SetBackgroundColor (COLORREF);
HRESULT IListView_GetTextColor (COLORREF *);
HRESULT IListView_SetTextColor (COLORREF);
HRESULT IListView_GetTextBackgroundColor (COLORREF *);
HRESULT IListView_SetTextBackgroundColor (COLORREF);
HRESULT IListView_GetHotLightColor (COLORREF *);
HRESULT IListView_SetHotLightColor (COLORREF);
HRESULT IListView_GetItemCount (int *);
HRESULT IListView_SetItemCount (int, DWORD);
HRESULT IListView_GetItem (LVITEMW *);
HRESULT IListView_SetItem (LVITEMW * const);
HRESULT IListView_GetItemState (int, int, ULONG, ULONG *);
HRESULT IListView_SetItemState (int, int, ULONG, ULONG);
HRESULT IListView_GetItemText (INT, INT, PWSTR, INT);
HRESULT IListView_SetItemText (int, int, PCWSTR);
HRESULT IListView_GetBackgroundImage (LVBKIMAGEW *);
HRESULT IListView_SetBackgroundImage (LVBKIMAGEW * const);
HRESULT IListView_GetFocusedColumn (INT *);
HRESULT IListView_SetSelectionFlags (ULONG, ULONG);
HRESULT IListView_GetSelectedColumn (INT *);
HRESULT IListView_SetSelectedColumn (int);
HRESULT IListView_GetView (DWORD *);
HRESULT IListView_SetView (DWORD);
HRESULT IListView_InsertItem (LVITEMW * const, int *);
HRESULT IListView_DeleteItem (int);
HRESULT IListView_DeleteAllItems (VOID);
HRESULT IListView_UpdateItem (int);
HRESULT IListView_GetItemRect (LVITEMINDEX, int, RECT *);
HRESULT IListView_GetSubItemRect (LVITEMINDEX, INT, INT, RECT *);
HRESULT IListView_HitTestSubItem (LVHITTESTINFO *);
HRESULT IListView_GetIncrSearchString (PWSTR, INT, INT *);
HRESULT IListView_GetItemSpacing (BOOL, INT *, INT *);
HRESULT IListView_SetIconSpacing (INT, INT, INT *, INT *);
HRESULT IListView_GetNextItem (LVITEMINDEX, ULONG, LVITEMINDEX *);
HRESULT IListView_FindItem (LVITEMINDEX, LVFINDINFOW const *, LVITEMINDEX *);
HRESULT IListView_GetSelectionMark (LVITEMINDEX *);
HRESULT IListView_SetSelectionMark (LVITEMINDEX, LVITEMINDEX *);
HRESULT IListView_GetItemPosition (LVITEMINDEX, POINT *);
HRESULT IListView_SetItemPosition (int, POINT const *);
HRESULT IListView_ScrollView (int, int);
HRESULT IListView_EnsureItemVisible (LVITEMINDEX, BOOL);
HRESULT IListView_EnsureSubItemVisible (LVITEMINDEX, INT);
HRESULT IListView_EditSubItem (LVITEMINDEX, INT);
HRESULT IListView_RedrawItems (int, int);
HRESULT IListView_ArrangeItems (int);
HRESULT IListView_RecomputeItems (INT);
HRESULT IListView_GetEditControl (HWND *);
HRESULT IListView_EditLabel (LVITEMINDEX, PCWSTR, HWND *);
HRESULT IListView_EditGroupLabel (INT);
HRESULT IListView_CancelEditLabel (VOID);
HRESULT IListView_GetEditItem (LVITEMINDEX *, INT *);
HRESULT IListView_HitTest (LVHITTESTINFO *);
HRESULT IListView_GetStringWidth (PCWSTR, int *);
HRESULT IListView_GetColumn (int, LVCOLUMNW *);
HRESULT IListView_SetColumn (int, LVCOLUMNW * const);
HRESULT IListView_GetColumnOrderArray (int, int *);
HRESULT IListView_SetColumnOrderArray (int, int const *);
HRESULT IListView_GetHeaderControl (HWND *);
HRESULT IListView_InsertColumn (int, LVCOLUMNW * const, int *);
HRESULT IListView_DeleteColumn (int);
HRESULT IListView_CreateDragImage (int, POINT const *, HIMAGELIST *);
HRESULT IListView_GetViewRect (RECT *);
HRESULT IListView_GetClientRect (BOOL, RECT *);
HRESULT IListView_GetColumnWidth (INT, INT *);
HRESULT IListView_SetColumnWidth (int, int);
HRESULT IListView_GetCallbackMask (ULONG *);
HRESULT IListView_SetCallbackMask (ULONG);
HRESULT IListView_GetTopIndex (int *);
HRESULT IListView_GetCountPerPage (int *);
HRESULT IListView_GetOrigin (POINT *);
HRESULT IListView_GetSelectedCount (INT *);
HRESULT IListView_SortItems (BOOL, LPARAM, PFNLVCOMPARE);
HRESULT IListView_GetExtendedStyle (DWORD *);
HRESULT IListView_SetExtendedStyle (DWORD, DWORD, DWORD *);
HRESULT IListView_GetHoverTime (UINT *);
HRESULT IListView_SetHoverTime (UINT, UINT *);
HRESULT IListView_GetToolTip (HWND *);
HRESULT IListView_SetToolTip (HWND, HWND *);
HRESULT IListView_GetHotItem (LVITEMINDEX *);
HRESULT IListView_SetHotItem (LVITEMINDEX, LVITEMINDEX *);
HRESULT IListView_GetHotCursor (HCURSOR *);
HRESULT IListView_SetHotCursor (HCURSOR, HCURSOR *);
HRESULT IListView_ApproximateViewRect (int, INT *, INT *);
//HRESULT IListView_SetRangeObject (INT, ILVRange *);
HRESULT IListView_GetWorkAreas (int, RECT *);
HRESULT IListView_SetWorkAreas (int, RECT const *);
HRESULT IListView_GetWorkAreaCount (INT *);
HRESULT IListView_ResetEmptyText (VOID);
HRESULT IListView_EnableGroupView (INT);
HRESULT IListView_IsGroupViewEnabled (BOOL *);
HRESULT IListView_SortGroups (PFNLVGROUPCOMPARE, PVOID);
HRESULT IListView_GetGroupInfo (INT, INT, LVGROUP *);
HRESULT IListView_SetGroupInfo (INT, INT, LVGROUP * const);
HRESULT IListView_GetGroupRect (BOOL, int, INT, RECT *);
HRESULT IListView_GetGroupState (INT, ULONG, ULONG *);
HRESULT IListView_HasGroup (int, BOOL *);
HRESULT IListView_InsertGroup (int, LVGROUP * const, int *);
HRESULT IListView_RemoveGroup (INT);
HRESULT IListView_InsertGroupSorted (LVINSERTGROUPSORTED const *, int *);
HRESULT IListView_GetGroupMetrics (LVGROUPMETRICS *);
HRESULT IListView_SetGroupMetrics (LVGROUPMETRICS * const);
HRESULT IListView_RemoveAllGroups (VOID);
HRESULT IListView_GetFocusedGroup (INT *);
HRESULT IListView_GetGroupCount (INT *);
//HRESULT IListView_SetOwnerDataCallback (IOwnerDataCallback *);
HRESULT IListView_GetTileViewInfo (LVTILEVIEWINFO *);
HRESULT IListView_SetTileViewInfo (LVTILEVIEWINFO * const);
HRESULT IListView_GetTileInfo (LVTILEINFO *);
HRESULT IListView_SetTileInfo (LVTILEINFO * const);
HRESULT IListView_GetInsertMark (LVINSERTMARK *);
HRESULT IListView_SetInsertMark (LVINSERTMARK const *);
HRESULT IListView_GetInsertMarkRect (RECT *);
HRESULT IListView_GetInsertMarkColor (COLORREF *);
HRESULT IListView_SetInsertMarkColor (COLORREF, COLORREF *);
HRESULT IListView_HitTestInsertMark (POINT const *, LVINSERTMARK *);
HRESULT IListView_SetInfoTip (LVSETINFOTIP * const);
HRESULT IListView_GetOutlineColor (COLORREF *);
HRESULT IListView_SetOutlineColor (COLORREF, COLORREF *);
HRESULT IListView_GetFrozenItem (INT *);
HRESULT IListView_SetFrozenItem (INT, INT);
HRESULT IListView_GetFrozenSlot (RECT *);
HRESULT IListView_SetFrozenSlot (INT, POINT const *);
HRESULT IListView_GetViewMargin (RECT *);
HRESULT IListView_SetViewMargin (RECT const *);
HRESULT IListView_SetKeyboardSelected (LVITEMINDEX);
HRESULT IListView_MapIndexToId (INT, INT *);
HRESULT IListView_MapIdToIndex (INT, INT *);
HRESULT IListView_IsItemVisible (LVITEMINDEX, BOOL *);
HRESULT IListView_GetGroupSubsetCount (INT *);
HRESULT IListView_SetGroupSubsetCount (INT);
HRESULT IListView_GetVisibleSlotCount (INT *);
HRESULT IListView_GetColumnMargin (RECT *);
HRESULT IListView_SetSubItemCallback (PVOID);
//HRESULT IListView_SetSubItemCallback (ISubItemCallback *);
HRESULT IListView_GetVisibleItemRange (LVITEMINDEX *, LVITEMINDEX *);
HRESULT IListView_SetTypeAheadFlags (UINT, UINT);
#endif

HRESULT
ListView_QueryInterface(LPARAM lParam)
{
    __debugbreak();
    return 0;
}
