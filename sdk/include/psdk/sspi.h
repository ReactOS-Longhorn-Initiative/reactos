/*
 * Copyright (C) 2004 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_SSPI_H__
#define __WINE_SSPI_H__

#include <wtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEC_ENTRY WINAPI

typedef WCHAR SEC_WCHAR;
typedef CHAR  SEC_CHAR;

#ifndef __SECSTATUS_DEFINED__
#define __SECSTATUS_DEFINED__
typedef LONG SECURITY_STATUS;
#endif

#ifndef UNISP_NAME_W
#define UNISP_NAME_A "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W L"Microsoft Unified Security Protocol Provider"
#define UNISP_NAME WINELIB_NAME_AW(UNISP_NAME_)
#endif

#ifdef UNICODE
typedef SEC_WCHAR * SECURITY_PSTR;
typedef CONST SEC_WCHAR *  SECURITY_PCSTR;
#else
typedef SEC_CHAR * SECURITY_PSTR;
typedef CONST SEC_CHAR *  SECURITY_PCSTR;
#endif

#ifndef __SECHANDLE_DEFINED__
#define __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#endif

#define SecInvalidateHandle(x) do { \
 ((PSecHandle)(x))->dwLower = ((ULONG_PTR)((INT_PTR)-1)); \
 ((PSecHandle)(x))->dwUpper = ((ULONG_PTR)((INT_PTR)-1)); \
 } while (0)

#define SecIsValidHandle(x) \
 ((((PSecHandle)(x))->dwLower != ((ULONG_PTR)(INT_PTR)-1)) && \
 (((PSecHandle)(x))->dwUpper != ((ULONG_PTR)(INT_PTR)-1)))

typedef SecHandle CredHandle;
typedef PSecHandle PCredHandle;

typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;

typedef struct _SECURITY_INTEGER
{
    ULONG LowPart;
    LONG HighPart;
} SECURITY_INTEGER, *PSECURITY_INTEGER;
typedef SECURITY_INTEGER TimeStamp, *PTimeStamp;

// UNICODE_STRING should have the same memory layout in 32 bit and 64 bit mode.
// In 32 bit mode SECURITY_STRING is simply a clone of UNICODE_STRING.
// It is used internal in kernel an security components.
#ifndef _NTDEF_
typedef struct _SECURITY_STRING {
    unsigned short Length;
    unsigned short MaximumLength;
    unsigned short *Buffer;
} SECURITY_STRING, *PSECURITY_STRING;
#else
typedef UNICODE_STRING SECURITY_STRING, *PSECURITY_STRING;
#endif

#define SSPIPFC_CREDPROV_DO_NOT_SAVE 0x00000001
#define SSPIPFC_NO_CHECKBOX 0x00000002

typedef void *PSEC_WINNT_AUTH_IDENTITY_OPAQUE;

ULONG SEC_ENTRY SspiPromptForCredentialsW(_In_ PCWSTR, _In_opt_ void *,
    _In_ ULONG, _In_ PCWSTR, _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE,
    _Outptr_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE *, _Inout_opt_ int *, _In_ ULONG);

typedef struct _SecPkgInfoA
{
    ULONG  fCapabilities;
    unsigned short wVersion;
    unsigned short wRPCID;
    ULONG  cbMaxToken;
    SEC_CHAR      *Name;
    SEC_CHAR      *Comment;
} SecPkgInfoA, *PSecPkgInfoA;

typedef struct _SecPkgInfoW
{
    ULONG  fCapabilities;
    unsigned short wVersion;
    unsigned short wRPCID;
    ULONG  cbMaxToken;
    SEC_WCHAR     *Name;
    SEC_WCHAR     *Comment;
} SecPkgInfoW, *PSecPkgInfoW;

#define SecPkgInfo WINELIB_NAME_AW(SecPkgInfo)
#define PSecPkgInfo WINELIB_NAME_AW(PSecPkgInfo)

/* fCapabilities field of SecPkgInfo */
#define SECPKG_FLAG_INTEGRITY                0x00000001
#define SECPKG_FLAG_PRIVACY                  0x00000002
#define SECPKG_FLAG_TOKEN_ONLY               0x00000004
#define SECPKG_FLAG_DATAGRAM                 0x00000008
#define SECPKG_FLAG_CONNECTION               0x00000010
#define SECPKG_FLAG_MULTI_REQUIRED           0x00000020
#define SECPKG_FLAG_CLIENT_ONLY              0x00000040
#define SECPKG_FLAG_EXTENDED_ERROR           0x00000080
#define SECPKG_FLAG_IMPERSONATION            0x00000100
#define SECPKG_FLAG_ACCEPT_WIN32_NAME        0x00000200
#define SECPKG_FLAG_STREAM                   0x00000400
#define SECPKG_FLAG_NEGOTIABLE               0x00000800
#define SECPKG_FLAG_GSS_COMPATIBLE           0x00001000
#define SECPKG_FLAG_LOGON                    0x00002000
#define SECPKG_FLAG_ASCII_BUFFERS            0x00004000
#define SECPKG_FLAG_FRAGMENT                 0x00008000
#define SECPKG_FLAG_MUTUAL_AUTH              0x00010000
#define SECPKG_FLAG_DELEGATION               0x00020000
#define SECPKG_FLAG_READONLY_WITH_CHECKSUM   0x00040000
#define SECPKG_FLAG_RESTRICTED_TOKENS        0x00080000
#define SECPKG_FLAG_NEGO_EXTENDER            0x00100000
#define SECPKG_FLAG_NEGOTIABLE2              0x00200000
#define SECPKG_FLAG_APPCONTAINER_PASSTHROUGH 0x00400000
#define SECPKG_FLAG_APPCONTAINER_CHECKS      0x00800000
#define SECPKG_FLAG_APPLY_LOOPBACK           0x02000000

typedef struct _SecBuffer {
  ULONG cbBuffer;
  ULONG BufferType;
  _Field_size_bytes_(cbBuffer) void *pvBuffer;
} SecBuffer, *PSecBuffer;

