; Matches Windows 8 RTM
1 stdcall -arch=i386 USBD_CreateConfigurationRequestEx@8(ptr ptr) USBD_CreateConfigurationRequestEx
@ stdcall -arch=i386 _USBD_ParseConfigurationDescriptorEx@28(ptr ptr long long long long long) USBD_ParseConfigurationDescriptorEx
2 stdcall USBD_ParseConfigurationDescriptorEx(ptr ptr long long long long long)
@ stdcall -arch=i386 _USBD_ParseDescriptors@16(ptr long ptr long) USBD_ParseDescriptors
3 stdcall USBD_ParseDescriptors(ptr long ptr long)
4 stdcall -private DllInitialize(long)
5 stdcall -private DllUnload()
6 stdcall -stub USBD_AddDeviceToGlobalList(long ptr long ptr long long long)
7 stdcall -stub USBD_AllocateHubNumber()
8 stdcall USBD_CalculateUsbBandwidth(long long long)
9 stdcall USBD_CreateConfigurationRequest(ptr ptr)
10 stdcall -arch=i386 _USBD_CreateConfigurationRequestEx@8(ptr ptr)
11 stdcall USBD_GetInterfaceLength(ptr ptr)
12 stdcall USBD_GetPdoRegistryParameter(ptr ptr long ptr long)
13 stdcall -stub USBD_GetRegistryKeyValue(ptr wstr long ptr long)
14 stdcall USBD_GetUSBDIVersion(ptr)
15 stdcall -stub USBD_MarkDeviceAsDisconnected(ptr)
16 stdcall USBD_ParseConfigurationDescriptor(ptr long long)
;;;;; hmm weird 17 stdcall USBD_ParseConfigurationDescriptorEx(ptr ptr long long long long long)
;;;;;;18 stdcall USBD_ParseDescriptors(ptr long ptr long)
19 stdcall USBD_QueryBusTime(ptr ptr)
20 stdcall USBD_RegisterHcFilter(ptr ptr) ; This is a stub in windows 8
21 stdcall -stub USBD_ReleaseHubNumber(long)
22 stdcall -stub USBD_RemoveDeviceFromGlobalList(ptr)
23 stdcall USBD_ValidateConfigurationDescriptor(ptr long long ptr long)


; Seems to be special
@ stdcall USBD_Debug_GetHeap(long long long long)
@ stdcall USBD_Debug_RetHeap(ptr long long)
@ stdcall USBD_Debug_LogEntry(ptr ptr ptr ptr)
