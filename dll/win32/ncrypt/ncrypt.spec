@ stdcall BCryptAddContextFunction(long wstr long wstr long) bcrypt.BCryptAddContextFunction
@ stdcall BCryptAddContextFunctionProvider(long wstr long wstr wstr long) bcrypt.BCryptAddContextFunctionProvider
@ stdcall BCryptCloseAlgorithmProvider(ptr long) bcrypt.BCryptCloseAlgorithmProvider
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long) bcrypt.BCryptCreateHash
@ stdcall BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptDecrypt
@ stub BCryptDeleteContext
@ stdcall BCryptDeriveKey(ptr wstr ptr ptr long ptr long) bcrypt.BCryptDeriveKey
@ stdcall BCryptDeriveKeyCapi(ptr ptr ptr long long) bcrypt.BCryptDeriveKeyCapi
@ stdcall BCryptDeriveKeyPBKDF2(ptr ptr long ptr long int64 ptr long long) bcrypt.BCryptDeriveKeyPBKDF2
@ stdcall BCryptDestroyHash(ptr) bcrypt.BCryptDestroyHash
@ stdcall BCryptDestroyKey(ptr) bcrypt.BCryptDestroyKey
@ stdcall BCryptDestroySecret(ptr) bcrypt.BCryptDestroySecret
@ stdcall BCryptDuplicateHash(ptr ptr ptr long long) bcrypt.BCryptDuplicateHash
@ stdcall BCryptDuplicateKey(ptr ptr ptr long long) bcrypt.BCryptDuplicateKey
@ stdcall BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptEncrypt
@ stdcall BCryptEnumAlgorithms(long ptr ptr long) bcrypt.BCryptEnumAlgorithms
@ stub BCryptEnumContextFunctionProviders
@ stdcall BCryptEnumContextFunctions(long wstr long ptr ptr) bcrypt.BCryptEnumContextFunctions
@ stub BCryptEnumContexts
@ stub BCryptEnumProviders
@ stub BCryptEnumRegisteredProviders
@ stdcall BCryptExportKey(ptr ptr wstr ptr long ptr long) bcrypt.BCryptExportKey
@ stdcall BCryptFinalizeKeyPair(ptr long) bcrypt.BCryptFinalizeKeyPair
@ stdcall BCryptFinishHash(ptr ptr long long) bcrypt.BCryptFinishHash
@ stdcall BCryptFreeBuffer(ptr) bcrypt.BCryptFreeBuffer
@ stdcall BCryptGenRandom(ptr ptr long long) bcrypt.BCryptGenRandom
@ stdcall BCryptGenerateKeyPair(ptr ptr long long) bcrypt.BCryptGenerateKeyPair
@ stdcall BCryptGenerateSymmetricKey(ptr ptr ptr long ptr long long) bcrypt.BCryptGenerateSymmetricKey
@ stdcall BCryptGetFipsAlgorithmMode(ptr) bcrypt.BCryptGetFipsAlgorithmMode
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long) bcrypt.BCryptGetProperty
@ stdcall BCryptHash(ptr ptr long ptr long ptr long) bcrypt.BCryptHash
@ stdcall BCryptHashData(ptr ptr long long) bcrypt.BCryptHashData
@ stdcall BCryptImportKey(ptr ptr wstr ptr ptr long ptr long long) bcrypt.BCryptImportKey
@ stdcall BCryptImportKeyPair(ptr ptr wstr ptr ptr long long) bcrypt.BCryptImportKeyPair
@ stdcall BCryptKeyDerivation(ptr ptr ptr long ptr long) bcrypt.BCryptKeyDerivation
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long) bcrypt.BCryptOpenAlgorithmProvider
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stdcall BCryptRegisterProvider(wstr long ptr) bcrypt.BCryptRegisterProvider
@ stdcall BCryptRemoveContextFunction(long wstr long wstr) bcrypt.BCryptRemoveContextFunction
@ stdcall BCryptRemoveContextFunctionProvider(long wstr long wstr wstr) bcrypt.BCryptRemoveContextFunctionProvider
@ stub BCryptResolveProviders
@ stdcall BCryptSecretAgreement(ptr ptr ptr long) bcrypt.BCryptSecretAgreement
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stdcall BCryptSetProperty(ptr wstr ptr long long) bcrypt.BCryptSetProperty
@ stdcall BCryptSignHash(ptr ptr ptr long ptr long ptr long) bcrypt.BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stdcall BCryptUnregisterProvider(wstr) bcrypt.BCryptUnregisterProvider
@ stdcall BCryptVerifySignature(ptr ptr ptr long ptr long long) bcrypt.BCryptVerifySignature
@ stub GetIsolationServerInterface
@ stub GetKeyStorageInterface
@ stub GetSChannelInterface
@ stub NCryptCloseKeyProtector
@ stub NCryptCloseProtectionDescriptor
@ stub NCryptCreateClaim
@ stdcall NCryptCreatePersistedKey(long ptr wstr wstr long long)
@ stub NCryptCreateProtectionDescriptor
@ stdcall NCryptDecrypt(long ptr long ptr ptr long ptr long)
@ stdcall NCryptDeleteKey(long long)
@ stub NCryptDeriveKey
@ stub NCryptDuplicateKeyProtectorHandle
@ stdcall NCryptEncrypt(long ptr long ptr ptr long ptr long)
@ stdcall NCryptEnumAlgorithms(long long ptr ptr long)
@ stdcall NCryptEnumKeys(long wstr ptr ptr long)
@ stub NCryptEnumStorageProviders
@ stdcall NCryptExportKey(long long wstr ptr ptr long ptr long)
@ stdcall NCryptFinalizeKey(long long)
@ stdcall NCryptFreeBuffer(ptr)
@ stdcall NCryptFreeObject(long)
@ stdcall NCryptGetProperty(ptr wstr ptr long ptr long)
@ stub NCryptGetProtectionDescriptorInfo
@ stdcall NCryptImportKey(long long wstr ptr ptr ptr long long)
@ stdcall NCryptIsAlgSupported(long wstr long)
@ stdcall NCryptIsKeyHandle(long)
@ stub NCryptKeyDerivation
@ stub NCryptNotifyChangeKey
@ stdcall NCryptOpenKey(long ptr wstr long long)
@ stub NCryptOpenKeyProtector
@ stdcall NCryptOpenStorageProvider(ptr wstr long)
@ stub NCryptProtectKey
@ stub NCryptProtectSecret
@ stub NCryptQueryProtectionDescriptorName
@ stub NCryptRegisterProtectionDescriptorName
@ stub NCryptSecretAgreement
@ stub NCryptSetAuditingInterface
@ stdcall NCryptSetProperty(ptr wstr ptr long long)
@ stdcall NCryptSignHash(long ptr ptr long ptr long ptr long)
@ stub NCryptStreamClose
@ stub NCryptStreamOpenToProtect
@ stub NCryptStreamOpenToUnprotect
@ stub NCryptStreamOpenToUnprotectEx
@ stub NCryptStreamUpdate
@ stdcall NCryptTranslateHandle(ptr ptr long long long long)
@ stub NCryptUnprotectKey
@ stub NCryptUnprotectSecret
@ stub NCryptVerifyClaim
@ stdcall NCryptVerifySignature(ptr ptr ptr long ptr long long)
@ stdcall SslChangeNotify(ptr long)
@ stdcall SslComputeClientAuthHash(long long long wstr ptr long ptr long)
@ stdcall SslComputeEapKeyBlock(long long ptr long ptr long ptr long)
@ stdcall SslComputeFinishedHash(long long long ptr long long)
@ stdcall SslComputeSessionHash(long long long ptr long ptr long)
@ stdcall SslCreateClientAuthHash(long ptr long long wstr long)
@ stdcall SslCreateEphemeralKey(long ptr long long long long ptr long long)
@ stdcall SslCreateHandshakeHash(long ptr long long long)
@ stdcall SslDecrementProviderReferenceCount(long)
@ stdcall SslDecryptPacket(long long ptr long ptr long ptr int64 long)
@ stdcall SslEncryptPacket(long long ptr long ptr long ptr int64 long long)
@ stdcall SslEnumCipherSuites(long long ptr ptr long)
@ stdcall SslEnumEccCurves(long ptr ptr long)
@ stdcall SslEnumProtocolProviders(ptr ptr long)
@ stdcall SslExportKey(long long wstr ptr long ptr long)
@ stdcall SslExportKeyingMaterial(long long str ptr long ptr long ptr long long)
@ stdcall SslFreeBuffer(ptr)
@ stdcall SslFreeObject(long long)
@ stdcall SslGenerateMasterKey(long long long ptr long long ptr ptr long ptr long)
@ stdcall SslGeneratePreMasterKey(long long ptr long long ptr ptr long long)
@ stdcall SslGenerateSessionKeys(long long ptr ptr ptr long)
@ stdcall SslGetCipherSuitePRFHashAlgorithm(long long long long wstr long)
@ stdcall SslGetKeyProperty(long wstr ptr ptr long)
@ stdcall SslGetProviderProperty(long wstr ptr ptr ptr long)
@ stdcall SslHashHandshake(long long ptr long long)
@ stdcall SslImportKey(long ptr wstr ptr long long)
@ stdcall SslImportMasterKey(long long ptr long long ptr long long)
@ stdcall SslIncrementProviderReferenceCount(long)
@ stdcall SslLookupCipherLengths(long long long long ptr long long)
@ stdcall SslLookupCipherSuiteInfo(long long long long ptr long)
@ stdcall SslOpenPrivateKey(long ptr ptr long)
@ stdcall SslOpenProvider(ptr wstr long)
@ stdcall SslSignHash(long long ptr long ptr long ptr long)
@ stdcall SslVerifySignature(long long ptr long ptr long long)