/* values for BufferType */
#define SECBUFFER_EMPTY               0
#define SECBUFFER_DATA                1
#define SECBUFFER_TOKEN               2
#define SECBUFFER_PKG_PARAMS          3
#define SECBUFFER_MISSING             4
#define SECBUFFER_EXTRA               5
#define SECBUFFER_STREAM_TRAILER      6
#define SECBUFFER_STREAM_HEADER       7
#define SECBUFFER_NEGOTIATION_INFO    8
#define SECBUFFER_PADDING             9
#define SECBUFFER_STREAM             10
#define SECBUFFER_MECHLIST           11
#define SECBUFFER_MECHLIST_SIGNATURE 12
#define SECBUFFER_TARGET             13
#define SECBUFFER_CHANNEL_BINDINGS   14

#define SECBUFFER_ATTRMASK               0xf0000000
#define SECBUFFER_READONLY               0x80000000
#define SECBUFFER_READONLY_WITH_CHECKSUM 0x10000000
#define SECBUFFER_RESERVED               0x60000000

typedef struct _SecBufferDesc {
  ULONG ulVersion;
  ULONG cBuffers;
  _Field_size_(cBuffers) PSecBuffer pBuffers;
} SecBufferDesc, *PSecBufferDesc;

/* values for ulVersion */
#define SECBUFFER_VERSION 0

typedef void (SEC_ENTRY *SEC_GET_KEY_FN)(void *Arg, void *Principal,
 ULONG KeyVer, void **Key, SECURITY_STATUS *Status);

SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesA(
  _Out_ PULONG pcPackages,
  _Outptr_ PSecPkgInfoA *ppPackageInfo);

SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(
  _Out_ PULONG pcPackages,
  _Outptr_ PSecPkgInfoW *ppPackageInfo);

#define EnumerateSecurityPackages WINELIB_NAME_AW(EnumerateSecurityPackages)

typedef SECURITY_STATUS (SEC_ENTRY *ENUMERATE_SECURITY_PACKAGES_FN_A)(PULONG,
 PSecPkgInfoA *);
typedef SECURITY_STATUS (SEC_ENTRY *ENUMERATE_SECURITY_PACKAGES_FN_W)(PULONG,
 PSecPkgInfoW *);
#define ENUMERATE_SECURITY_PACKAGES_FN WINELIB_NAME_AW(ENUMERATE_SECURITY_PACKAGES_FN_)

SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesA(
  _In_ PCredHandle phCredential,
  _In_ ULONG ulAttribute,
  _Inout_ void *pBuffer);

SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesW(
  _In_ PCredHandle phCredential,
  _In_ ULONG ulAttribute,
  _Inout_ void *pBuffer);

#define QueryCredentialsAttributes WINELIB_NAME_AW(QueryCredentialsAttributes)

typedef SECURITY_STATUS (SEC_ENTRY *QUERY_CREDENTIALS_ATTRIBUTES_FN_A)
 (PCredHandle, ULONG, PVOID);
typedef SECURITY_STATUS (SEC_ENTRY *QUERY_CREDENTIALS_ATTRIBUTES_FN_W)
 (PCredHandle, ULONG, PVOID);
#define QUERY_CREDENTIALS_ATTRIBUTES_FN WINELIB_NAME_AW(QUERY_CREDENTIALS_ATTRIBUTES_FN_)

typedef struct _SEC_CHANNEL_BINDINGS {
    ULONG dwInitiatorAddrType;
    ULONG cbInitiatorLength;
    ULONG dwInitiatorOffset;
    ULONG dwAcceptorAddrType;
    ULONG cbAcceptorLength;
    ULONG dwAcceptorOffset;
    ULONG cbApplicationDataLength;
    ULONG dwApplicationDataOffset;
} SEC_CHANNEL_BINDINGS, *PSEC_CHANNEL_BINDINGS;

/* values for QueryCredentialsAttributes ulAttribute */
#define SECPKG_CRED_ATTR_NAMES 1

/* types for QueryCredentialsAttributes */
typedef struct _SecPkgCredentials_NamesA
{
    SEC_CHAR *sUserName;
} SecPkgCredentials_NamesA, *PSecPkgCredentials_NamesA;

typedef struct _SecPkgCredentials_NamesW
{
    SEC_WCHAR *sUserName;
} SecPkgCredentials_NamesW, *PSecPkgCredentials_NamesW;

