1 stdcall -stub DirectPlayCreate(ptr ptr ptr)
2 stdcall -stub DirectPlayEnumerateA(ptr ptr)
3 stdcall -stub DirectPlayEnumerateW(ptr ptr)
4 stdcall -stub DirectPlayLobbyCreateA(ptr ptr ptr ptr long)
5 stdcall -stub DirectPlayLobbyCreateW(ptr ptr ptr ptr long)
;6 extern gdwDPlaySPRefCount
9 stdcall -stub DirectPlayEnumerate(ptr ptr) DirectPlayEnumerateA
@ stdcall -stub -private DllCanUnloadNow()
@ stdcall -stub -private DllGetClassObject(ptr ptr ptr)
@ stdcall -stub -private DllRegisterServer()
@ stdcall -stub -private DllUnregisterServer()
