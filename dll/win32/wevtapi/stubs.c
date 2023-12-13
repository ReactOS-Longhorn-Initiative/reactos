#include "KB.h"
#include "wine/list.h"
#include "wine/debug.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wevtapi);

static const WCHAR log_pathW[] = L"C:\\windows\\temp\\evt.log";

EVT_HANDLE WINAPI EvtOpenSession(EVT_LOGIN_CLASS login_class, void *login, DWORD timeout, DWORD flags)
{
    //("(%u %p %lu %lu) stub\n", login_class, login, timeout, flags);
    return NULL;
}

EVT_HANDLE WINAPI EvtOpenLog(EVT_HANDLE session, const WCHAR *path, DWORD flags)
{
    //("(%p %s %lu) stub\n", session, debugstr_w(path), flags);
    return NULL;
}

BOOL WINAPI EvtGetChannelConfigProperty(EVT_HANDLE ChannelConfig,
                                        EVT_CHANNEL_CONFIG_PROPERTY_ID PropertyId,
                                        DWORD Flags,
                                        DWORD PropertyValueBufferSize,
                                        PEVT_VARIANT PropertyValueBuffer,
                                        DWORD *PropertyValueBufferUsed)
{
    //("(%p %i %lu %lu %p %p) stub\n", ChannelConfig, PropertyId, Flags, PropertyValueBufferSize,
      ///    PropertyValueBuffer, PropertyValueBufferUsed);

    switch (PropertyId)
    {
    case EvtChannelLoggingConfigLogFilePath:
        *PropertyValueBufferUsed = sizeof(log_pathW) + sizeof(EVT_VARIANT);

        if (PropertyValueBufferSize < sizeof(log_pathW) + sizeof(EVT_VARIANT) || !PropertyValueBuffer)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        PropertyValueBuffer->StringVal = (LPWSTR)(PropertyValueBuffer + 1);
        wcscpy((LPWSTR)PropertyValueBuffer->StringVal, log_pathW);
        PropertyValueBuffer->Type = EvtVarTypeString;
        return TRUE;

    default:
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        break;
    }

    return FALSE;
}

BOOL WINAPI EvtSetChannelConfigProperty(EVT_HANDLE ChannelConfig,
                                        EVT_CHANNEL_CONFIG_PROPERTY_ID PropertyId,
                                        DWORD Flags,
                                        PEVT_VARIANT PropertyValue)
{
    //("(%p %i %lu %p) stub\n", ChannelConfig, PropertyId, Flags, PropertyValue);
    return TRUE;
}

EVT_HANDLE WINAPI EvtSubscribe(EVT_HANDLE Session, HANDLE SignalEvent, LPCWSTR ChannelPath,
                               LPCWSTR Query, EVT_HANDLE Bookmark, PVOID context,
                               EVT_SUBSCRIBE_CALLBACK Callback, DWORD Flags)
{
    //("(%p %p %s %s %p %p %p %lu) stub\n", Session, SignalEvent, debugstr_w(ChannelPath),
          //debugstr_w(Query), Bookmark, context, Callback, Flags);
    return NULL;
}

EVT_HANDLE WINAPI EvtOpenChannelEnum(EVT_HANDLE session, DWORD flags)
{
    //("(%p %lu) stub\n", session, flags);
    return NULL;
}

BOOL WINAPI EvtNextChannelPath(EVT_HANDLE channel_enum, DWORD buffer_len, WCHAR *buffer, DWORD *used)
{
    //("(%p %lu %p %p) stub\n", channel_enum, buffer_len, buffer, used);
    return FALSE;
}

EVT_HANDLE WINAPI EvtOpenChannelConfig(EVT_HANDLE Session, LPCWSTR ChannelPath, DWORD Flags)
{
    //("(%p %s %lu) stub\n", Session, debugstr_w(ChannelPath), Flags);
    return (EVT_HANDLE)NULL;
}

EVT_HANDLE WINAPI EvtQuery(EVT_HANDLE session, const WCHAR *path, const WCHAR *query, DWORD flags)
{
    //("(%p %s %s %lu) stub\n", session, debugstr_w(path), debugstr_w(query), flags);
    return NULL;
}

BOOL WINAPI EvtSaveChannelConfig(EVT_HANDLE channel, DWORD flags)
{
    //("(%p,%08lx) stub\n", channel, flags);
    return TRUE;
}

BOOL WINAPI EvtClose(EVT_HANDLE handle)
{
    ////("(%p) stub\n", handle);
    return TRUE;
}

BOOL WINAPI EvtNext(EVT_HANDLE result_set, DWORD size, EVT_HANDLE *array, DWORD timeout, DWORD flags, DWORD *ret_count)
{
    ////("(%p %lu %p %lu %#lx %p) stub!\n", result_set, size, array, timeout, flags, ret_count);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI EvtExportLog(EVT_HANDLE session, const WCHAR *path, const WCHAR *query, const WCHAR *file, DWORD flags)
{
    //("(%p %s %s %s %#lx) stub!\n", session, debugstr_w(path), debugstr_w(query), debugstr_w(file), flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