#define SecPkgCredentials_Names WINELIB_NAME_AW(SecPkgCredentials_Names)

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleA(
  _In_opt_ SEC_CHAR *pszPrincipal,
  _In_ SEC_CHAR *pszPackage,
  _In_ ULONG fCredentialsUse,
  _In_opt_ PLUID pvLogonID,
  _In_opt_ PVOID pAuthData,
  _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
  _In_opt_ PVOID pvGetKeyArgument,
  _Out_ PCredHandle phCredential,
  _Out_opt_ PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(
  _In_opt_ SEC_WCHAR *pszPrincipal,
  _In_ SEC_WCHAR *pszPackage,
  _In_ ULONG fCredentialsUse,
  _In_opt_ PLUID pvLogonID,
  _In_opt_ PVOID pAuthData,
  _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
  _In_opt_ PVOID pvGetKeyArgument,
  _Out_ PCredHandle phCredential,
  _Out_opt_ PTimeStamp ptsExpiry);

#define AcquireCredentialsHandle WINELIB_NAME_AW(AcquireCredentialsHandle)

/* flags for fCredentialsUse */
#define SECPKG_CRED_INBOUND  0x00000001
#define SECPKG_CRED_OUTBOUND 0x00000002
#define SECPKG_CRED_BOTH     (SECPKG_CRED_INBOUND | SECPKG_CRED_OUTBOUND)
#define SECPKG_CRED_DEFAULT  0x00000004
#define SECPKG_CRED_RESERVED 0xf0000000

typedef SECURITY_STATUS (SEC_ENTRY *ACQUIRE_CREDENTIALS_HANDLE_FN_A)(
 SEC_CHAR *, SEC_CHAR *, ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID,
 PCredHandle, PTimeStamp);
typedef SECURITY_STATUS (SEC_ENTRY *ACQUIRE_CREDENTIALS_HANDLE_FN_W)(
 SEC_WCHAR *, SEC_WCHAR *, ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID,
 PCredHandle, PTimeStamp);
#define ACQUIRE_CREDENTIALS_HANDLE_FN WINELIB_NAME_AW(ACQUIRE_CREDENTIALS_HANDLE_FN_)

SECURITY_STATUS SEC_ENTRY FreeContextBuffer(_Inout_ PVOID pv);

typedef SECURITY_STATUS (SEC_ENTRY *FREE_CONTEXT_BUFFER_FN)(_Inout_ PVOID);

SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(
  _In_ PCredHandle phCredential);

#define FreeCredentialHandle FreeCredentialsHandle

typedef SECURITY_STATUS (SEC_ENTRY *FREE_CREDENTIALS_HANDLE_FN)(PCredHandle);

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextA(
  _In_opt_ PCredHandle phCredential,
  _In_opt_ PCtxtHandle phContext,
  _In_opt_ SEC_CHAR *pszTargetName,
  _In_ ULONG fContextReq,
  _In_ ULONG Reserved1,
  _In_ ULONG TargetDataRep,
  _In_opt_ PSecBufferDesc pInput,
  _In_ ULONG Reserved2,
  _Inout_opt_ PCtxtHandle phNewContext,
  _Inout_opt_ PSecBufferDesc pOutput,
  _Out_ ULONG *pfContextAttr,
  _Out_opt_ PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(
  _In_opt_ PCredHandle phCredential,
  _In_opt_ PCtxtHandle phContext,
  _In_opt_ SEC_WCHAR *pszTargetName,
  _In_ ULONG fContextReq,
  _In_ ULONG Reserved1,
  _In_ ULONG TargetDataRep,
  _In_opt_ PSecBufferDesc pInput,
  _In_ ULONG Reserved2,
  _Inout_opt_ PCtxtHandle phNewContext,
  _Inout_opt_ PSecBufferDesc pOutput,
  _Out_ ULONG *pfContextAttr,
  _Out_opt_ PTimeStamp ptsExpiry);

#define InitializeSecurityContext WINELIB_NAME_AW(InitializeSecurityContext)

typedef SECURITY_STATUS (SEC_ENTRY *INITIALIZE_SECURITY_CONTEXT_FN_A)
 (PCredHandle, PCtxtHandle, SEC_CHAR *, ULONG, ULONG,
 ULONG, PSecBufferDesc, ULONG, PCtxtHandle, PSecBufferDesc,
 ULONG *, PTimeStamp);
typedef SECURITY_STATUS (SEC_ENTRY *INITIALIZE_SECURITY_CONTEXT_FN_W)
 (PCredHandle, PCtxtHandle, SEC_WCHAR *, ULONG, ULONG,
 ULONG, PSecBufferDesc, ULONG, PCtxtHandle, PSecBufferDesc,
 ULONG *, PTimeStamp);
#define INITIALIZE_SECURITY_CONTEXT_FN WINELIB_NAME_AW(INITIALIZE_SECURITY_CONTEXT_FN_)

/* flags for InitializeSecurityContext fContextReq and pfContextAttr */
#define ISC_REQ_DELEGATE               0x00000001
#define ISC_REQ_MUTUAL_AUTH            0x00000002
#define ISC_REQ_REPLAY_DETECT          0x00000004
#define ISC_REQ_SEQUENCE_DETECT        0x00000008
#define ISC_REQ_CONFIDENTIALITY        0x00000010
#define ISC_REQ_USE_SESSION_KEY        0x00000020
#define ISC_REQ_PROMPT_FOR_CREDS       0x00000040
#define ISC_REQ_USE_SUPPLIED_CREDS     0x00000080
#define ISC_REQ_ALLOCATE_MEMORY        0x00000100
#define ISC_REQ_USE_DCE_STYLE          0x00000200
#define ISC_REQ_DATAGRAM               0x00000400
#define ISC_REQ_CONNECTION             0x00000800
#define ISC_REQ_CALL_LEVEL             0x00001000
#define ISC_REQ_FRAGMENT_SUPPLIED      0x00002000
#define ISC_REQ_EXTENDED_ERROR         0x00004000
#define ISC_REQ_STREAM                 0x00008000
#define ISC_REQ_INTEGRITY              0x00010000
#define ISC_REQ_IDENTIFY               0x00020000
#define ISC_REQ_NULL_SESSION           0x00040000
#define ISC_REQ_MANUAL_CRED_VALIDATION 0x00080000
#define ISC_REQ_RESERVED1              0x00100000
#define ISC_REQ_FRAGMENT_TO_FIT        0x00200000

#define ISC_RET_DELEGATE               0x00000001
#define ISC_RET_MUTUAL_AUTH            0x00000002
#define ISC_RET_REPLAY_DETECT          0x00000004
#define ISC_RET_SEQUENCE_DETECT        0x00000008
#define ISC_RET_CONFIDENTIALITY        0x00000010
#define ISC_RET_USE_SESSION_KEY        0x00000020
#define ISC_RET_USED_COLLECTED_CREDS   0x00000040
#define ISC_RET_USED_SUPPLIED_CREDS    0x00000080
#define ISC_RET_ALLOCATED_MEMORY       0x00000100
#define ISC_RET_USED_DCE_STYLE         0x00000200
#define ISC_RET_DATAGRAM               0x00000400
#define ISC_RET_CONNECTION             0x00000800
#define ISC_RET_INTERMEDIATE_RETURN    0x00001000
#define ISC_RET_CALL_LEVEL             0x00002000
#define ISC_RET_EXTENDED_ERROR         0x00004000
#define ISC_RET_STREAM                 0x00008000
#define ISC_RET_INTEGRITY              0x00010000
#define ISC_RET_IDENTIFY               0x00020000
#define ISC_RET_NULL_SESSION           0x00040000
#define ISC_RET_MANUAL_CRED_VALIDATION 0x00080000
#define ISC_RET_RESERVED1              0x00100000
#define ISC_RET_FRAGMENT_ONLY          0x00200000

SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(
  _In_opt_ PCredHandle phCredential,
  _In_opt_ PCtxtHandle phContext,
  _In_opt_ PSecBufferDesc pInput,
  _In_ ULONG fContextReq,
  _In_ ULONG TargetDataRep,
  _Inout_opt_ PCtxtHandle phNewContext,
  _Inout_opt_ PSecBufferDesc pOutput,
  _Out_ ULONG *pfContextAttr,
  _Out_opt_ PTimeStamp ptsExpiry);

typedef SECURITY_STATUS (SEC_ENTRY *ACCEPT_SECURITY_CONTEXT_FN)(PCredHandle,
 PCtxtHandle, PSecBufferDesc, ULONG, ULONG, PCtxtHandle,
 PSecBufferDesc, ULONG *, PTimeStamp);

/* flags for AcceptSecurityContext fContextReq and pfContextAttr */
#define ASC_REQ_DELEGATE               0x00000001
#define ASC_REQ_MUTUAL_AUTH            0x00000002
#define ASC_REQ_REPLAY_DETECT          0x00000004
#define ASC_REQ_SEQUENCE_DETECT        0x00000008
#define ASC_REQ_CONFIDENTIALITY        0x00000010
#define ASC_REQ_USE_SESSION_KEY        0x00000020
#define ASC_REQ_ALLOCATE_MEMORY        0x00000100
#define ASC_REQ_USE_DCE_STYLE          0x00000200
#define ASC_REQ_DATAGRAM               0x00000400
#define ASC_REQ_CONNECTION             0x00000800
#define ASC_REQ_CALL_LEVEL             0x00001000
#define ASC_REQ_FRAGMENT_SUPPLIED      0x00002000
#define ASC_REQ_EXTENDED_ERROR         0x00008000
#define ASC_REQ_STREAM                 0x00010000
#define ASC_REQ_INTEGRITY              0x00020000
#define ASC_REQ_LICENSING              0x00040000
#define ASC_REQ_IDENTIFY               0x00080000
#define ASC_REQ_ALLOW_NULL_SESSION     0x00100000
#define ASC_REQ_ALLOW_NON_USER_LOGONS  0x00200000
#define ASC_REQ_ALLOW_CONTEXT_REPLAY   0x00400000
#define ASC_REQ_FRAGMENT_TO_FIT        0x00800000
#define ASC_REQ_FRAGMENT_NO_TOKEN      0x01000000

#define ASC_RET_DELEGATE               0x00000001
#define ASC_RET_MUTUAL_AUTH            0x00000002
#define ASC_RET_REPLAY_DETECT          0x00000004
#define ASC_RET_SEQUENCE_DETECT        0x00000008
#define ASC_RET_CONFIDENTIALITY        0x00000010
#define ASC_RET_USE_SESSION_KEY        0x00000020
#define ASC_RET_ALLOCATED_MEMORY       0x00000100
#define ASC_RET_USED_DCE_STYLE         0x00000200
#define ASC_RET_DATAGRAM               0x00000400
#define ASC_RET_CONNECTION             0x00000800
#define ASC_RET_CALL_LEVEL             0x00002000
#define ASC_RET_THIRD_LEG_FAILED       0x00004000
#define ASC_RET_EXTENDED_ERROR         0x00008000
#define ASC_RET_STREAM                 0x00010000
#define ASC_RET_INTEGRITY              0x00020000
#define ASC_RET_LICENSING              0x00040000
#define ASC_RET_IDENTIFY               0x00080000
#define ASC_RET_NULL_SESSION           0x00100000
#define ASC_RET_ALLOW_NON_USER_LOGONS  0x00200000
#define ASC_RET_ALLOW_CONTEXT_REPLAY   0x00400000
#define ASC_RET_FRAGMENT_ONLY          0x00800000
#define ASC_RET_NO_TOKEN               0x01000000

/*Vvalues for TargetDataRep */
#define SECURITY_NATIVE_DREP           0x00000010
#define SECURITY_NETWORK_DREP          0x00000000

SECURITY_STATUS
SEC_ENTRY
CompleteAuthToken(
  _In_ PCtxtHandle phContext,
  _In_ PSecBufferDesc pToken);

typedef SECURITY_STATUS (SEC_ENTRY *COMPLETE_AUTH_TOKEN_FN)(PCtxtHandle,
 PSecBufferDesc);

SECURITY_STATUS SEC_ENTRY DeleteSecurityContext(_In_ PCtxtHandle phContext);

typedef SECURITY_STATUS (SEC_ENTRY *DELETE_SECURITY_CONTEXT_FN)(PCtxtHandle);

SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(
  _In_ PCtxtHandle phContext,
  _In_ PSecBufferDesc pInput);

typedef SECURITY_STATUS (SEC_ENTRY *APPLY_CONTROL_TOKEN_FN)(PCtxtHandle,
 PSecBufferDesc);

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(
  _In_ PCtxtHandle phContext,
  _In_ ULONG ulAttribute,
  _Out_ void *pBuffer);

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
  _In_ PCtxtHandle phContext,
  _In_ ULONG ulAttribute,
  _Out_ void *pBuffer);

#define QueryContextAttributes WINELIB_NAME_AW(QueryContextAttributes)

typedef SECURITY_STATUS (SEC_ENTRY *QUERY_CONTEXT_ATTRIBUTES_FN_A)(PCtxtHandle,
 ULONG, void *);
typedef SECURITY_STATUS (SEC_ENTRY *QUERY_CONTEXT_ATTRIBUTES_FN_W)(PCtxtHandle,
 ULONG, void *);
#define QUERY_CONTEXT_ATTRIBUTES_FN WINELIB_NAME_AW(QUERY_CONTEXT_ATTRIBUTES_FN_)

/* values for QueryContextAttributes/SetContextAttributes ulAttribute */
#define SECPKG_ATTR_SIZES               0
#define SECPKG_ATTR_NAMES               1
#define SECPKG_ATTR_LIFESPAN            2
#define SECPKG_ATTR_DCE_INFO            3
#define SECPKG_ATTR_STREAM_SIZES        4
#define SECPKG_ATTR_KEY_INFO            5
#define SECPKG_ATTR_AUTHORITY           6
#define SECPKG_ATTR_PROTO_INFO          7
#define SECPKG_ATTR_PASSWORD_EXPIRY     8
#define SECPKG_ATTR_SESSION_KEY         9
#define SECPKG_ATTR_PACKAGE_INFO       10
#define SECPKG_ATTR_USER_FLAGS         11
#define SECPKG_ATTR_NEGOTIATION_INFO   12
#define SECPKG_ATTR_NATIVE_NAMES       13
#define SECPKG_ATTR_FLAGS              14
#define SECPKG_ATTR_USE_VALIDATED      15
#define SECPKG_ATTR_CREDENTIAL_NAME    16
#define SECPKG_ATTR_TARGET_INFORMATION 17
#define SECPKG_ATTR_ACCESS_TOKEN       18
#define SECPKG_ATTR_TARGET             19
#define SECPKG_ATTR_AUTHENTICATION_ID  20
#define SECPKG_ATTR_LOGOFF_TIME        21
#define SECPKG_ATTR_NEGO_KEYS          22
#define SECPKG_ATTR_PROMPTING_NEEDED   24
#define SECPKG_ATTR_UNIQUE_BINDINGS    25
#define SECPKG_ATTR_ENDPOINT_BINDINGS  26
#define SECPKG_ATTR_CLIENT_SPECIFIED_TARGET  27
#define SECPKG_ATTR_LAST_CLIENT_TOKEN_STATUS 30
#define SECPKG_ATTR_NEGO_PKG_INFO      31
#define SECPKG_ATTR_NEGO_STATUS        32
#define SECPKG_ATTR_CONTEXT_DELETED    33

#define SECPKG_ATTR_SUBJECT_SECURITY_ATTRIBUTES 128
#define SECPKG_ATTR_NEGO_INFO_FLAG_NO_KERBEROS 0x1
#define SECPKG_ATTR_NEGO_INFO_FLAG_NO_NTLM     0x2

/* types for QueryContextAttributes/SetContextAttributes */

typedef struct _SecPkgContext_Sizes
{
    ULONG cbMaxToken;
    ULONG cbMaxSignature;
    ULONG cbBlockSize;
    ULONG cbSecurityTrailer;
} SecPkgContext_Sizes, *PSecPkgContext_Sizes;

typedef struct _SecPkgContext_StreamSizes
{
    ULONG cbHeader;
    ULONG cbTrailer;
    ULONG cbMaximumMessage;
    ULONG cbBuffers;
    ULONG cbBlockSize;
} SecPkgContext_StreamSizes, *PSecPkgContext_StreamSizes;

typedef struct _SecPkgContext_NamesA
{
    SEC_CHAR *sUserName;
} SecPkgContext_NamesA, *PSecPkgContext_NamesA;

typedef struct _SecPkgContext_NamesW
{
    SEC_WCHAR *sUserName;
} SecPkgContext_NamesW, *PSecPkgContext_NamesW;

#define SecPkgContext_Names WINELIB_NAME_AW(SecPkgContext_Names)
#define PSecPkgContext_Names WINELIB_NAME_AW(PSecPkgContext_Names)

typedef struct _SecPkgContext_Lifespan
{
    TimeStamp tsStart;
    TimeStamp tsExpiry;
} SecPkgContext_Lifespan, *PSecPkgContext_Lifespan;

typedef struct _SecPkgContext_DceInfo
{
    ULONG AuthzSvc;
    void *pPac;
} SecPkgContext_DceInfo, *PSecPkgContext_DceInfo;

typedef struct _SecPkgContext_KeyInfoA
{
    SEC_CHAR      *sSignatureAlgorithmName;
    SEC_CHAR      *sEncryptAlgorithmName;
    ULONG  KeySize;
    ULONG  SignatureAlgorithm;
    ULONG  EncryptAlgorithm;
} SecPkgContext_KeyInfoA, *PSecPkgContext_KeyInfoA;

typedef struct _SecPkgContext_KeyInfoW
{
    SEC_WCHAR     *sSignatureAlgorithmName;
    SEC_WCHAR     *sEncryptAlgorithmName;
    ULONG  KeySize;
    ULONG  SignatureAlgorithm;
    ULONG  EncryptAlgorithm;
} SecPkgContext_KeyInfoW, *PSecPkgContext_KeyInfoW;

#define SecPkgContext_KeyInfo WINELIB_NAME_AW(SecPkgContext_KeyInfo)
#define PSecPkgContext_KeyInfo WINELIB_NAME_AW(PSecPkgContext_KeyInfo)

typedef struct _SecPkgContext_AuthorityA
{
    SEC_CHAR *sAuthorityName;
} SecPkgContext_AuthorityA, *PSecPkgContext_AuthorityA;

typedef struct _SecPkgContext_AuthorityW
{
    SEC_WCHAR *sAuthorityName;
} SecPkgContext_AuthorityW, *PSecPkgContext_AuthorityW;

#define SecPkgContext_Authority WINELIB_NAME_AW(SecPkgContext_Authority)
#define PSecPkgContext_Authority WINELIB_NAME_AW(PSecPkgContext_Authority)

typedef struct _SecPkgContext_ProtoInfoA
{
    SEC_CHAR     *sProtocolName;
    ULONG majorVersion;
    ULONG minorVersion;
} SecPkgContext_ProtoInfoA, *PSecPkgContext_ProtoInfoA;

typedef struct _SecPkgContext_ProtoInfoW
{
    SEC_WCHAR    *sProtocolName;
    ULONG majorVersion;
    ULONG minorVersion;
} SecPkgContext_ProtoInfoW, *PSecPkgContext_ProtoInfoW;

#define SecPkgContext_ProtoInfo WINELIB_NAME_AW(SecPkgContext_ProtoInfo)
#define PSecPkgContext_ProtoInfo WINELIB_NAME_AW(PSecPkgContext_ProtoInfo)

typedef struct _SecPkgContext_PasswordExpiry
{
    TimeStamp tsPasswordExpires;
} SecPkgContext_PasswordExpiry, *PSecPkgContext_PasswordExpiry;

typedef struct _SecPkgContext_SessionKey {
  ULONG SessionKeyLength;
  _Field_size_bytes_(SessionKeyLength) unsigned char *SessionKey;
} SecPkgContext_SessionKey, *PSecPkgContext_SessionKey;

typedef struct _SecPkgContext_PackageInfoA
{
    PSecPkgInfoA PackageInfo;
} SecPkgContext_PackageInfoA, *PSecPkgContext_PackageInfoA;

typedef struct _SecPkgContext_PackageInfoW
{
    PSecPkgInfoW PackageInfo;
} SecPkgContext_PackageInfoW, *PSecPkgContext_PackageInfoW;

#define SecPkgContext_PackageInfo WINELIB_NAME_AW(SecPkgContext_PackageInfo)
#define PSecPkgContext_PackageInfo WINELIB_NAME_AW(PSecPkgContext_PackageInfo)

typedef struct _SecPkgContext_Flags
{
    ULONG Flags;
} SecPkgContext_Flags, *PSecPkgContext_Flags;

typedef struct _SecPkgContext_UserFlags
{
    ULONG UserFlags;
} SecPkgContext_UserFlags, *PSecPkgContext_UserFlags;

typedef struct _SecPkgContext_NegotiationInfoA
{
    PSecPkgInfoA  PackageInfo;
    ULONG NegotiationState;
} SecPkgContext_NegotiationInfoA, *PSecPkgContext_NegotiationInfoA;

typedef struct _SecPkgContext_NegotiationInfoW
{
    PSecPkgInfoW  PackageInfo;
    ULONG NegotiationState;
} SecPkgContext_NegotiationInfoW, *PSecPkgContext_NegotiationInfoW;

#define SecPkgContext_NegotiationInfo WINELIB_NAME_AW(SecPkgContext_NegotiationInfo)
#define PSecPkgContext_NegotiationInfo WINELIB_NAME_AW(PSecPkgContext_NegotiationInfo)

/* values for NegotiationState */
#define SECPKG_NEGOTIATION_COMPLETE      0
#define SECPKG_NEGOTIATION_OPTIMISTIC    1
#define SECPKG_NEGOTIATION_IN_PROGRESS   2
#define SECPKG_NEGOTIATION_DIRECT        3
#define SECPKG_NEGOTIATION_TRY_MULTICRED 4

typedef struct _SecPkgContext_NativeNamesA
{
    SEC_CHAR *sClientName;
    SEC_CHAR *sServerName;
} SecPkgContext_NativeNamesA, *PSecPkgContext_NativeNamesA;

typedef struct _SecPkgContext_NativeNamesW
{
    SEC_WCHAR *sClientName;
    SEC_WCHAR *sServerName;
} SecPkgContext_NativeNamesW, *PSecPkgContext_NativeNamesW;

#define SecPkgContext_NativeNames WINELIB_NAME_AW(SecPkgContext_NativeNames)
#define PSecPkgContext_NativeNames WINELIB_NAME_AW(PSecPkgContext_NativeNames)

typedef struct _SecPkgContext_CredentialNameA
{
    ULONG  CredentialType;
    SEC_CHAR      *sCredentialName;
} SecPkgContext_CredentialNameA, *PSecPkgContext_CredentialNameA;

typedef struct _SecPkgContext_CredentialNameW
{
    ULONG  CredentialType;
    SEC_WCHAR     *sCredentialName;
} SecPkgContext_CredentialNameW, *PSecPkgContext_CredentialNameW;

#define SecPkgContext_CredentialName WINELIB_NAME_AW(SecPkgContext_CredentialName)
#define PSecPkgContext_CredentialName WINELIB_NAME_AW(PSecPkgContext_CredentialName)

typedef struct _SecPkgContext_AccessToken
{
    void *AccessToken;
} SecPkgContext_AccessToken, *PSecPkgContext_AccessToken;

typedef struct _SecPkgContext_TargetInformation
{
    ULONG  MarshalledTargetInfoLength;
    unsigned char *MarshalledTargetInfo;
} SecPkgContext_TargetInformation, *PSecPkgContext_TargetInformation;

typedef struct _SecPkgContext_AuthzID
{
    ULONG  AuthzIDLength;
    char          *AuthzID;
} SecPkgContext_AuthzID, *PSecPkgContext_AuthzID;

typedef struct _SecPkgContext_Target
{
    ULONG  TargetLength;
    char          *Target;
} SecPkgContext_Target, *PSecPkgContext_Target;

typedef struct _SecPkgContext_Bindings
{
    ULONG BindingsLength;
    SEC_CHANNEL_BINDINGS *Bindings;
} SecPkgContext_Bindings, *PSecPkgContext_Bindings;

_Check_return_
SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(
  _In_ PCtxtHandle phContext);

typedef SECURITY_STATUS (SEC_ENTRY *IMPERSONATE_SECURITY_CONTEXT_FN)
 (PCtxtHandle);

SECURITY_STATUS SEC_ENTRY RevertSecurityContext(_In_ PCtxtHandle phContext);

typedef SECURITY_STATUS (SEC_ENTRY *REVERT_SECURITY_CONTEXT_FN)(PCtxtHandle);

SECURITY_STATUS
SEC_ENTRY
MakeSignature(
  _In_ PCtxtHandle phContext,
  _In_ ULONG fQOP,
  _In_ PSecBufferDesc pMessage,
  _In_ ULONG MessageSeqNo);

typedef SECURITY_STATUS (SEC_ENTRY *MAKE_SIGNATURE_FN)(PCtxtHandle,
 ULONG, PSecBufferDesc, ULONG);

SECURITY_STATUS
SEC_ENTRY
VerifySignature(
  _In_ PCtxtHandle phContext,
  _In_ PSecBufferDesc pMessage,
  _In_ ULONG MessageSeqNo,
  _Out_ PULONG pfQOP);

typedef SECURITY_STATUS (SEC_ENTRY *VERIFY_SIGNATURE_FN)(PCtxtHandle,
 PSecBufferDesc, ULONG, PULONG);

SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoA(
  _In_ SEC_CHAR *pszPackageName,
  _Outptr_ PSecPkgInfoA *ppPackageInfo);

SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoW(
  _In_ SEC_WCHAR *pszPackageName,
  _Outptr_ PSecPkgInfoW *ppPackageInfo);

#define QuerySecurityPackageInfo WINELIB_NAME_AW(QuerySecurityPackageInfo)

typedef SECURITY_STATUS (SEC_ENTRY *QUERY_SECURITY_PACKAGE_INFO_FN_A)
 (SEC_CHAR *, PSecPkgInfoA *);
typedef SECURITY_STATUS (SEC_ENTRY *QUERY_SECURITY_PACKAGE_INFO_FN_W)
 (SEC_WCHAR *, PSecPkgInfoW *);
#define QUERY_SECURITY_PACKAGE_INFO_FN WINELIB_NAME_AW(QUERY_SECURITY_PACKAGE_INFO_FN_)

SECURITY_STATUS
SEC_ENTRY
ExportSecurityContext(
  _In_ PCtxtHandle phContext,
  _In_ ULONG fFlags,
  _Out_ PSecBuffer pPackedContext,
  _Out_ void **pToken);

typedef SECURITY_STATUS (SEC_ENTRY *EXPORT_SECURITY_CONTEXT_FN)(PCtxtHandle,
 ULONG, PSecBuffer, void **);

/* values for ExportSecurityContext fFlags */
#define SECPKG_CONTEXT_EXPORT_RESET_NEW  0x00000001
#define SECPKG_CONTEXT_EXPORT_DELETE_OLD 0x00000002

SECURITY_STATUS
SEC_ENTRY
ImportSecurityContextA(
  _In_ SEC_CHAR *pszPackage,
  _In_ PSecBuffer pPackedContext,
  _In_ void *Token,
  _Out_ PCtxtHandle phContext);

SECURITY_STATUS
SEC_ENTRY
ImportSecurityContextW(
  _In_ SEC_WCHAR *pszPackage,
  _In_ PSecBuffer pPackedContext,
  _In_ void *Token,
  _Out_ PCtxtHandle phContext);

#define ImportSecurityContext WINELIB_NAME_AW(ImportSecurityContext)

typedef SECURITY_STATUS (SEC_ENTRY *IMPORT_SECURITY_CONTEXT_FN_A)(SEC_CHAR *,
 PSecBuffer, void *, PCtxtHandle);
typedef SECURITY_STATUS (SEC_ENTRY *IMPORT_SECURITY_CONTEXT_FN_W)(SEC_WCHAR *,
 PSecBuffer, void *, PCtxtHandle);
#define IMPORT_SECURITY_CONTEXT_FN WINELIB_NAME_AW(IMPORT_SECURITY_CONTEXT_FN_)

SECURITY_STATUS
SEC_ENTRY
AddCredentialsA(
  _In_ PCredHandle hCredentials,
  _In_opt_ SEC_CHAR *pszPrincipal,
  _In_ SEC_CHAR *pszPackage,
  _In_ ULONG fCredentialUse,
  _In_opt_ void *pAuthData,
  _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
  _In_opt_ void *pvGetKeyArgument,
  _Out_opt_ PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
AddCredentialsW(
  _In_ PCredHandle hCredentials,
  _In_opt_ SEC_WCHAR *pszPrincipal,
  _In_ SEC_WCHAR *pszPackage,
  _In_ ULONG fCredentialUse,
  _In_opt_ void *pAuthData,
  _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
  _In_opt_ void *pvGetKeyArgument,
  _Out_opt_ PTimeStamp ptsExpiry);

#define AddCredentials WINELIB_NAME_AW(AddCredentials)

typedef SECURITY_STATUS (SEC_ENTRY *ADD_CREDENTIALS_FN_A)(PCredHandle,
 SEC_CHAR *, SEC_CHAR *, ULONG, void *, SEC_GET_KEY_FN, void *,
 PTimeStamp);
typedef SECURITY_STATUS (SEC_ENTRY *ADD_CREDENTIALS_FN_W)(PCredHandle,
 SEC_WCHAR *, SEC_WCHAR *, ULONG, void *, SEC_GET_KEY_FN, void *,
 PTimeStamp);

SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
  _In_ PCtxtHandle phContext,
  _Out_ HANDLE *phToken);

typedef SECURITY_STATUS (SEC_ENTRY *QUERY_SECURITY_CONTEXT_TOKEN_FN)
 (PCtxtHandle, HANDLE *);

SECURITY_STATUS
SEC_ENTRY
EncryptMessage(
  _In_ PCtxtHandle phContext,
  _In_ ULONG fQOP,
  _In_ PSecBufferDesc pMessage,
  _In_ ULONG MessageSeqNo);

SECURITY_STATUS
SEC_ENTRY
DecryptMessage(
  _In_ PCtxtHandle phContext,
  _In_ PSecBufferDesc pMessage,
  _In_ ULONG MessageSeqNo,
  _Out_opt_ PULONG pfQOP);

/* values for EncryptMessage fQOP */
#define SECQOP_WRAP_NO_ENCRYPT 0x80000001

typedef SECURITY_STATUS (SEC_ENTRY *ENCRYPT_MESSAGE_FN)(PCtxtHandle, ULONG,
 PSecBufferDesc, ULONG);
typedef SECURITY_STATUS (SEC_ENTRY *DECRYPT_MESSAGE_FN)(PCtxtHandle,
 PSecBufferDesc, ULONG, PULONG);

SECURITY_STATUS
SEC_ENTRY
SetContextAttributesA(
  _In_ PCtxtHandle phContext,
  _In_ ULONG ulAttribute,
  _In_reads_bytes_(cbBuffer) void *pBuffer,
  _In_ ULONG cbBuffer);

SECURITY_STATUS
SEC_ENTRY
SetContextAttributesW(
  _In_ PCtxtHandle phContext,
  _In_ ULONG ulAttribute,
  _In_reads_bytes_(cbBuffer) void *pBuffer,
  _In_ ULONG cbBuffer);

#define SetContextAttributes WINELIB_NAME_AW(SetContextAttributes)

typedef SECURITY_STATUS (SEC_ENTRY *SET_CONTEXT_ATTRIBUTES_FN_A)(PCtxtHandle,
 ULONG, void *, ULONG);
typedef SECURITY_STATUS (SEC_ENTRY *SET_CONTEXT_ATTRIBUTES_FN_W)(PCtxtHandle,
 ULONG, void *, ULONG);

#define SECURITY_ENTRYPOINT_ANSIA "InitSecurityInterfaceA"
#define SECURITY_ENTRYPOINT_ANSIW "InitSecurityInterfaceW"
#define SECURITY_ENTRYPOINT_ANSI WINELIB_NAME_AW(SECURITY_ENTRYPOINT_ANSI)

typedef struct _SECURITY_FUNCTION_TABLE_A
{
    ULONG                     dwVersion;
    ENUMERATE_SECURITY_PACKAGES_FN_A  EnumerateSecurityPackagesA;
    QUERY_CREDENTIALS_ATTRIBUTES_FN_A QueryCredentialsAttributesA;
    ACQUIRE_CREDENTIALS_HANDLE_FN_A   AcquireCredentialsHandleA;
    FREE_CREDENTIALS_HANDLE_FN        FreeCredentialsHandle;
    void                             *Reserved2;
    INITIALIZE_SECURITY_CONTEXT_FN_A  InitializeSecurityContextA;
    ACCEPT_SECURITY_CONTEXT_FN        AcceptSecurityContext;
    COMPLETE_AUTH_TOKEN_FN            CompleteAuthToken;
    DELETE_SECURITY_CONTEXT_FN        DeleteSecurityContext;
    APPLY_CONTROL_TOKEN_FN            ApplyControlToken;
    QUERY_CONTEXT_ATTRIBUTES_FN_A     QueryContextAttributesA;
    IMPERSONATE_SECURITY_CONTEXT_FN   ImpersonateSecurityContext;
    REVERT_SECURITY_CONTEXT_FN        RevertSecurityContext;
    MAKE_SIGNATURE_FN                 MakeSignature;
    VERIFY_SIGNATURE_FN               VerifySignature;
    FREE_CONTEXT_BUFFER_FN            FreeContextBuffer;
    QUERY_SECURITY_PACKAGE_INFO_FN_A  QuerySecurityPackageInfoA;
    void                             *Reserved3;
    void                             *Reserved4;
    EXPORT_SECURITY_CONTEXT_FN        ExportSecurityContext;
    IMPORT_SECURITY_CONTEXT_FN_A      ImportSecurityContextA;
    ADD_CREDENTIALS_FN_A              AddCredentialsA;
    void                             *Reserved8;
    QUERY_SECURITY_CONTEXT_TOKEN_FN   QuerySecurityContextToken;
    ENCRYPT_MESSAGE_FN                EncryptMessage;
    DECRYPT_MESSAGE_FN                DecryptMessage;
    SET_CONTEXT_ATTRIBUTES_FN_A       SetContextAttributesA;
} SecurityFunctionTableA, *PSecurityFunctionTableA;

/* No, it really is FreeCredentialsHandle, see the thread beginning
 * http://sourceforge.net/mailarchive/message.php?msg_id=4321080 for a
 * discovery discussion. */
typedef struct _SECURITY_FUNCTION_TABLE_W
{
    ULONG                     dwVersion;
    ENUMERATE_SECURITY_PACKAGES_FN_W  EnumerateSecurityPackagesW;
    QUERY_CREDENTIALS_ATTRIBUTES_FN_W QueryCredentialsAttributesW;
    ACQUIRE_CREDENTIALS_HANDLE_FN_W   AcquireCredentialsHandleW;
    FREE_CREDENTIALS_HANDLE_FN        FreeCredentialsHandle;
    void                             *Reserved2;
    INITIALIZE_SECURITY_CONTEXT_FN_W  InitializeSecurityContextW;
    ACCEPT_SECURITY_CONTEXT_FN        AcceptSecurityContext;
    COMPLETE_AUTH_TOKEN_FN            CompleteAuthToken;
    DELETE_SECURITY_CONTEXT_FN        DeleteSecurityContext;
    APPLY_CONTROL_TOKEN_FN            ApplyControlToken;
    QUERY_CONTEXT_ATTRIBUTES_FN_W     QueryContextAttributesW;
    IMPERSONATE_SECURITY_CONTEXT_FN   ImpersonateSecurityContext;
    REVERT_SECURITY_CONTEXT_FN        RevertSecurityContext;
    MAKE_SIGNATURE_FN                 MakeSignature;
    VERIFY_SIGNATURE_FN               VerifySignature;
    FREE_CONTEXT_BUFFER_FN            FreeContextBuffer;
    QUERY_SECURITY_PACKAGE_INFO_FN_W  QuerySecurityPackageInfoW;
    void                             *Reserved3;
    void                             *Reserved4;
    EXPORT_SECURITY_CONTEXT_FN        ExportSecurityContext;
    IMPORT_SECURITY_CONTEXT_FN_W      ImportSecurityContextW;
    ADD_CREDENTIALS_FN_W              AddCredentialsW;
    void                             *Reserved8;
    QUERY_SECURITY_CONTEXT_TOKEN_FN   QuerySecurityContextToken;
    ENCRYPT_MESSAGE_FN                EncryptMessage;
    DECRYPT_MESSAGE_FN                DecryptMessage;
    SET_CONTEXT_ATTRIBUTES_FN_W       SetContextAttributesW;
} SecurityFunctionTableW, *PSecurityFunctionTableW;

#define SecurityFunctionTable WINELIB_NAME_AW(SecurityFunctionTable)
#define PSecurityFunctionTable WINELIB_NAME_AW(PSecurityFunctionTable)

#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION   1
#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2 2

PSecurityFunctionTableA SEC_ENTRY InitSecurityInterfaceA(void);
PSecurityFunctionTableW SEC_ENTRY InitSecurityInterfaceW(void);
#define InitSecurityInterface WINELIB_NAME_AW(InitSecurityInterface)

typedef PSecurityFunctionTableA (SEC_ENTRY *INIT_SECURITY_INTERFACE_A)(void);
typedef PSecurityFunctionTableW (SEC_ENTRY *INIT_SECURITY_INTERFACE_W)(void);
#define INIT_SECURITY_INTERFACE WINELIB_NAME_AW(INIT_SECURITY_INTERFACE_)

#ifdef __cplusplus
}
#endif

#endif /* ndef __WINE_SSPI_H__ */
