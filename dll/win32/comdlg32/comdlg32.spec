@ stdcall ChooseColorA(ptr)
@ stdcall ChooseColorW(ptr)
@ stdcall ChooseFontA(ptr)
@ stdcall ChooseFontW(ptr)
@ stdcall CommDlgExtendedError()
@ stdcall -private DllGetClassObject(ptr ptr ptr) ; Win 7
@ stdcall -private DllRegisterServer() ; Win 7
@ stdcall -private DllUnregisterServer() ; Win 7
@ stdcall FindTextA(ptr)
@ stdcall FindTextW(ptr)
@ stdcall GetFileTitleA(str ptr long)
@ stdcall GetFileTitleW(wstr ptr long)
@ stdcall GetOpenFileNameA(ptr)
@ stdcall GetOpenFileNameW(ptr)
@ stdcall GetSaveFileNameA(ptr)
@ stdcall GetSaveFileNameW(ptr)
@ stub LoadAlterBitmap
@ stdcall PageSetupDlgA(ptr)
@ stdcall PageSetupDlgW(ptr)
@ stdcall PrintDlgA(ptr)
@ stdcall PrintDlgExA(ptr)
@ stdcall PrintDlgExW(ptr)
@ stdcall PrintDlgW(ptr)
@ stdcall ReplaceTextA(ptr)
@ stdcall ReplaceTextW(ptr)
@ stub WantArrows
@ stub dwLBSubclass
@ stub dwOKSubclass
