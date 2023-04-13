@ stdcall -stub BCryptAddContextFunction(long wstr long wstr long) bcrypt.BCryptAddContextFunction
@ stdcall -stub BCryptAddContextFunctionProvider(long wstr long wstr wstr long) bcrypt.BCryptAddContextFunctionProvider
@ stdcall -stub BCryptCloseAlgorithmProvider(ptr long) bcrypt.BCryptCloseAlgorithmProvider
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall -stub BCryptCreateHash(ptr ptr ptr long ptr long long) bcrypt.BCryptCreateHash
@ stdcall -stub BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptDecrypt
@ stub BCryptDeleteContext
@ stdcall -stub BCryptDeriveKey(ptr wstr ptr ptr long ptr long) bcrypt.BCryptDeriveKey
@ stdcall -stub BCryptDeriveKeyCapi(ptr ptr ptr long long) bcrypt.BCryptDeriveKeyCapi
@ stdcall -stub BCryptDeriveKeyPBKDF2(ptr ptr long ptr long int64 ptr long long) bcrypt.BCryptDeriveKeyPBKDF2
@ stdcall -stub BCryptDestroyHash(ptr) bcrypt.BCryptDestroyHash
@ stdcall -stub BCryptDestroyKey(ptr) bcrypt.BCryptDestroyKey
@ stdcall -stub BCryptDestroySecret(ptr) bcrypt.BCryptDestroySecret
@ stdcall -stub BCryptDuplicateHash(ptr ptr ptr long long) bcrypt.BCryptDuplicateHash
@ stdcall -stub BCryptDuplicateKey(ptr ptr ptr long long) bcrypt.BCryptDuplicateKey
@ stdcall -stub BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long) bcrypt.BCryptEncrypt
@ stdcall -stub BCryptEnumAlgorithms(long ptr ptr long) bcrypt.BCryptEnumAlgorithms
@ stub BCryptEnumContextFunctionProviders
@ stdcall -stub BCryptEnumContextFunctions(long wstr long ptr ptr) bcrypt.BCryptEnumContextFunctions
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
@ stub BCryptKeyDerivation
@ stdcall CryptOpenAlgorithmProvider(ptr wstr wstr long) bcrypt.BCryptOpenAlgorithmProvider
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stdcall BCryptRegisterProvider(wstr long ptr) bcrypt.BCryptRegisterProvider
@ stdcall BCryptRemoveContextFunction(long wstr long wstr) bcrypt.BCryptRemoveContextFunction
@ stdcall  BCryptRemoveContextFunctionProvider(long wstr long wstr wstr) bcrypt.BCryptRemoveContextFunctionProvider
@ stub BCryptResolveProviders
@ stdcall  BCryptSecretAgreement(ptr ptr ptr long) bcrypt.BCryptSecretAgreement
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stdcall BCryptSetProperty(ptr wstr ptr long long) bcrypt.BCryptSetProperty
@ stdcall BCryptSignHash(ptr ptr ptr long ptr long ptr long) bcrypt.BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stdcall BCryptUnregisterProvider(wstr) bcrypt.BCryptUnregisterProvider
@ stdcall  BCryptVerifySignature(ptr ptr ptr long ptr long long) bcrypt.BCryptVerifySignature
@ stub GetIsolationServerInterface
@ stub GetKeyStorageInterface
@ stub GetSChannelInterface
@ stub NCryptCloseKeyProtector
@ stub NCryptCloseProtectionDescriptor
@ stub NCryptCreateClaim
@ stdcall -stub NCryptCreatePersistedKey(long ptr wstr wstr long long)
@ stub NCryptCreateProtectionDescriptor
@ stdcall -stub NCryptDecrypt(long ptr long ptr ptr long ptr long)
@ stdcall -stub NCryptDeleteKey(long long)
@ stub NCryptDeriveKey
@ stub NCryptDuplicateKeyProtectorHandle
@ stdcall -stub NCryptEncrypt(long ptr long ptr ptr long ptr long)
@ stdcall -stub NCryptEnumAlgorithms(long long ptr ptr long)
@ stdcall -stub NCryptEnumKeys(long wstr ptr ptr long)
@ stub NCryptEnumStorageProviders
@ stub NCryptExportKey
@ stdcall -stub NCryptFinalizeKey(long long)
@ stdcall -stub NCryptFreeBuffer(ptr)
@ stdcall -stub NCryptFreeObject(long)
@ stdcall -stub NCryptGetProperty(ptr wstr ptr long ptr long)
@ stub NCryptGetProtectionDescriptorInfo
@ stdcall -stub NCryptImportKey(long long wstr ptr ptr ptr long long)
@ stdcall -stub NCryptIsAlgSupported(long wstr long)
@ stdcall -stub NCryptIsKeyHandle(long)
@ stub NCryptKeyDerivation
@ stub NCryptNotifyChangeKey
@ stdcall -stub NCryptOpenKey(long ptr wstr long long)
@ stub NCryptOpenKeyProtector
@ stdcall -stub NCryptOpenStorageProvider(ptr wstr long)
@ stub NCryptProtectKey
@ stub NCryptProtectSecret
@ stub NCryptQueryProtectionDescriptorName
@ stub NCryptRegisterProtectionDescriptorName
@ stub NCryptSecretAgreement
@ stub NCryptSetAuditingInterface
@ stdcall -stub NCryptSetProperty(ptr wstr ptr long long)
@ stdcall -stub NCryptSignHash(long ptr ptr long ptr long long long)
@ stub NCryptStreamClose
@ stub NCryptStreamOpenToProtect
@ stub NCryptStreamOpenToUnprotect
@ stub NCryptStreamOpenToUnprotectEx
@ stub NCryptStreamUpdate
@ stub NCryptTranslateHandle
@ stub NCryptUnprotectKey
@ stub NCryptUnprotectSecret
@ stub NCryptVerifyClaim
@ stdcall -stub NCryptVerifySignature(ptr ptr ptr long ptr long long)
@ stub SslChangeNotify
@ stub SslComputeClientAuthHash
@ stub SslComputeEapKeyBlock
@ stub SslComputeFinishedHash
@ stub SslComputeSessionHash
@ stub SslCreateClientAuthHash
@ stub SslCreateEphemeralKey
@ stub SslCreateHandshakeHash
@ stub SslDecrementProviderReferenceCount
@ stub SslDecryptPacket
@ stub SslEncryptPacket
@ stub SslEnumCipherSuites
@ stub SslEnumEccCurves
@ stub SslEnumProtocolProviders
@ stub SslExportKey
@ stub SslExportKeyingMaterial
@ stub SslFreeBuffer
@ stub SslFreeObject
@ stub SslGenerateMasterKey
@ stub SslGeneratePreMasterKey
@ stub SslGenerateSessionKeys
@ stub SslGetCipherSuitePRFHashAlgorithm
@ stub SslGetKeyProperty
@ stub SslGetProviderProperty
@ stub SslHashHandshake
@ stub SslImportKey
@ stub SslImportMasterKey
@ stub SslIncrementProviderReferenceCount
@ stub SslLookupCipherLengths
@ stub SslLookupCipherSuiteInfo
@ stub SslOpenPrivateKey
@ stub SslOpenProvider
@ stub SslSignHash
@ stub SslVerifySignature