@ stub FormatTagFromWfx
@ stub MFCreateGuid
@ stub MFGetIoPortHandle
@ stub MFGetPlatformVersion
@ stub MFGetRandomNumber
@ stub MFIsFeatureEnabled
@ stub MFIsQueueThread
@ stub MFPlatformBigEndian
@ stub MFPlatformLittleEndian
@ stub ValidateWaveFormat
@ stub CopyPropVariant
@ stub CreatePropVariant
@ stdcall -stub CreatePropertyStore(ptr)
@ stub DestroyPropVariant
@ stub GetAMSubtypeFromD3DFormat
@ stub GetD3DFormatFromMFSubtype
@ stub LFGetGlobalPool
@ stdcall -stub MFAddPeriodicCallback(ptr ptr ptr) 
@ stdcall -stub MFAllocateSerialWorkQueue(long ptr)
@ stdcall -stub MFAllocateWorkQueue(ptr)
@ stdcall -stub MFAllocateWorkQueueEx(long ptr)
@ stub MFAppendCollection
@ stub MFAverageTimePerFrameToFrameRate
@ stdcall -stub MFBeginCreateFile(long long long wstr ptr ptr ptr)
@ stub MFBeginGetHostByName
@ stdcall -stub MFBeginRegisterWorkQueueWithMMCSS(long wstr long ptr ptr)
@ stdcall -stub MFBeginRegisterWorkQueueWithMMCSSEx(long wstr long long ptr ptr) #.RtwqBeginRegisterWorkQueueWithMMCSS
@ stdcall -stub MFBeginUnregisterWorkQueueWithMMCSS(long ptr ptr) #.RtwqBeginUnregisterWorkQueueWithMMCSS
@ stub MFBlockThread
@ stub MFCalculateBitmapImageSize
@ stdcall -stub MFCalculateImageSize(ptr long long ptr)
@ stdcall -stub MFCancelCreateFile(ptr)
@ stdcall -stub MFCancelWorkItem(int64) #.RtwqCancelWorkItem
@ stdcall -stub MFCompareFullToPartialMediaType(ptr ptr)
@ stub MFCompareSockaddrAddresses
@ stub MFConvertColorInfoFromDXVA
@ stdcall -stub MFConvertColorInfoToDXVA(ptr ptr)
@ stub MFConvertFromFP16Array
@ stub MFConvertToFP16Array
@ stdcall -stub MFCopyImage(ptr long ptr long long long)
@ stdcall -stub MFCreate2DMediaBuffer(long long long long ptr)
@ stub MFCreateAMMediaTypeFromMFMediaType
@ stdcall -stub MFCreateAlignedMemoryBuffer(long long ptr)
@ stdcall -stub MFCreateAsyncResult(ptr ptr ptr ptr) #.RtwqCreateAsyncResult
@ stdcall -stub MFCreateAttributes(ptr long)
@ stdcall -stub MFCreateAudioMediaType(ptr ptr)
@ stdcall -stub MFCreateCollection(ptr)
@ stdcall -stub MFCreateDXGIDeviceManager(ptr ptr)
@ stdcall -stub MFCreateDXGISurfaceBuffer(ptr ptr long long ptr)
@ stdcall -stub MFCreateDXSurfaceBuffer(ptr ptr long ptr)
@ stdcall -stub MFCreateEventQueue(ptr)
@ stdcall -stub MFCreateFile(long long long wstr ptr)
@ stub MFCreateLegacyMediaBufferOnMFMediaBuffer
@ stdcall -stub MFCreateMFByteStreamOnStream(ptr ptr)
@ stdcall -stub MFCreateMFByteStreamOnStreamEx(ptr ptr)
@ stdcall -stub MFCreateMFByteStreamWrapper(ptr ptr)
@ stdcall -stub MFCreateMFVideoFormatFromMFMediaType(ptr ptr ptr)
@ stdcall -stub MFCreateMediaBufferFromMediaType(ptr int64 long long ptr)
@ stub MFCreateMediaBufferWrapper
@ stdcall -stub MFCreateMediaEvent(long ptr long ptr ptr)
@ stdcall -stub MFCreateMediaType(ptr)
@ stub MFCreateMediaTypeFromRepresentation
@ stdcall -stub MFCreateMemoryBuffer(long ptr)
@ stub MFCreateMemoryStream
@ stub MFCreatePathFromURL
@ stdcall -stub MFCreatePresentationDescriptor(long ptr ptr)
@ stdcall -stub MFCreateSample(ptr)
@ stub MFCreateSocket
@ stub MFCreateSocketListener
@ stdcall -stub MFCreateSourceResolver(ptr)
@ stdcall -stub MFCreateStreamDescriptor(long long ptr ptr)
@ stdcall -stub MFCreateSystemTimeSource(ptr)
@ stub MFCreateSystemUnderlyingClock
@ stub MFCreateTempFile
@ stdcall -stub MFCreateTrackedSample(ptr)
@ stdcall -stub MFCreateTransformActivate(ptr)
@ stub MFCreateURLFromPath
@ stub MFCreateUdpSockets
@ stub MFCreateVideoMediaType
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeader
@ stub MFCreateVideoMediaTypeFromBitMapInfoHeaderEx
@ stdcall -stub MFCreateVideoMediaTypeFromSubtype(ptr ptr)
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader2
@ stub MFCreateVideoMediaTypeFromVideoInfoHeader
@ stdcall -stub MFCreateVideoSampleAllocatorEx(ptr ptr)
@ stdcall -stub MFCreateWaveFormatExFromMFMediaType(ptr ptr ptr long)
@ stub MFDeserializeAttributesFromStream
@ stub MFDeserializeEvent
@ stub MFDeserializeMediaTypeFromStream
@ stub MFDeserializePresentationDescriptor
@ stdcall -stub MFEndCreateFile(ptr ptr)
@ stub MFEndGetHostByName
@ stdcall -stub MFEndRegisterWorkQueueWithMMCSS(ptr ptr) #.RtwqEndRegisterWorkQueueWithMMCSS
@ stdcall -stub MFEndUnregisterWorkQueueWithMMCSS(ptr) #.RtwqEndUnregisterWorkQueueWithMMCSS
@ stdcall -stub MFFrameRateToAverageTimePerFrame(long long ptr)
@ stub MFFreeAdaptersAddresses
@ stub MFGetAdaptersAddresses
@ stdcall -stub MFGetAttributesAsBlob(ptr ptr long)
@ stdcall -stub MFGetAttributesAsBlobSize(ptr ptr)
@ stub MFGetConfigurationDWORD
@ stub MFGetConfigurationPolicy
@ stub MFGetConfigurationStore
@ stub MFGetConfigurationString
@ stub MFGetMFTMerit
@ stub MFGetNumericNameFromSockaddr
@ stdcall -stub MFGetPlaneSize(long long long ptr)
@ stub MFGetPlatform
@ stdcall -stub MFGetPluginControl(ptr)
@ stub MFGetPrivateWorkqueues
@ stub MFGetSockaddrFromNumericName
@ stdcall -stub MFGetStrideForBitmapInfoHeader(long long ptr)
@ stdcall -stub MFGetSystemTime()
@ stdcall -stub MFGetTimerPeriodicity(ptr)
@ stub MFGetUncompressedVideoFormat
@ stdcall -stub MFGetWorkQueueMMCSSClass(long ptr ptr) #.RtwqGetWorkQueueMMCSSClass
@ stdcall -stub MFGetWorkQueueMMCSSTaskId(long ptr) #.RtwqGetWorkQueueMMCSSTaskId
@ stdcall -stub MFGetWorkQueueMMCSSPriority(long ptr) #.RtwqGetWorkQueueMMCSSPriority
@ stdcall -stub MFHeapAlloc(long long str long long)
@ stdcall -stub MFHeapFree(ptr)
@ stub MFInitAMMediaTypeFromMFMediaType
@ stdcall -stub MFInitAttributesFromBlob(ptr ptr long)
@ stub MFInitMediaTypeFromAMMediaType
@ stub MFInitMediaTypeFromMFVideoFormat
@ stub MFInitMediaTypeFromMPEG1VideoInfo
@ stub MFInitMediaTypeFromMPEG2VideoInfo
@ stub MFInitMediaTypeFromVideoInfoHeader2
@ stub MFInitMediaTypeFromVideoInfoHeader
@ stdcall -stub MFInitMediaTypeFromWaveFormatEx(ptr ptr long)
@ stub MFInitVideoFormat
@ stdcall -stub MFInitVideoFormat_RGB(ptr long long long)
@ stdcall -stub MFInvokeCallback(ptr)
@ stub MFJoinIoPort
@ stdcall -stub MFJoinWorkQueue(long long ptr) #.RtwqJoinWorkQueue
@ stdcall -stub MFLockDXGIDeviceManager(ptr ptr)
@ stdcall -stub MFLockPlatform() #.RtwqLockPlatform
@ stdcall -stub MFLockSharedWorkQueue(wstr long ptr ptr) #.RtwqLockSharedWorkQueue
@ stdcall -stub MFLockWorkQueue(long) #.RtwqLockWorkQueue
@ stdcall -stub MFMapDX9FormatToDXGIFormat(long)
@ stdcall -stub MFMapDXGIFormatToDX9Format(long)
@ stdcall -stub MFPutWaitingWorkItem(long long ptr ptr) #.RtwqPutWaitingWorkItem
@ stdcall -stub MFPutWorkItem(long ptr ptr)
@ stdcall -stub MFPutWorkItem2(long long ptr ptr)
@ stdcall -stub MFPutWorkItemEx(long ptr)
@ stdcall -stub MFPutWorkItemEx2(long long ptr)
@ stub MFRecordError
@ stdcall -stub MFRegisterLocalByteStreamHandler(wstr wstr ptr)
@ stdcall -stub MFRegisterLocalSchemeHandler(wstr ptr)
@ stdcall -stub MFRegisterPlatformWithMMCSS(wstr ptr long) #.RtwqRegisterPlatformWithMMCSS
@ stdcall -stub MFRemovePeriodicCallback(long) #.RtwqRemovePeriodicCallback
@ stdcall -stub MFScheduleWorkItem(ptr ptr int64 ptr)
@ stdcall -stub MFScheduleWorkItemEx(ptr int64 ptr) #.RtwqScheduleWorkItem
@ stub MFSerializeAttributesToStream
@ stub MFSerializeEvent
@ stub MFSerializeMediaTypeToStream
@ stub MFSerializePresentationDescriptor
@ stub MFSetSockaddrAny
@ stdcall -stub MFShutdown()
@ stdcall -stub MFStartup(long long)
@ stub MFStreamDescriptorProtectMediaType
@ stdcall -stub MFTEnum(int128 long ptr ptr ptr ptr ptr)
@ stdcall -stub MFTEnum2(int128 long ptr ptr ptr ptr ptr)
@ stdcall -stub MFTEnumEx(int128 long ptr ptr ptr ptr)
@ stub MFTGetInfo
@ stdcall -stub MFTRegister(int128 int128 wstr long long ptr long ptr ptr)
@ stdcall -stub MFTRegisterLocal(ptr ptr wstr long long  ptr long ptr)
@ stdcall -stub MFTRegisterLocalByCLSID(ptr ptr wstr long long ptr long ptr)
@ stdcall -stub MFTUnregister(int128)
@ stdcall -stub MFTUnregisterLocal(ptr)
@ stdcall -stub MFTUnregisterLocalByCLSID(int128)
@ stdcall -stub MFUnregisterPlatformFromMMCSS() #.RtwqUnregisterPlatformFromMMCSS
@ stub MFTraceError
@ stub MFTraceFuncEnter
@ stub MFUnblockThread
@ stdcall -stub MFUnjoinWorkQueue(long long) #.RtwqUnjoinWorkQueue
@ stdcall -stub MFUnlockDXGIDeviceManager()
@ stdcall -stub MFUnlockPlatform() #.RtwqUnlockPlatform
@ stdcall -stub MFUnlockWorkQueue(long) #.RtwqUnlockWorkQueue
@ stdcall -stub MFUnwrapMediaType(ptr ptr)
@ stub MFValidateMediaTypeSize
@ stdcall -stub MFWrapMediaType(ptr ptr ptr ptr)
@ stdcall -stub -ret64 MFllMulDiv(int64 int64 int64 int64)
@ stub PropVariantFromStream
@ stub PropVariantToStream
