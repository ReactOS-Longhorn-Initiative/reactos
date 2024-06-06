;Windows Vista - 7+
@ stdcall -stub _DxgkSqmAddToStream(long long ptr)
@ stdcall -stub _DxgkSqmCommonGeneric(long long ptr long)
@ stdcall -stub _DxgkSqmCreateDwordStreamEntry(ptr long)
@ stdcall -stub _DxgkSqmCreateStringStreamEntry(ptr ptr)
@ stdcall -stub _DxgkSqmGenericDword64(ptr)
@ stdcall -stub _DxgkSqmGenericDword(ptr)
@ stdcall -stub DpiGetDriverVersion(long)
@ stdcall -stub DpiGetDxgAdapter(long)
@ stdcall -stub DpiGetSchedulerCallbackState(long)
@ stdcall -stub DpiSetSchedulerCallbackState(long ptr)
@ stdcall -stub DpSynchronizeExecution(long long ptr long ptr)
@ stdcall -stub TdrCompleteRecoveryContext(ptr long)
@ stdcall -stub TdrCreateRecoveryContext()
@ stdcall -stub TdrIsRecoveryRequired(ptr)
@ stdcall -stub TdrIsTimeoutForcedFlip()
@ stdcall -stub TdrResetFromTimeout(ptr)
@ stdcall -stub TraceDxgkBlockThread(long)
@ stdcall -stub TraceDxgkContext(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall -stub TraceDxgkDevice(long ptr ptr ptr ptr)
@ stdcall -stub TraceDxgkFunctionProfiler(long long)
@ stdcall -stub TraceDxgkPerformanceWarning(long)
@ stdcall -stub TraceDxgkPresentHistory(long long ptr)
;Windows 8
@ stub -version=0x602+ DxgkSqmCommonGeneric ;26
@ stub -version=0x602+ DxgkSqmCreateDwordStreamEntry ;27
@ stub -version=0x602+ DxgkSqmCreateStringStreamEntry ;28
@ stub -version=0x602+ DxgkSqmGenericDword ;29
@ stub -version=0x602+ DxgkSqmGenericDword64 ;30
@ stub -version=0x602+ DxgkSqmGenericString ;31
@ stub -version=0x602+ DxgkSqmOptedIn ;32
@ stub -version=0x602+ DxgkSqmSetDword ;33
@ stub -version=0x602+ TraceDxgkBlockThread ;34
@ stub -version=0x602+ TraceDxgkContext ;35
@ stub -version=0x602+ TraceDxgkDevice ;36
@ stub -version=0x602+ TraceDxgkFunctionProfiler ;37
@ stub -version=0x602+ TraceDxgkPerformanceWarning ;38
@ stub -version=0x602+ DxgkSqmSetDword ;39
