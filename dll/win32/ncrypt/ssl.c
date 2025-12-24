/* this is a completel messy cluster fuck that i dont really know how it this ended up working
 * horrible code, but good for now
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ncrypt.h"
#include "bcrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ncrypt);

#ifndef BCRYPT_ALG_HANDLE_HMAC_FLAG
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008
#endif
#ifndef BCRYPT_INIT_AUTH_MODE_INFO
#define BCRYPT_INIT_AUTH_MODE_INFO(a) do { ZeroMemory(&(a), sizeof(a)); (a).cbSize = sizeof(a); (a).dwInfoVersion = 1; } while(0)
#endif
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

/* Fallback defines for BCrypt constants that may be missing on older headers/toolchains */
#ifndef BCRYPT_ECDH_ALGORITHM
#define BCRYPT_ECDH_ALGORITHM             L"ECDH"
#endif
#ifndef BCRYPT_ECDH_P256_ALGORITHM
#define BCRYPT_ECDH_P256_ALGORITHM        L"ECDH_P256"
#endif
#ifndef BCRYPT_KDF_RAW_SECRET
#define BCRYPT_KDF_RAW_SECRET             L"RAW_SECRET"
#endif
#ifndef BCRYPT_ECCPUBLIC_BLOB
#define BCRYPT_ECCPUBLIC_BLOB             L"ECCPUBLICBLOB"
#endif
#ifndef BCRYPT_ECCPRIVATE_BLOB
#define BCRYPT_ECCPRIVATE_BLOB            L"ECCPRIVATEBLOB"
#endif
#ifndef BCRYPT_ECC_CURVE_NAME
#define BCRYPT_ECC_CURVE_NAME              L"ECCCurveName"
#endif
#ifndef BCRYPT_ECC_CURVE_NISTP256
#define BCRYPT_ECC_CURVE_NISTP256          L"nistP256"
#endif
#ifndef BCRYPT_ECC_CURVE_NISTP384
#define BCRYPT_ECC_CURVE_NISTP384          L"nistP384"
#endif
#ifndef BCRYPT_SHA384_ALGORITHM
#define BCRYPT_SHA384_ALGORITHM             L"SHA384"
#endif

/* Forward declarations needed for function table initializer */
int WINAPI SslEnumCipherSuites(ULONG_PTR hSslProvider, ULONG_PTR hPrivateKey,
                               void **ppCipherSuite, void **ppEnumState, unsigned int dwFlags);
int WINAPI SslLookupCipherSuiteInfo(ULONG_PTR hSslProvider, unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwKeyType, void *pCipherSuite, unsigned int dwFlags);
int WINAPI SslLookupCipherLengths(ULONG_PTR hSslProvider, unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwKeyType, void *pCipherLengths, unsigned int cbCipherLengths, unsigned int dwFlags);

/* Lightweight opaque handle headers mirror reference fields: */
typedef struct ssl_provider
{
    DWORD cb;                /* >= 0x98 in reference; we use size for sanity */
    DWORD magic;             /* 1145324609 in ref; arbitrary for sanity */
    DWORD flags;             /* unused */
    DWORD refcount;          /* interlocked */
    /* Pointers to underlying dispatch not modeled; we proxy to NCrypt/BCrypt where possible */
    const void *ftbl;        /* minimal function table pointer for introspection */
} ssl_provider;

typedef struct ssl_key
{
    DWORD cb;                /* 0x18 in reference */
    DWORD magic;             /* 1145324610 in ref for key; 1145324611 for hash */
    DWORD reserved0;
    DWORD refcount;
    ssl_provider *prov;
    NCRYPT_KEY_HANDLE hKey;  /* For NCrypt or BCrypt (both are HANDLE typedefs) */
    BOOL is_bcrypt;          /* TRUE when hKey is a BCrypt key */
} ssl_key;

typedef struct ssl_hash
{
    DWORD cb;
    DWORD magic;             /* 1145324611 for hash */
    DWORD reserved0;
    DWORD refcount;
    ssl_provider *prov;
    BCRYPT_HASH_HANDLE hHash; /* Handshake hash */
} ssl_hash;

/* Additional opaque types for secrets and symmetric keys */
typedef struct ssl_secret
{
    DWORD cb;
    DWORD magic;             /* 1145324612 for secret */
    DWORD reserved0;
    DWORD refcount;
    ssl_provider *prov;
    BYTE *data;
    DWORD len;
} ssl_secret;

typedef struct ssl_symkey
{
    DWORD cb;
    DWORD magic;             /* 1145324613 for symmetric key */
    DWORD reserved0;
    DWORD refcount;
    ssl_provider *prov;
    DWORD suite;             /* TLS cipher suite id */
    BYTE key[32];            /* up to AES-256 */
    DWORD key_len;
    BYTE salt[8];            /* implicit part of nonce for AEAD */
    DWORD salt_len;
    BYTE mac_key[32];        /* up to SHA-256 MAC key (we use up to 20 for SHA1) */
    DWORD mac_len;           /* MAC length in bytes (e.g., 20 for SHA1) */
    DWORD iv_len;            /* explicit IV length for CBC (16 for AES-CBC in TLS1.2) */
    DWORD block_len;         /* block size (16 for AES) */
} ssl_symkey;

/* Minimal function table type, akin to _NCRYPT_SSL_FUNCTION_TABLE; private to this module */
typedef struct ssl_function_table
{
    USHORT ver_major, ver_minor;
    void *pEnumCipherSuites;
    void *pLookupCipherSuiteInfo;
    void *pLookupCipherLengths;
} ssl_function_table;

/* Cipher suite metadata (subset) */
#define TLS1_2_VERSION 0x0303
#define TLS_RSA_WITH_AES_128_CBC_SHA        0x002F
#define TLS_RSA_WITH_AES_256_CBC_SHA        0x0035
#define TLS_RSA_WITH_AES_128_CBC_SHA256     0x003C
#define TLS_RSA_WITH_AES_256_CBC_SHA256     0x003D
#define TLS_RSA_WITH_AES_128_GCM_SHA256     0x009C
#define TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 0xC02F
#define TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 0xC030
/* Optional future: #define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 0xC027 */

typedef struct _NCRYPT_SSL_CIPHER_SUITE_LOCAL
{
    unsigned int dwProtocol;
    unsigned int dwCipherSuite;
    unsigned int dwBaseCipherSuite;
    WCHAR szCipherSuite[64];
    WCHAR szCipher[64];
    unsigned int dwCipherLen;       /* key length in bits */
    unsigned int dwCipherBlockLen;  /* block or tag size */
    WCHAR szHash[64];
    unsigned int dwHashLen;         /* hash length in bytes */
    WCHAR szExchange[64];
    unsigned int dwMinExchangeLen;
    unsigned int dwMaxExchangeLen;
    WCHAR szCertificate[64];
    unsigned int dwKeyType;         /* 0=RSA */
} NCRYPT_SSL_CIPHER_SUITE_LOCAL;

typedef struct _NCRYPT_SSL_CIPHER_LENGTHS_LOCAL
{
    unsigned int cbLength;
    unsigned int dwHeaderLen;
    unsigned int dwFixedTrailerLen;
    unsigned int dwMaxVariableTrailerLen;
    unsigned int dwFlags;
} NCRYPT_SSL_CIPHER_LENGTHS_LOCAL;

/* Local ECC curve info structure for SslEnumEccCurves */
typedef struct _NCRYPT_SSL_ECC_CURVE_INFO_LOCAL
{
    WCHAR szCurve[64];      /* Friendly/CNG curve name */
    unsigned int dwBits;    /* Curve size in bits */
    unsigned int dwFlags;   /* Reserved */
} NCRYPT_SSL_ECC_CURVE_INFO_LOCAL;

typedef struct suite_meta
{
    DWORD id;
    const WCHAR *name;
    const WCHAR *cipher_name;
    DWORD key_bits;
    DWORD block_len;   /* 16 for AES, 16 tag for GCM */
    const WCHAR *hash_name; /* L"SHA" or L"SHA256" or L"SHA384" */
    DWORD hash_len;    /* 20 for SHA1, 32 for SHA256, 48 for SHA384 */
    const WCHAR *kx;   /* L"RSA" or L"ECDHE_RSA" */
    DWORD kx_min;
    DWORD kx_max;
    const WCHAR *cert;
    BOOL is_aead;
} suite_meta;

static const suite_meta g_suites[] =
{
    { TLS_RSA_WITH_AES_128_CBC_SHA,        L"TLS_RSA_WITH_AES_128_CBC_SHA",        L"AES", 128, 16, L"SHA",    20, L"RSA",        1024, 16384, L"RSA", FALSE },
    { TLS_RSA_WITH_AES_256_CBC_SHA,        L"TLS_RSA_WITH_AES_256_CBC_SHA",        L"AES", 256, 16, L"SHA",    20, L"RSA",        1024, 16384, L"RSA", FALSE },
    { TLS_RSA_WITH_AES_128_CBC_SHA256,     L"TLS_RSA_WITH_AES_128_CBC_SHA256",     L"AES", 128, 16, L"SHA256", 32, L"RSA",        1024, 16384, L"RSA", FALSE },
    { TLS_RSA_WITH_AES_256_CBC_SHA256,     L"TLS_RSA_WITH_AES_256_CBC_SHA256",     L"AES", 256, 16, L"SHA256", 32, L"RSA",        1024, 16384, L"RSA", FALSE },
    { TLS_RSA_WITH_AES_128_GCM_SHA256,     L"TLS_RSA_WITH_AES_128_GCM_SHA256",     L"AES-GCM", 128, 16, L"SHA256", 32, L"RSA",        1024, 16384, L"RSA", TRUE },
    { TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, L"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256", L"AES-GCM", 128, 16, L"SHA256", 32, L"ECDHE_RSA", 256,   521,   L"RSA", TRUE },
    { TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, L"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384", L"AES-GCM", 256, 16, L"SHA384", 48, L"ECDHE_RSA", 256,   521,   L"RSA", TRUE },
};

static const suite_meta *find_suite(DWORD id)
{
    unsigned int i;
    for (i = 0; i < sizeof(g_suites)/sizeof(g_suites[0]); ++i)
        if (g_suites[i].id == id) return &g_suites[i];
    return NULL;
}

/* KDF helpers: TLS 1.2 PRF with HMAC using SHA256 or SHA384 */
static NTSTATUS hmac_hash(const WCHAR *algId, const BYTE *key, DWORD key_len, const BYTE *data, DWORD data_len, BYTE *out, DWORD out_len)
{
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_HASH_HANDLE hash = NULL; NTSTATUS status;
    status = BCryptOpenAlgorithmProvider(&alg, algId, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status) return status;
    status = BCryptCreateHash(alg, &hash, NULL, 0, (PUCHAR)key, key_len, 0);
    if (!status) status = BCryptHashData(hash, (PUCHAR)data, data_len, 0);
    if (!status) status = BCryptFinishHash(hash, out, out_len, 0);
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return status;
}

/* HMAC helpers for CBC MAC */
static NTSTATUS hmac_sha1(const BYTE *key, DWORD key_len, const BYTE *data, DWORD data_len, BYTE *out, DWORD out_len)
{
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_HASH_HANDLE hash = NULL; NTSTATUS status;
    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status) return status;
    status = BCryptCreateHash(alg, &hash, NULL, 0, (PUCHAR)key, key_len, 0);
    if (!status) status = BCryptHashData(hash, (PUCHAR)data, data_len, 0);
    if (!status) status = BCryptFinishHash(hash, out, out_len, 0);
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return status;
}

static NTSTATUS hmac_sha256_mac(const BYTE *key, DWORD key_len, const BYTE *data, DWORD data_len, BYTE *out, DWORD out_len)
{
    return hmac_hash(BCRYPT_SHA256_ALGORITHM, key, key_len, data, data_len, out, out_len);
}

static BOOL const_time_eq(const BYTE *a, const BYTE *b, SIZE_T len)
{
    SIZE_T i; BYTE v = 0;
    for (i = 0; i < len; ++i) v |= (a[i] ^ b[i]);
    return v == 0;
}

static NTSTATUS tls12_prf(const WCHAR *hashAlg, DWORD hashLen,
                          const BYTE *secret, DWORD secret_len,
                          const char *label, const BYTE *seed, DWORD seed_len,
                          BYTE *out, DWORD out_len)
{
    BYTE a[64];
    BYTE buf[64];
    NTSTATUS status;
    DWORD produced = 0;
    DWORD label_len = (DWORD)strlen(label);
    BYTE *seedbuf = NULL;
    DWORD seedbuf_len = label_len + seed_len;
    seedbuf = HeapAlloc(GetProcessHeap(), 0, seedbuf_len);
    if (!seedbuf) return STATUS_NO_MEMORY;
    memcpy(seedbuf, label, label_len);
    if (seed_len) memcpy(seedbuf + label_len, seed, seed_len);

    /* A(1) = HMAC(secret, seed) */
    status = hmac_hash(hashAlg, secret, secret_len, seedbuf, seedbuf_len, a, hashLen);
    if (status) goto done;
    while (produced < out_len)
    {
        /* HMAC(secret, A(i) + seed) */
        BYTE *tmp = HeapAlloc(GetProcessHeap(), 0, hashLen + seedbuf_len);
        DWORD to_copy;
        if (!tmp) { status = STATUS_NO_MEMORY; goto done; }
        memcpy(tmp, a, hashLen);
        memcpy(tmp + hashLen, seedbuf, seedbuf_len);
        status = hmac_hash(hashAlg, secret, secret_len, tmp, hashLen + seedbuf_len, buf, hashLen);
        HeapFree(GetProcessHeap(), 0, tmp);
        if (status) goto done;
        to_copy = min(hashLen, out_len - produced);
        memcpy(out + produced, buf, to_copy);
        produced += to_copy;
        /* A(i+1) = HMAC(secret, A(i)) */
        status = hmac_hash(hashAlg, secret, secret_len, a, hashLen, a, hashLen);
        if (status) goto done;
    }
    status = STATUS_SUCCESS;
done:
    SecureZeroMemory(a, sizeof(a));
    SecureZeroMemory(buf, sizeof(buf));
    if (seedbuf) HeapFree(GetProcessHeap(), 0, seedbuf);
    return status;
}

/* Local buffer type ids (best-effort) */
#define NCRYPTBUFFER_SSL_CLIENT_RANDOM  20
#define NCRYPTBUFFER_SSL_SERVER_RANDOM  21
#define NCRYPTBUFFER_SSL_CIPHER_SUITE   50
/* Local flag for SslGenerateSessionKeys: when set, derive keys for client role (write=client, read=server). */
#define SSLGENKEYS_FLAG_CLIENT          0x00000001

static BOOL get_param_buffer(BCryptBufferDesc *desc, ULONG type, PUCHAR *data, ULONG *len)
{
    ULONG i;
    if (!desc || !desc->pBuffers) return FALSE;
    for (i = 0; i < desc->cBuffers; ++i)
    {
        if (desc->pBuffers[i].BufferType == type)
        {
            if (data) *data = desc->pBuffers[i].pvBuffer;
            if (len) *len = desc->pBuffers[i].cbBuffer;
            return TRUE;
        }
    }
    return FALSE;
}

/* Be tolerant to differing suite encodings: accept 2-byte or 4-byte values and try LE/BE. */
static DWORD parse_cipher_suite(const PUCHAR suitep, ULONG suitel)
{
    DWORD cand = 0;
    if (!suitep || suitel == 0) return 0;
    if (suitel >= sizeof(DWORD))
    {
        /* Try low 16 bits first (common), then high 16 bits if needed */
        DWORD v = 0; memcpy(&v, suitep, sizeof(DWORD));
        cand = (v & 0xFFFF);
        if (find_suite(cand)) return cand;
        cand = ((v >> 16) & 0xFFFF);
        if (find_suite(cand)) return cand;
        /* Try byte-swapped forms */
        cand = ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
        if (find_suite(cand)) return cand;
        cand = ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00);
        if (find_suite(cand)) return cand;
    }
    if (suitel >= sizeof(USHORT))
    {
        USHORT s = 0; memcpy(&s, suitep, sizeof(USHORT));
        cand = s;
        if (find_suite(cand)) return cand;
        /* Try big-endian */
        cand = ((s & 0xFF) << 8) | (s >> 8);
        if (find_suite(cand)) return cand;
    }
    return 0;
}

static inline BOOL is_valid_provider(ssl_provider *p)
{
    return p && p->cb >= sizeof(*p) && p->magic == 1145324609 && !p->flags;
}

static inline BOOL is_valid_key(ssl_key *k)
{
    return k && k->cb >= sizeof(*k) && k->magic == 1145324610;
}

static inline BOOL is_valid_hash(ssl_hash *h)
{
    return h && h->cb >= sizeof(*h) && h->magic == 1145324611;
}

static SECURITY_STATUS map_ntstatus(NTSTATUS status)
{
    switch (status)
    {
    case STATUS_SUCCESS:           return ERROR_SUCCESS;
    case STATUS_INVALID_PARAMETER: return NTE_INVALID_PARAMETER;
    case STATUS_NO_MEMORY:         return NTE_NO_MEMORY;
    case STATUS_NOT_SUPPORTED:     return NTE_NOT_SUPPORTED;
    case STATUS_INVALID_HANDLE:    return NTE_INVALID_HANDLE;
    case STATUS_BUFFER_TOO_SMALL:  return NTE_BUFFER_TOO_SMALL;
    default:                       return NTE_FAIL;
    }
}

/* Provider ref helpers */
static void prov_addref(ssl_provider *p)
{
    if (p) InterlockedIncrement((LONG *)&p->refcount);
}

static void prov_release(ssl_provider *p)
{
    if (p && InterlockedDecrement((LONG *)&p->refcount) <= 0)
    {
        HeapFree(GetProcessHeap(), 0, p);
    }
}

/* Exported Ssl* functions */

int WINAPI SslOpenProvider(ULONG_PTR *phSslProvider, const WCHAR *pszProviderName, unsigned int dwFlags)
{
    ssl_provider *p;
    static const ssl_function_table tbl = { 1, 0, (void *)SslEnumCipherSuites, (void *)SslLookupCipherSuiteInfo, (void *)SslLookupCipherLengths };
    TRACE("SslOpenProvider(%p, %s, %#x)\n", phSslProvider, debugstr_w(pszProviderName), dwFlags);
    if (!phSslProvider || dwFlags) return STATUS_INVALID_PARAMETER;

    /* Reference behavior: unknown provider name -> STATUS_NOT_FOUND. Accept NULL (default),
       our shim name, and the Windows provider name for compatibility. */
    if (pszProviderName &&
        lstrcmpiW(pszProviderName, L"ReactOS-SSL") != 0 &&
        lstrcmpiW(pszProviderName, L"Microsoft SSL Protocol Provider") != 0)
    {
        return STATUS_NOT_FOUND;
    }

    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*p));
    if (!p) return STATUS_NO_MEMORY;
    p->cb = sizeof(*p);
    p->magic = 1145324609;
    p->refcount = 1;
    p->ftbl = &tbl;
    *phSslProvider = (ULONG_PTR)p;
    return STATUS_SUCCESS;
}

int WINAPI SslIncrementProviderReferenceCount(ULONG_PTR hSslProvider)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    TRACE("SslIncrementProviderReferenceCount(%p)\n", p);
    if (!is_valid_provider(p)) return STATUS_INVALID_HANDLE;
    prov_addref(p);
    return STATUS_SUCCESS;
}

int WINAPI SslDecrementProviderReferenceCount(ULONG_PTR hSslProvider)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    TRACE("SslDecrementProviderReferenceCount(%p)\n", p);
    if (!is_valid_provider(p)) return STATUS_INVALID_HANDLE;
    prov_release(p);
    return STATUS_SUCCESS;
}

int WINAPI SslFreeBuffer(void *pv)
{
    TRACE("SslFreeBuffer(%p)\n", pv);
    if (pv) HeapFree(GetProcessHeap(), 0, pv);
    return STATUS_SUCCESS;
}

int WINAPI SslFreeObject(ULONG_PTR hObject, unsigned int dwFlags)
{
    void *obj = (void *)hObject;
    DWORD *hdr = (DWORD *)obj;
    TRACE("SslFreeObject(%p, %#x)\n", obj, dwFlags);
    if (!obj || hdr[0] < 0x18) return STATUS_INVALID_HANDLE;
    /* free key/hash wrapper; release provider ref */
    if (hdr[1] == 1145324610)
    {
        ssl_key *k = (ssl_key *)obj;
        if (k->hKey)
        {
            if (k->is_bcrypt) BCryptDestroyKey((BCRYPT_KEY_HANDLE)k->hKey);
            else NCryptFreeObject((NCRYPT_HANDLE)k->hKey);
        }
        prov_release(k->prov);
        HeapFree(GetProcessHeap(), 0, k);
        return STATUS_SUCCESS;
    }
    if (hdr[1] == 1145324611)
    {
        ssl_hash *h = (ssl_hash *)obj;
        if (h->hHash) BCryptDestroyHash(h->hHash);
        prov_release(h->prov);
        HeapFree(GetProcessHeap(), 0, h);
        return STATUS_SUCCESS;
    }
    if (hdr[1] == 1145324612)
    {
        ssl_secret *s = (ssl_secret *)obj;
        if (s->data) { SecureZeroMemory(s->data, s->len); HeapFree(GetProcessHeap(), 0, s->data); }
        prov_release(s->prov);
        HeapFree(GetProcessHeap(), 0, s);
        return STATUS_SUCCESS;
    }
    if (hdr[1] == 1145324613)
    {
        ssl_symkey *sk = (ssl_symkey *)obj;
        SecureZeroMemory(sk->key, sizeof(sk->key));
        SecureZeroMemory(sk->salt, sizeof(sk->salt));
        SecureZeroMemory(sk->mac_key, sizeof(sk->mac_key));
        prov_release(sk->prov);
        HeapFree(GetProcessHeap(), 0, sk);
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

/* Handshake hash management; minimal flow: create + update + compute session hash */
int WINAPI SslCreateHandshakeHash(ULONG_PTR hSslProvider, ULONG_PTR *phHandshakeHash,
                                  unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    NTSTATUS status;
    ssl_hash *h;
    const suite_meta *m = find_suite(dwCipherSuite);
    const WCHAR *hashAlg = (m && m->hash_len == 48) ? BCRYPT_SHA384_ALGORITHM : BCRYPT_SHA256_ALGORITHM;

    TRACE("SslCreateHandshakeHash(%p, %p, %u, %u, %#x)\n", p, phHandshakeHash, dwProtocol, dwCipherSuite, dwFlags);
    if (!is_valid_provider(p) || !phHandshakeHash) return STATUS_INVALID_PARAMETER;

    /* Default to SHA256; use SHA384 if suite requires */
    status = BCryptOpenAlgorithmProvider(&alg, hashAlg, NULL, 0);
    if (status) return status;
    status = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (status) return status;

    h = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*h));
    if (!h)
    {
        BCryptDestroyHash(hash);
        return STATUS_NO_MEMORY;
    }
    h->cb = sizeof(*h);
    h->magic = 1145324611;
    h->refcount = 1;
    h->prov = p; prov_addref(p);
    h->hHash = hash;
    *phHandshakeHash = (ULONG_PTR)h;
    return STATUS_SUCCESS;
}

int WINAPI SslHashHandshake(ULONG_PTR hSslProvider, ULONG_PTR hHandshakeHash,
                            unsigned __int8 *pbInput, unsigned int cbInput, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_hash *h = (ssl_hash *)hHandshakeHash;
    NTSTATUS status;
    TRACE("SslHashHandshake(%p, %p, %p, %u, %#x)\n", p, h, pbInput, cbInput, dwFlags);
    if (!is_valid_provider(p) || !is_valid_hash(h) || (!pbInput && cbInput)) return STATUS_INVALID_HANDLE;
    status = BCryptHashData(h->hHash, pbInput, cbInput, 0);
    return status;
}

int WINAPI SslComputeSessionHash(ULONG_PTR hSslProvider, ULONG_PTR hHandshakeHash, unsigned int dwProtocol,
                                 unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_hash *h = (ssl_hash *)hHandshakeHash;
    NTSTATUS status;
    DWORD size = 0;
    TRACE("SslComputeSessionHash(%p, %p, %u, %p, %u, %p, %#x)\n", p, h, dwProtocol, pbOutput, cbOutput, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !is_valid_hash(h)) return STATUS_INVALID_HANDLE;
    /* Determine hash length */
    status = BCryptGetProperty(h->hHash, BCRYPT_HASH_LENGTH, (PUCHAR)&size, sizeof(size), &size, 0);
    if (status) return status;
    if (!pbOutput || cbOutput < size)
    {
        if (pcbResult) *pcbResult = size;
        return STATUS_BUFFER_TOO_SMALL;
    }
    status = BCryptFinishHash(h->hHash, pbOutput, size, 0);
    if (!status)
    {
        if (pcbResult) *pcbResult = size;
    }
    return status;
}

int WINAPI SslGetCipherSuitePRFHashAlgorithm(ULONG_PTR hSslProvider, unsigned int dwProtocol, unsigned int dwCipherSuite,
                                             unsigned int dwKeyType, WCHAR *szPRFHash, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    TRACE("SslGetCipherSuitePRFHashAlgorithm(%p, %u, %u, %u, %p, %#x)\n", p, dwProtocol, dwCipherSuite, dwKeyType, szPRFHash, dwFlags);
    if (!is_valid_provider(p) || !szPRFHash) return STATUS_INVALID_PARAMETER;
    {
        const suite_meta *m = find_suite(dwCipherSuite);
        if (m && m->hash_len == 48) lstrcpyW(szPRFHash, L"SHA384");
        else lstrcpyW(szPRFHash, L"SHA256");
    }
    return STATUS_SUCCESS;
}

/* Remaining SSL entry points based on reference signatures */
int WINAPI SslChangeNotify(void *hEvent, unsigned int dwFlags)
{
    TRACE("SslChangeNotify(%p, %#x)\n", hEvent, dwFlags);
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslComputeClientAuthHash(ULONG_PTR hSslProvider, ULONG_PTR hMasterKey, ULONG_PTR hHandshakeHash,
                                    const WCHAR *pszAlgId, unsigned __int8 *pbOutput, unsigned int cbOutput,
                                    unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_secret *ms = (ssl_secret *)hMasterKey;
    ssl_hash *hh = (ssl_hash *)hHandshakeHash;
    BYTE trans_hash[32]; DWORD hash_len = 0; NTSTATUS status;
    TRACE("SslComputeClientAuthHash(%p, %p, %p, %s, %p, %u, %p, %#x)\n", p, ms, hh, debugstr_w(pszAlgId), pbOutput, cbOutput, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !ms || ms->magic != 1145324612 || !is_valid_hash(hh)) return STATUS_INVALID_PARAMETER;
    {
        BCRYPT_HASH_HANDLE dup = NULL; DWORD lenDummy = 0;
        status = BCryptGetProperty(hh->hHash, BCRYPT_HASH_LENGTH, (PUCHAR)&hash_len, sizeof(hash_len), &lenDummy, 0);
        if (status) return status;
        if (hash_len > sizeof(trans_hash)) return STATUS_NOT_SUPPORTED;
        status = BCryptDuplicateHash(hh->hHash, &dup, NULL, 0, 0);
        if (status) return status;
        status = BCryptFinishHash(dup, trans_hash, hash_len, 0);
        BCryptDestroyHash(dup);
        if (status) return status;
    }
    if (!pbOutput || cbOutput < hash_len)
    {
        if (pcbResult) *pcbResult = hash_len;
        return STATUS_BUFFER_TOO_SMALL;
    }
    {
        const WCHAR *alg = (hash_len > 32) ? BCRYPT_SHA384_ALGORITHM : BCRYPT_SHA256_ALGORITHM;
        DWORD prfLen = (lstrcmpW(alg, BCRYPT_SHA384_ALGORITHM) == 0) ? 48 : 32;
        status = tls12_prf(alg, prfLen, ms->data, ms->len, "client auth", trans_hash, hash_len, pbOutput, hash_len);
    }
    if (!status && pcbResult) *pcbResult = hash_len;
    return status;
}

int WINAPI SslComputeEapKeyBlock(ULONG_PTR hSslProvider, ULONG_PTR hMasterKey,
                                 unsigned __int8 *pbRandoms, unsigned int cbRandoms,
                                 unsigned __int8 *pbOutput, unsigned int cbOutput,
                                 unsigned int *pcbResult, unsigned int dwFlags)
{
    TRACE("SslComputeEapKeyBlock(...)\n");
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslComputeFinishedHash(ULONG_PTR hSslProvider, ULONG_PTR hMasterKey, ULONG_PTR hHandshakeHash,
                                  unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_secret *ms = (ssl_secret *)hMasterKey;
    ssl_hash *hh = (ssl_hash *)hHandshakeHash;
    BYTE trans_hash[32]; DWORD hash_len = 0; NTSTATUS status; const char *label;
    TRACE("SslComputeFinishedHash(%p, %p, %p, %p, %u, %#x)\n", p, ms, hh, pbOutput, cbOutput, dwFlags);
    if (!is_valid_provider(p) || !ms || ms->magic != 1145324612 || !is_valid_hash(hh)) return STATUS_INVALID_PARAMETER;
    {
        BCRYPT_HASH_HANDLE dup = NULL; DWORD lenDummy = 0;
        status = BCryptGetProperty(hh->hHash, BCRYPT_HASH_LENGTH, (PUCHAR)&hash_len, sizeof(hash_len), &lenDummy, 0);
        if (status) return status;
        if (hash_len > sizeof(trans_hash)) return STATUS_NOT_SUPPORTED;
        status = BCryptDuplicateHash(hh->hHash, &dup, NULL, 0, 0);
        if (status) return status;
        status = BCryptFinishHash(dup, trans_hash, hash_len, 0);
        BCryptDestroyHash(dup);
        if (status) return status;
    }
    label = (dwFlags & 1) ? "client finished" : "server finished";
    if (!pbOutput || cbOutput < 12) return STATUS_BUFFER_TOO_SMALL;
    {
        const WCHAR *alg = (hash_len > 32) ? BCRYPT_SHA384_ALGORITHM : BCRYPT_SHA256_ALGORITHM;
        DWORD prfLen = (lstrcmpW(alg, BCRYPT_SHA384_ALGORITHM) == 0) ? 48 : 32;
        status = tls12_prf(alg, prfLen, ms->data, ms->len, label, trans_hash, hash_len, pbOutput, 12);
    }
    return status;
}

int WINAPI SslEnumCipherSuites(ULONG_PTR hSslProvider, ULONG_PTR hPrivateKey,
                               void **ppCipherSuite, void **ppEnumState, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    SIZE_T idx = 0;
    TRACE("SslEnumCipherSuites(%p, %p, %p, %p, %#x)\n", p, (void *)hPrivateKey, ppCipherSuite, ppEnumState, dwFlags);
    if (!is_valid_provider(p) || !ppCipherSuite || !ppEnumState) return STATUS_INVALID_PARAMETER;
    if (*ppEnumState) idx = (SIZE_T)(ULONG_PTR)(*ppEnumState);
    if (idx >= sizeof(g_suites)/sizeof(g_suites[0]))
    {
        *ppCipherSuite = NULL;
        *ppEnumState = NULL;
        return STATUS_SUCCESS;
    }
    {
        const suite_meta *m = &g_suites[idx];
        NCRYPT_SSL_CIPHER_SUITE_LOCAL *cs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cs));
        if (!cs) return STATUS_NO_MEMORY;
        cs->dwProtocol = TLS1_2_VERSION;
        cs->dwCipherSuite = m->id;
        cs->dwBaseCipherSuite = m->id;
        lstrcpynW(cs->szCipherSuite, m->name, ARRAYSIZE(cs->szCipherSuite));
        lstrcpynW(cs->szCipher, m->cipher_name, ARRAYSIZE(cs->szCipher));
        cs->dwCipherLen = m->key_bits;
        cs->dwCipherBlockLen = m->block_len;
        lstrcpynW(cs->szHash, m->hash_name, ARRAYSIZE(cs->szHash));
        cs->dwHashLen = m->hash_len;
        lstrcpynW(cs->szExchange, m->kx, ARRAYSIZE(cs->szExchange));
        cs->dwMinExchangeLen = m->kx_min;
        cs->dwMaxExchangeLen = m->kx_max;
        lstrcpynW(cs->szCertificate, m->cert, ARRAYSIZE(cs->szCertificate));
        cs->dwKeyType = 0; /* RSA */
        *ppCipherSuite = cs;
        *ppEnumState = (void *)(idx + 1);
        return STATUS_SUCCESS;
    }
}

int WINAPI SslEnumEccCurves(ULONG_PTR hSslProvider, unsigned int *pEccCurveCount,
                            void **ppEccCurve, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    TRACE("SslEnumEccCurves(%p, %p, %p, %#x)\n", p, pEccCurveCount, ppEccCurve, dwFlags);
    if (!is_valid_provider(p) || !pEccCurveCount || !ppEccCurve) return STATUS_INVALID_PARAMETER;
    /* Advertise nistP256 and nistP384 */
    {
        NCRYPT_SSL_ECC_CURVE_INFO_LOCAL *list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*list) * 2);
        if (!list) return STATUS_NO_MEMORY;
        lstrcpynW(list[0].szCurve, BCRYPT_ECC_CURVE_NISTP256, ARRAYSIZE(list[0].szCurve));
        list[0].dwBits = 256; list[0].dwFlags = 0;
        lstrcpynW(list[1].szCurve, BCRYPT_ECC_CURVE_NISTP384, ARRAYSIZE(list[1].szCurve));
        list[1].dwBits = 384; list[1].dwFlags = 0;
        *pEccCurveCount = 2;
        *ppEccCurve = list;
        return STATUS_SUCCESS;
    }
}

int WINAPI SslEnumProtocolProviders(unsigned int *pdwProviderCount, void **ppProviderList, unsigned int dwFlags)
{
    TRACE("SslEnumProtocolProviders(%p, %p, %#x)\n", pdwProviderCount, ppProviderList, dwFlags);
    if (!pdwProviderCount || !ppProviderList) return STATUS_INVALID_PARAMETER;
    *pdwProviderCount = 0;
    *ppProviderList = NULL;
    return STATUS_SUCCESS;
}

int WINAPI SslExportKeyingMaterial(ULONG_PTR hSslProvider, ULONG_PTR hMasterKey, char *sLabel,
                                   unsigned __int8 *pbRandoms, unsigned int cbRandoms,
                                   unsigned __int8 *pbContextValue, int cbContextValue,
                                   unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int dwFlags)
{
    TRACE("SslExportKeyingMaterial(...)\n");
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslGetKeyProperty(ULONG_PTR hKey, const WCHAR *pszProperty,
                             unsigned __int8 **ppbOutput, unsigned int *pcbOutput, unsigned int dwFlags)
{
    TRACE("SslGetKeyProperty(%p, %s, %p, %p, %#x)\n", (void *)hKey, debugstr_w(pszProperty), ppbOutput, pcbOutput, dwFlags);
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslGetProviderProperty(ULONG_PTR hSslProvider, const WCHAR *pszProperty,
                                  unsigned __int8 **ppbOutput, unsigned int *pcbOutput,
                                  void **ppEnumState, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    TRACE("SslGetProviderProperty(%p, %s, %p, %p, %p, %#x)\n", p, debugstr_w(pszProperty), ppbOutput, pcbOutput, ppEnumState, dwFlags);
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslImportMasterKey(ULONG_PTR hSslProvider, ULONG_PTR hPrivateKey, ULONG_PTR *phMasterKey,
                              unsigned int dwProtocol, unsigned int dwCipherSuite, void *pParameterList,
                              unsigned __int8 *pbEncryptedKey, unsigned int cbEncryptedKey, unsigned int dwFlags)
{
    TRACE("SslImportMasterKey(...)\n");
    return STATUS_NOT_SUPPORTED;
}

int WINAPI SslOpenPrivateKey(ULONG_PTR hSslProvider, ULONG_PTR *phPrivateKey, const void *pCertContext, unsigned int dwFlags)
{
    TRACE("SslOpenPrivateKey(...)\n");
    return STATUS_NOT_SUPPORTED;
}

/* Create client auth hash: honor requested alg if provided */
int WINAPI SslCreateClientAuthHash(ULONG_PTR hSslProvider, ULONG_PTR *phHandshakeHash, unsigned int dwProtocol,
                                   unsigned int dwCipherSuite, const WCHAR *pszHashAlgId, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    NTSTATUS status;
    ssl_hash *h;
    const WCHAR *algid = pszHashAlgId ? pszHashAlgId : BCRYPT_SHA256_ALGORITHM;
    TRACE("SslCreateClientAuthHash(%p, %p, %u, %u, %s, %#x)\n", p, phHandshakeHash, dwProtocol, dwCipherSuite, debugstr_w(algid), dwFlags);
    if (!is_valid_provider(p) || !phHandshakeHash) return STATUS_INVALID_PARAMETER;
    status = BCryptOpenAlgorithmProvider(&alg, algid, NULL, 0);
    if (status) return status;
    status = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (status) return status;
    h = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*h));
    if (!h)
    {
        BCryptDestroyHash(hash);
        return STATUS_NO_MEMORY;
    }
    h->cb = sizeof(*h);
    h->magic = 1145324611;
    h->refcount = 1;
    h->prov = p; prov_addref(p);
    h->hHash = hash;
    *phHandshakeHash = (ULONG_PTR)h;
    return STATUS_SUCCESS;
}

int WINAPI SslGeneratePreMasterKey(ULONG_PTR hSslProvider, ULONG_PTR hPublicKey, ULONG_PTR *phPreMasterKey,
                                   unsigned int dwProtocol, unsigned int dwCipherSuite, void *pParameterList,
                                   unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_key *k = (ssl_key *)hPublicKey;
    ssl_secret *s; BYTE premaster[48]; DWORD needed = 0; SECURITY_STATUS sec;
    TRACE("SslGeneratePreMasterKey(%p, %p, %p, %u, %u, %p, %p, %u, %p, %#x)\n", p, k, phPreMasterKey, dwProtocol, dwCipherSuite, pParameterList, pbOutput, cbOutput, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !is_valid_key(k) || !phPreMasterKey) return STATUS_INVALID_PARAMETER;
    premaster[0]=0x03; premaster[1]=0x03; BCryptGenRandom(NULL, premaster+2, sizeof(premaster)-2, 0);
    sec = NCryptEncrypt(k->hKey, premaster, sizeof(premaster), NULL, NULL, 0, &needed, NCRYPT_PAD_PKCS1_FLAG);
    if (sec) return sec;
    if (!pbOutput || cbOutput < needed) { if (pcbResult) *pcbResult = needed; }
    else { sec = NCryptEncrypt(k->hKey, premaster, sizeof(premaster), NULL, pbOutput, cbOutput, &needed, NCRYPT_PAD_PKCS1_FLAG); if (sec) return sec; if (pcbResult) *pcbResult = needed; }
    s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*s)); if (!s) return STATUS_NO_MEMORY;
    s->cb=sizeof(*s); s->magic=1145324612; s->refcount=1; s->prov=p; prov_addref(p);
    s->data = HeapAlloc(GetProcessHeap(),0,sizeof(premaster)); if (!s->data) { HeapFree(GetProcessHeap(),0,s); return STATUS_NO_MEMORY; }
    memcpy(s->data, premaster, sizeof(premaster)); s->len = sizeof(premaster);
    *phPreMasterKey = (ULONG_PTR)s; SecureZeroMemory(premaster, sizeof(premaster)); return STATUS_SUCCESS;
}

int WINAPI SslGenerateMasterKey(ULONG_PTR hSslProvider, ULONG_PTR hPrivateKey, ULONG_PTR hPublicKey,
                                ULONG_PTR *phMasterKey, unsigned int dwProtocol, unsigned int dwCipherSuite,
                                void *pParameterList, unsigned __int8 *pbOutput, unsigned int cbOutput,
                                unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    BYTE *cr=NULL,*sr=NULL; ULONG crl=0,srl=0; NTSTATUS status; BYTE ms[48];
    TRACE("SslGenerateMasterKey(%p, %p, %p, %p, %u, %u, %p, %p, %u, %p, %#x)\n", p, (void *)hPrivateKey, (void *)hPublicKey, phMasterKey, dwProtocol, dwCipherSuite, pParameterList, pbOutput, cbOutput, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !phMasterKey) return STATUS_INVALID_PARAMETER;
    if (!get_param_buffer((BCryptBufferDesc *)pParameterList, NCRYPTBUFFER_SSL_CLIENT_RANDOM, &cr, &crl)) return STATUS_INVALID_PARAMETER;
    if (!get_param_buffer((BCryptBufferDesc *)pParameterList, NCRYPTBUFFER_SSL_SERVER_RANDOM, &sr, &srl)) return STATUS_INVALID_PARAMETER;
    /* Determine premaster: either provided as ssl_secret (RSA path), or computed via ECDH if keys passed */
    if (hPublicKey && hPrivateKey)
    {
        ssl_key *priv = (ssl_key *)hPrivateKey;
        ssl_key *pub = (ssl_key *)hPublicKey;
        BCRYPT_SECRET_HANDLE secret = NULL; NTSTATUS st; DWORD cb=0; BYTE *pmk=NULL;
        if (!is_valid_key(priv) || !is_valid_key(pub) || !priv->is_bcrypt) return STATUS_INVALID_PARAMETER;
        st = BCryptSecretAgreement((BCRYPT_KEY_HANDLE)priv->hKey, (BCRYPT_KEY_HANDLE)pub->hKey, &secret, 0);
        if (st) return st;
        /* Export raw shared secret via KDF null */
        st = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &cb, 0);
        if (st) { BCryptDestroySecret(secret); return st; }
        pmk = HeapAlloc(GetProcessHeap(), 0, cb);
        if (!pmk) { BCryptDestroySecret(secret); return STATUS_NO_MEMORY; }
        st = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, pmk, cb, &cb, 0);
        BCryptDestroySecret(secret);
        if (st) { HeapFree(GetProcessHeap(),0,pmk); return st; }
        {
            BYTE seed[64]; DWORD sl=0;
            if (crl+srl>sizeof(seed)) { HeapFree(GetProcessHeap(),0,pmk); return STATUS_INVALID_PARAMETER; }
            memcpy(seed+sl, cr, crl); sl+=crl; memcpy(seed+sl, sr, srl); sl+=srl;
            /* For ECDHE suites, choose PRF by suite hash (SHA256 or SHA384) */
            {
                const suite_meta *sm = find_suite(dwCipherSuite);
                const WCHAR *alg = (sm && sm->hash_len == 48) ? BCRYPT_SHA384_ALGORITHM : BCRYPT_SHA256_ALGORITHM;
                DWORD prfLen = (sm && sm->hash_len == 48) ? 48 : 32;
                status = tls12_prf(alg, prfLen, pmk, cb, "master secret", seed, sl, ms, sizeof(ms));
            }
            SecureZeroMemory(pmk, cb); HeapFree(GetProcessHeap(),0,pmk);
            if (status) return status;
        }
    }
    else
    {
        ssl_secret *prem = (ssl_secret *)hPrivateKey;
        if (!prem || prem->magic != 1145324612) return STATUS_INVALID_PARAMETER;
    { BYTE seed[64]; DWORD sl=0; const suite_meta *sm = find_suite(dwCipherSuite); const WCHAR *alg = (sm && sm->hash_len == 48) ? BCRYPT_SHA384_ALGORITHM : BCRYPT_SHA256_ALGORITHM; DWORD prfLen = (sm && sm->hash_len == 48) ? 48 : 32; if (crl+srl>sizeof(seed)) return STATUS_INVALID_PARAMETER; memcpy(seed+sl, cr, crl); sl+=crl; memcpy(seed+sl, sr, srl); sl+=srl; status = tls12_prf(alg, prfLen, prem->data, prem->len, "master secret", seed, sl, ms, sizeof(ms)); if (status) return status; }
    }
    { ssl_secret *out = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*out)); if (!out) return STATUS_NO_MEMORY; out->cb=sizeof(*out); out->magic=1145324612; out->refcount=1; out->prov=p; prov_addref(p); out->data = HeapAlloc(GetProcessHeap(),0,sizeof(ms)); if (!out->data) { HeapFree(GetProcessHeap(),0,out); return STATUS_NO_MEMORY; } memcpy(out->data, ms, sizeof(ms)); out->len=sizeof(ms); *phMasterKey=(ULONG_PTR)out; SecureZeroMemory(ms,sizeof(ms)); }
    if (pcbResult) *pcbResult = 0; return STATUS_SUCCESS;
}

int WINAPI SslGenerateSessionKeys(ULONG_PTR hSslProvider, ULONG_PTR hMasterKey,
                                  ULONG_PTR *phReadKey, ULONG_PTR *phWriteKey,
                                  void *pParameterList, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider; ssl_secret *ms = (ssl_secret *)hMasterKey; BYTE *cr=NULL,*sr=NULL; ULONG crl=0,srl=0; PUCHAR suitep=NULL; ULONG suitel=0; DWORD suite=0; const suite_meta *m; NTSTATUS status; BYTE kb[128];
    TRACE("SslGenerateSessionKeys(%p, %p, %p, %p, %p, %#x)\n", p, ms, phReadKey, phWriteKey, pParameterList, dwFlags);
    if (!is_valid_provider(p) || !ms || ms->magic != 1145324612 || !phReadKey || !phWriteKey) return STATUS_INVALID_PARAMETER;
    if (!get_param_buffer((BCryptBufferDesc *)pParameterList, NCRYPTBUFFER_SSL_CLIENT_RANDOM, &cr, &crl)) return STATUS_INVALID_PARAMETER;
    if (!get_param_buffer((BCryptBufferDesc *)pParameterList, NCRYPTBUFFER_SSL_SERVER_RANDOM, &sr, &srl)) return STATUS_INVALID_PARAMETER;
    if (!get_param_buffer((BCryptBufferDesc *)pParameterList, NCRYPTBUFFER_SSL_CIPHER_SUITE, &suitep, &suitel)) return STATUS_INVALID_PARAMETER;
    /* Accept multiple encodings */
    suite = parse_cipher_suite(suitep, suitel);
    if (!suite) return STATUS_INVALID_PARAMETER;
    m = find_suite(suite);
    if (!m) return STATUS_NOT_SUPPORTED;
    if (m->is_aead)
    {
        /* GCM: client_write_key(16), server_write_key(16), client_salt(4), server_salt(4) */
    { BYTE seed[64]; DWORD sl=0; if (srl+crl > sizeof(seed)) return STATUS_INVALID_PARAMETER; memcpy(seed+sl, sr, srl); sl+=srl; memcpy(seed+sl, cr, crl); sl+=crl; status = tls12_prf(BCRYPT_SHA256_ALGORITHM, 32, ms->data, ms->len, "key expansion", seed, sl, kb, 40); if (status) return status; }
        {
            ssl_symkey *rk = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*rk));
            ssl_symkey *wk = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*wk));
            if (!rk||!wk){ if(rk)HeapFree(GetProcessHeap(),0,rk); if(wk)HeapFree(GetProcessHeap(),0,wk); return STATUS_NO_MEMORY; }
            rk->cb=wk->cb=sizeof(*rk); rk->magic=wk->magic=1145324613; rk->refcount=wk->refcount=1; rk->prov=wk->prov=p; prov_addref(p); prov_addref(p);
            rk->suite=wk->suite=suite; rk->key_len=wk->key_len=16; rk->salt_len=wk->salt_len=4; rk->block_len=wk->block_len=16; rk->iv_len=wk->iv_len=0; rk->mac_len=wk->mac_len=0;
            if (dwFlags & SSLGENKEYS_FLAG_CLIENT)
            {
                /* client: write=client, read=server */
                memcpy(wk->key,kb+0,16); memcpy(wk->salt,kb+32,4);
                memcpy(rk->key,kb+16,16); memcpy(rk->salt,kb+36,4);
            }
            else
            {
                /* server: write=server, read=client */
                memcpy(wk->key,kb+16,16); memcpy(wk->salt,kb+36,4);
                memcpy(rk->key,kb+0,16);  memcpy(rk->salt,kb+32,4);
            }
            *phWriteKey=(ULONG_PTR)wk; *phReadKey=(ULONG_PTR)rk; SecureZeroMemory(kb,sizeof(kb));
        }
    }
    else
    {
        /* CBC with HMAC-SHA1: derive mac_write_secret(20) each, keys (key_len bytes) each, IVs (block_len bytes) each. TLS1.1+ uses explicit IV per-record, but we keep a block_len for sanity. */
        DWORD key_bytes = (m->key_bits/8);
    DWORD mac_bytes = (m->hash_len == 32 ? 32 : 20); /* SHA256 or SHA1 */
        DWORD block = 16;
        DWORD need = 2*mac_bytes + 2*key_bytes + 2*block;
    BYTE seed[64]; DWORD sl=0;
    if (srl+crl > sizeof(seed)) return STATUS_INVALID_PARAMETER;
        if (need > sizeof(kb)) return STATUS_INVALID_PARAMETER;
    memcpy(seed+sl, sr, srl); sl+=srl; memcpy(seed+sl, cr, crl); sl+=crl;
    /* CBC uses SHA1 for MAC, but TLS 1.2 key expansion uses the PRF hash; keep SHA256 for our set */
    status = tls12_prf(BCRYPT_SHA256_ALGORITHM, 32, ms->data, ms->len, "key expansion", seed, sl, kb, need);
        if (status) return status;
        {
            ssl_symkey *rk = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*rk));
            ssl_symkey *wk = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*wk));
            DWORD o = 0;
            BYTE *client_mac = kb+o; o+=mac_bytes;
            BYTE *server_mac = kb+o; o+=mac_bytes;
            BYTE *client_key = kb+o; o+=key_bytes;
            BYTE *server_key = kb+o; o+=key_bytes;
            BYTE *client_iv  = kb+o; o+=block;
            BYTE *server_iv  = kb+o; o+=block;
            if (!rk||!wk){ if(rk)HeapFree(GetProcessHeap(),0,rk); if(wk)HeapFree(GetProcessHeap(),0,wk); return STATUS_NO_MEMORY; }
            rk->cb=wk->cb=sizeof(*rk); rk->magic=wk->magic=1145324613; rk->refcount=wk->refcount=1; rk->prov=wk->prov=p; prov_addref(p); prov_addref(p);
            rk->suite=wk->suite=suite; rk->block_len=wk->block_len=block; rk->iv_len=wk->iv_len=block; rk->salt_len=wk->salt_len=0;
            rk->mac_len=wk->mac_len=mac_bytes; rk->key_len=wk->key_len=key_bytes;
            if (dwFlags & SSLGENKEYS_FLAG_CLIENT)
            {
                /* client: write=client, read=server */
                memcpy(wk->mac_key, client_mac, mac_bytes); memcpy(wk->key, client_key, key_bytes); memcpy(wk->salt, client_iv, block);
                memcpy(rk->mac_key, server_mac, mac_bytes); memcpy(rk->key, server_key, key_bytes); memcpy(rk->salt, server_iv, block);
            }
            else
            {
                /* server: write=server, read=client */
                memcpy(wk->mac_key, server_mac, mac_bytes); memcpy(wk->key, server_key, key_bytes); memcpy(wk->salt, server_iv, block);
                memcpy(rk->mac_key, client_mac, mac_bytes); memcpy(rk->key, client_key, key_bytes); memcpy(rk->salt, client_iv, block);
            }
            *phWriteKey=(ULONG_PTR)wk; *phReadKey=(ULONG_PTR)rk; SecureZeroMemory(kb,sizeof(kb));
        }
    }
    return STATUS_SUCCESS;
}

int WINAPI SslSignHash(ULONG_PTR hSslProvider, ULONG_PTR hPrivateKey,
                       unsigned __int8 *pbHashValue, unsigned int cbHashValue,
                       unsigned __int8 *pbSignature, unsigned int cbSignature,
                       unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_key *k = (ssl_key *)hPrivateKey;
    SECURITY_STATUS sec;
    DWORD needed = 0;
    TRACE("SslSignHash(%p, %p, %p, %u, %p, %u, %p, %#x)\n", p, k, pbHashValue, cbHashValue, pbSignature, cbSignature, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !is_valid_key(k) || !pbHashValue) return STATUS_INVALID_PARAMETER;
    /* size query */
    sec = NCryptSignHash(k->hKey, NULL, pbHashValue, cbHashValue, NULL, 0, &needed, dwFlags);
    if (sec) return sec;
    if (!pbSignature || cbSignature < needed)
    {
        if (pcbResult) *pcbResult = needed;
        return STATUS_BUFFER_TOO_SMALL;
    }
    sec = NCryptSignHash(k->hKey, NULL, pbHashValue, cbHashValue, pbSignature, cbSignature, &needed, dwFlags);
    if (!sec && pcbResult) *pcbResult = needed;
    return sec;
}

int WINAPI SslVerifySignature(ULONG_PTR hSslProvider, ULONG_PTR hPublicKey,
                              unsigned __int8 *pbHashValue, unsigned int cbHashValue,
                              unsigned __int8 *pbSignature, unsigned int cbSignature,
                              unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_key *k = (ssl_key *)hPublicKey;
    SECURITY_STATUS sec;
    TRACE("SslVerifySignature(%p, %p, %p, %u, %p, %u, %#x)\n", p, k, pbHashValue, cbHashValue, pbSignature, cbSignature, dwFlags);
    if (!is_valid_provider(p) || !is_valid_key(k) || !pbHashValue || !pbSignature) return STATUS_INVALID_PARAMETER;
    sec = NCryptVerifySignature(k->hKey, NULL, pbHashValue, cbHashValue, pbSignature, cbSignature, dwFlags);
    return sec;
}

/* Key import/export wrappers: we proxy to NCryptImportKey/NCryptExportKey using our NCrypt provider
   already in use elsewhere (NCryptOpenStorageProvider). For simplicity, we require caller to pass
   blob types that NCrypt supports (e.g., BCRYPT_RSAFULLPRIVATE_BLOB via NCrypt).
*/
int WINAPI SslImportKey(ULONG_PTR hSslProvider, ULONG_PTR *phKey, const WCHAR *pszBlobType,
                        unsigned __int8 *pbKeyBlob, unsigned int cbKeyBlob, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    NCRYPT_PROV_HANDLE prov = 0;
    NCRYPT_KEY_HANDLE h = 0;
    SECURITY_STATUS sec;
    BOOL is_bcrypt = FALSE;
    ssl_key *k;

    TRACE("SslImportKey(%p, %p, %s, %p, %u, %#x)\n", p, phKey, debugstr_w(pszBlobType), pbKeyBlob, cbKeyBlob, dwFlags);
    if (!is_valid_provider(p) || !phKey || !pszBlobType || !pbKeyBlob) return STATUS_INVALID_PARAMETER;
    if (lstrcmpW(pszBlobType, BCRYPT_ECCPUBLIC_BLOB) == 0 || lstrcmpW(pszBlobType, BCRYPT_ECCPRIVATE_BLOB) == 0)
    {
        BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_KEY_HANDLE bk = NULL; NTSTATUS st;
        st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_ALGORITHM, NULL, 0);
        if (st)
        {
            /* Fallback to curve-specific provider if generic ECDH is unavailable */
            st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
        }
        if (st) return st;
    /* Set curve name if supported; ignore failure on older providers */
    (void)BCryptSetProperty(alg, BCRYPT_ECC_CURVE_NAME, (PUCHAR)BCRYPT_ECC_CURVE_NISTP256, (ULONG)((wcslen(BCRYPT_ECC_CURVE_NISTP256)+1)*sizeof(WCHAR)), 0);
    st = BCryptImportKeyPair(alg, NULL, pszBlobType, &bk, pbKeyBlob, cbKeyBlob, 0);
        BCryptCloseAlgorithmProvider(alg, 0);
        if (st) return st;
        h = (NCRYPT_KEY_HANDLE)bk;
        is_bcrypt = TRUE;
    }
    else
    {
        sec = NCryptOpenStorageProvider(&prov, NULL, 0);
        if (sec) return sec;
        sec = NCryptImportKey(prov, 0, pszBlobType, NULL, &h, pbKeyBlob, cbKeyBlob, 0);
        if (sec)
        {
            NCryptFreeObject(prov);
            return sec;
        }
    }
    k = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*k));
    if (!k)
    {
        NCryptFreeObject(h);
        NCryptFreeObject(prov);
        return STATUS_NO_MEMORY;
    }
    k->cb = sizeof(*k);
    k->magic = 1145324610;
    k->refcount = 1;
    k->prov = p; prov_addref(p);
    k->hKey = h;
    k->is_bcrypt = is_bcrypt;
    *phKey = (ULONG_PTR)k;
    /* provider handle now owned by key via NCrypt inside h; free our provider */
    if (prov) NCryptFreeObject(prov);
    return STATUS_SUCCESS;
}

int WINAPI SslExportKey(ULONG_PTR hSslProvider, ULONG_PTR hKey, const WCHAR *pszBlobType,
                        unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int *pcbResult, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    ssl_key *k = (ssl_key *)hKey;
    SECURITY_STATUS sec;
    TRACE("SslExportKey(%p, %p, %s, %p, %u, %p, %#x)\n", p, k, debugstr_w(pszBlobType), pbOutput, cbOutput, pcbResult, dwFlags);
    if (!is_valid_provider(p) || !is_valid_key(k) || !pszBlobType) return STATUS_INVALID_HANDLE;
    if (k->is_bcrypt)
    {
        NTSTATUS st; DWORD got = 0; BCRYPT_KEY_HANDLE bk = (BCRYPT_KEY_HANDLE)k->hKey;
        st = BCryptExportKey(bk, NULL, pszBlobType, pbOutput, cbOutput, &got, 0);
        if (st) return st; if (pcbResult) *pcbResult = got; return STATUS_SUCCESS;
    }
    sec = NCryptExportKey(k->hKey, 0, pszBlobType, NULL, pbOutput, cbOutput, pcbResult, 0);
    return sec;
}

/* The following are not wired yet; return not supported to match Windows when provider lacks feature. */
int WINAPI SslLookupCipherLengths(ULONG_PTR hSslProvider, unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwKeyType, void *pCipherLengths, unsigned int cbCipherLengths, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    const suite_meta *m = find_suite(dwCipherSuite);
    NCRYPT_SSL_CIPHER_LENGTHS_LOCAL *L = (NCRYPT_SSL_CIPHER_LENGTHS_LOCAL *)pCipherLengths;
    TRACE("SslLookupCipherLengths(%p, %u, %u, %u, %p, %u, %#x)\n", p, dwProtocol, dwCipherSuite, dwKeyType, pCipherLengths, cbCipherLengths, dwFlags);
    if (!is_valid_provider(p) || !L || cbCipherLengths < sizeof(*L) || !m) return STATUS_INVALID_PARAMETER;
    L->cbLength = sizeof(*L);
    if (m->is_aead)
    {
        /* TLS1.2 GCM: 5 bytes TLS header + 8 byte explicit nonce; 16 byte tag */
        L->dwHeaderLen = 5 + 8;
        L->dwFixedTrailerLen = 16;
        L->dwMaxVariableTrailerLen = 0;
        L->dwFlags = 0;
    }
    else
    {
    /* TLS1.1+ CBC: 5 bytes header, 16 byte explicit IV; trailer = MAC(20 or 32) + padding(1..16) */
    L->dwHeaderLen = 5 + 16;
    L->dwFixedTrailerLen = ((m && m->hash_len == 32) ? 32 : 20) + 1; /* at least one padding byte */
        L->dwMaxVariableTrailerLen = 15; /* up to 16 total including the 1 above */
        L->dwFlags = 0;
    }
    return STATUS_SUCCESS;
}

int WINAPI SslLookupCipherSuiteInfo(ULONG_PTR hSslProvider, unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwKeyType, void *pCipherSuite, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    const suite_meta *m = find_suite(dwCipherSuite);
    NCRYPT_SSL_CIPHER_SUITE_LOCAL *cs = (NCRYPT_SSL_CIPHER_SUITE_LOCAL *)pCipherSuite;
    TRACE("SslLookupCipherSuiteInfo(%p, %u, %u, %u, %p, %#x)\n", p, dwProtocol, dwCipherSuite, dwKeyType, pCipherSuite, dwFlags);
    if (!is_valid_provider(p) || !cs || !m) return STATUS_INVALID_PARAMETER;
    ZeroMemory(cs, sizeof(*cs));
    cs->dwProtocol = TLS1_2_VERSION;
    cs->dwCipherSuite = m->id;
    cs->dwBaseCipherSuite = m->id;
    lstrcpynW(cs->szCipherSuite, m->name, ARRAYSIZE(cs->szCipherSuite));
    lstrcpynW(cs->szCipher, m->cipher_name, ARRAYSIZE(cs->szCipher));
    cs->dwCipherLen = m->key_bits;
    cs->dwCipherBlockLen = m->block_len;
    lstrcpynW(cs->szHash, m->hash_name, ARRAYSIZE(cs->szHash));
    cs->dwHashLen = m->hash_len;
    lstrcpynW(cs->szExchange, m->kx, ARRAYSIZE(cs->szExchange));
    cs->dwMinExchangeLen = m->kx_min;
    cs->dwMaxExchangeLen = m->kx_max;
    lstrcpynW(cs->szCertificate, m->cert, ARRAYSIZE(cs->szCertificate));
    cs->dwKeyType = 0;
    return STATUS_SUCCESS;
}

int WINAPI SslCreateEphemeralKey(ULONG_PTR hSslProvider, ULONG_PTR *phEphemeralKey, unsigned int dwProtocol, unsigned int dwCipherSuite, unsigned int dwKeyType, unsigned int dwKeyBitLen, unsigned __int8 *pbParams, unsigned int cbParams, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    NTSTATUS status;
    ssl_key *sk;
    TRACE("SslCreateEphemeralKey(%p, %p, proto=%u, suite=%u, type=%u, bits=%u, params=%p/%u, flags=%#x)\n", p, phEphemeralKey, dwProtocol, dwCipherSuite, dwKeyType, dwKeyBitLen, pbParams, cbParams, dwFlags);
    if (!is_valid_provider(p) || !phEphemeralKey) return STATUS_INVALID_PARAMETER;
    if (dwKeyBitLen == 0) dwKeyBitLen = 256; /* default P-256 */
    if (dwKeyBitLen != 256) return STATUS_NOT_SUPPORTED; /* only P-256 supported for now */
    /* For now, only ECDHE on P-256 */
    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_ALGORITHM, NULL, 0);
    if (status == STATUS_SUCCESS)
    {
        /* Some providers require setting the curve via property prior to key gen */
        (void)BCryptSetProperty(alg, BCRYPT_ECC_CURVE_NAME, (PUCHAR)BCRYPT_ECC_CURVE_NISTP256, (ULONG)((wcslen(BCRYPT_ECC_CURVE_NISTP256)+1)*sizeof(WCHAR)), 0);
        status = BCryptGenerateKeyPair(alg, &key, dwKeyBitLen, 0);
        if (status == STATUS_SUCCESS)
            status = BCryptFinalizeKeyPair(key, 0);
        BCryptCloseAlgorithmProvider(alg, 0);
        if (status == STATUS_INVALID_PARAMETER || status == STATUS_NOT_SUPPORTED)
        {
            /* Retry with curve-specific provider */
            if (key) { BCryptDestroyKey(key); key = NULL; }
            status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
            if (!status)
            {
                status = BCryptGenerateKeyPair(alg, &key, dwKeyBitLen, 0);
                if (!status) status = BCryptFinalizeKeyPair(key, 0);
                BCryptCloseAlgorithmProvider(alg, 0);
            }
        }
    }
    else
    {
        /* Fallback to curve-specific name immediately */
        status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
        if (!status)
        {
            status = BCryptGenerateKeyPair(alg, &key, dwKeyBitLen, 0);
            if (!status) status = BCryptFinalizeKeyPair(key, 0);
            BCryptCloseAlgorithmProvider(alg, 0);
        }
    }
    if (status) { if (key) BCryptDestroyKey(key); return status; }
    sk = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sk));
    if (!sk) { BCryptDestroyKey(key); return STATUS_NO_MEMORY; }
    sk->cb = sizeof(*sk);
    sk->magic = 1145324610;
    sk->refcount = 1;
    sk->prov = p; prov_addref(p);
    sk->hKey = (NCRYPT_KEY_HANDLE)key;
    sk->is_bcrypt = TRUE;
    *phEphemeralKey = (ULONG_PTR)sk;
    return STATUS_SUCCESS;
}

int WINAPI SslEncryptPacket(ULONG_PTR hSslProvider, ULONG_PTR hKey, unsigned __int8 *pbInput, unsigned int cbInput, unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int *pcbResult, unsigned __int64 SequenceNumber, unsigned int dwContentType, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider; ssl_symkey *wk = (ssl_symkey *)hKey; const suite_meta *m; DWORD tag_len=16, explicit_len=8, total,len_field; BYTE header[5], nonce[12], aad[13], explicit_nonce[8]; BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_KEY_HANDLE key=NULL; DWORD obj_len=0, cbres=0, ret=0; BYTE *keyobj=NULL; NTSTATUS status;
    TRACE("SslEncryptPacket(%p, %p, %p, %u, %p, %u, %p, %I64u, %u, %#x)\n", p, wk, pbInput, cbInput, pbOutput, cbOutput, pcbResult, SequenceNumber, dwContentType, dwFlags);
    if (!is_valid_provider(p) || !wk || wk->magic != 1145324613 || !pbInput) return STATUS_INVALID_PARAMETER; m = find_suite(wk->suite); if (!m) return STATUS_NOT_SUPPORTED;
    if (!m->is_aead)
    {
        /* CBC + HMAC-SHA1; TLS 1.1+: explicit IV */
    DWORD block = 16; DWORD expl = block; DWORD mac_len = (m && m->hash_len == 32) ? 32 : 20; DWORD pad_len, plain_len, ct_total, inner_len; BYTE seqhdr[13]; BYTE iv[16]; DWORD i; PUCHAR out;
        if (wk->block_len) block = wk->block_len;
        /* header: TLS record header uses payload length = expl + inner_len */
        /* Compute MAC over: seq(8) + type(1) + ver(2) + length(2 of inner content) + content */
        for (i=0;i<8;i++) seqhdr[i] = (BYTE)((SequenceNumber>>(56-8*i)) & 0xFF);
        seqhdr[8] = (BYTE)dwContentType; seqhdr[9]=0x03; seqhdr[10]=0x03; seqhdr[11]=(BYTE)(cbInput>>8); seqhdr[12]=(BYTE)(cbInput&0xFF);
        {
            BYTE *maccat = HeapAlloc(GetProcessHeap(),0, sizeof(seqhdr)+cbInput);
            BYTE mac[32];
            if (!maccat) return STATUS_NO_MEMORY;
            memcpy(maccat, seqhdr, sizeof(seqhdr));
            memcpy(maccat+sizeof(seqhdr), pbInput, cbInput);
            if (m && m->hash_len == 32)
                status = hmac_sha256_mac(wk->mac_key, wk->mac_len ? wk->mac_len : 32, maccat, (DWORD)(sizeof(seqhdr)+cbInput), mac, 32);
            else
                status = hmac_sha1(wk->mac_key, wk->mac_len ? wk->mac_len : 20, maccat, (DWORD)(sizeof(seqhdr)+cbInput), mac, 20);
            HeapFree(GetProcessHeap(),0,maccat);
            if (status) return status;
            /* Compute padding: pad to multiple of block size. Content = data || mac || padding_bytes (value = pad_len) */
            inner_len = cbInput + mac_len;
            pad_len = block - ((inner_len + 1) % block);
            if (pad_len == block) pad_len = 0;
            plain_len = inner_len + 1 + pad_len;
            ct_total = 5 + expl + plain_len;
            if (!pbOutput || cbOutput < ct_total) { if (pcbResult) *pcbResult = ct_total; return STATUS_BUFFER_TOO_SMALL; }
            header[0]=(BYTE)dwContentType; header[1]=0x03; header[2]=0x03; header[3]=(BYTE)((expl+plain_len)>>8); header[4]=(BYTE)((expl+plain_len)&0xFF);
            memcpy(pbOutput, header, 5);
            /* Explicit IV: random */
            BCryptGenRandom(NULL, iv, block, 0);
            memcpy(pbOutput+5, iv, block);
            /* Build plaintext = data || mac || padding */
            out = pbOutput + 5 + expl;
            memcpy(out, pbInput, cbInput);
            memcpy(out+cbInput, mac, mac_len);
            {
                BYTE padv = (BYTE)pad_len;
                for (i=0;i<pad_len+1;i++) out[cbInput+mac_len+i] = padv;
            }
            /* Encrypt with AES-CBC */
            status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0); if (status) return status;
            status = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, (ULONG)((wcslen(BCRYPT_CHAIN_MODE_CBC)+1)*sizeof(WCHAR)), 0); if (status) goto enc_out_cbc;
            status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&obj_len, sizeof(obj_len), &cbres, 0); if (status) goto enc_out_cbc;
            keyobj = HeapAlloc(GetProcessHeap(),0,obj_len); if (!keyobj) { status = STATUS_NO_MEMORY; goto enc_out_cbc; }
            status = BCryptGenerateSymmetricKey(alg, &key, keyobj, obj_len, wk->key, wk->key_len, 0); if (status) goto enc_out_cbc;
            status = BCryptEncrypt(key, out, plain_len, NULL, iv, block, out, plain_len, &ret, 0);
enc_out_cbc:
            if (key) BCryptDestroyKey(key); if (keyobj) HeapFree(GetProcessHeap(),0,keyobj); if (alg) BCryptCloseAlgorithmProvider(alg,0);
            if (status) return status;
            if (pcbResult) *pcbResult = ct_total;
            return STATUS_SUCCESS;
        }
    }
    /* AEAD path (GCM) */
    if (wk->salt_len < 4) return STATUS_INVALID_PARAMETER;
    len_field = explicit_len + cbInput + tag_len; total = 5 + len_field; if (!pbOutput || cbOutput < total) { if (pcbResult) *pcbResult = total; return STATUS_BUFFER_TOO_SMALL; }
    header[0]=(BYTE)dwContentType; header[1]=0x03; header[2]=0x03; header[3]=(BYTE)(len_field>>8); header[4]=(BYTE)(len_field&0xFF); memcpy(pbOutput, header, 5);
    {
        int i;
        for(i=0;i<8;i++) explicit_nonce[7-i]=(BYTE)((SequenceNumber>>(i*8))&0xFF);
    }
    memcpy(pbOutput+5, explicit_nonce, 8);
    memcpy(nonce, wk->salt, 4);
    memcpy(nonce+4, explicit_nonce, 8);
    {
        int i;
        for(i=0;i<8;i++) aad[i]=(BYTE)((SequenceNumber>>(56-8*i))&0xFF);
    }
    aad[8]=header[0]; aad[9]=header[1]; aad[10]=header[2]; aad[11]=(BYTE)(cbInput>>8); aad[12]=(BYTE)(cbInput&0xFF);
    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0); if (status) return status;
    status = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, (ULONG)((wcslen(BCRYPT_CHAIN_MODE_GCM)+1)*sizeof(WCHAR)), 0); if (status) goto enc_out;
    status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&obj_len, sizeof(obj_len), &cbres, 0); if (status) goto enc_out;
    keyobj = HeapAlloc(GetProcessHeap(),0,obj_len); if (!keyobj) { status = STATUS_NO_MEMORY; goto enc_out; }
    status = BCryptGenerateSymmetricKey(alg, &key, keyobj, obj_len, wk->key, wk->key_len, 0); if (status) goto enc_out;
    { BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO a; BCRYPT_INIT_AUTH_MODE_INFO(a); a.pbNonce=nonce; a.cbNonce=12; a.pbAuthData=aad; a.cbAuthData=sizeof(aad); a.cbTag=16; a.pbTag=pbOutput+5+8+cbInput; status = BCryptEncrypt(key, pbInput, cbInput, &a, NULL, 0, pbOutput+5+8, cbInput, &ret, 0); }
enc_out: if (key) BCryptDestroyKey(key); if (keyobj) HeapFree(GetProcessHeap(),0,keyobj); if (alg) BCryptCloseAlgorithmProvider(alg,0); if (status) return status; if (pcbResult) *pcbResult = total; return STATUS_SUCCESS;
}

int WINAPI SslDecryptPacket(ULONG_PTR hSslProvider, ULONG_PTR hKey, unsigned __int8 *pbInput, unsigned int cbInput, unsigned __int8 *pbOutput, unsigned int cbOutput, unsigned int *pcbResult, unsigned __int64 SequenceNumber, unsigned int dwFlags)
{
    ssl_provider *p = (ssl_provider *)hSslProvider; ssl_symkey *rk = (ssl_symkey *)hKey; const suite_meta *m; NTSTATUS status; BYTE header[5];
    TRACE("SslDecryptPacket(%p, %p, %p, %u, %p, %u, %p, %I64u, %#x)\n", p, rk, pbInput, cbInput, pbOutput, cbOutput, pcbResult, SequenceNumber, dwFlags);
    if (!is_valid_provider(p) || !rk || rk->magic != 1145324613 || !pbInput) return STATUS_INVALID_PARAMETER;
    memcpy(header, pbInput, 5);
    m = find_suite(rk->suite); if (!m) return STATUS_NOT_SUPPORTED;
    if (!m->is_aead)
    {
        /* CBC + HMAC-SHA1 */
    DWORD block = rk->block_len ? rk->block_len : 16; DWORD expl = block; PUCHAR iv, ct; DWORD ct_len; BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_KEY_HANDLE key=NULL; DWORD obj_len=0, cbres=0; BYTE *keyobj=NULL; DWORD outlen=0; BYTE *plain; DWORD data_plus_mac_len, padlen, data_len; BYTE seqhdr[13]; BYTE mac_calc[32];
        if (cbInput < 5 + expl + block) return STATUS_INVALID_PARAMETER; /* at least one block */
        if (((pbInput[3]<<8)|pbInput[4]) + 5 != cbInput) return STATUS_INVALID_PARAMETER;
        iv = pbInput + 5;
        ct = iv + expl;
        ct_len = cbInput - 5 - expl;
        if (ct_len % block) return STATUS_INVALID_PARAMETER;
    /* We can't know plaintext length until after decrypt and padding/MAC check; defer size check */
        /* Decrypt */
        plain = HeapAlloc(GetProcessHeap(),0, ct_len);
        if (!plain) return STATUS_NO_MEMORY;
        status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0); if (status) { HeapFree(GetProcessHeap(),0,plain); return status; }
        status = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, (ULONG)((wcslen(BCRYPT_CHAIN_MODE_CBC)+1)*sizeof(WCHAR)), 0); if (status) goto dec_out_cbc;
        status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&obj_len, sizeof(obj_len), &cbres, 0); if (status) goto dec_out_cbc;
        keyobj = HeapAlloc(GetProcessHeap(),0,obj_len); if (!keyobj) { status = STATUS_NO_MEMORY; goto dec_out_cbc; }
        status = BCryptGenerateSymmetricKey(alg, &key, keyobj, obj_len, rk->key, rk->key_len, 0); if (status) goto dec_out_cbc;
        status = BCryptDecrypt(key, ct, ct_len, NULL, iv, block, plain, ct_len, &outlen, 0);
        if (status) goto dec_out_cbc;
        /* Check padding: last byte value v means v+1 padding bytes */
        padlen = plain[ct_len-1];
        if (padlen >= block) { status = STATUS_INVALID_PARAMETER; goto dec_out_cbc; }
        /* MAC and data lengths */
        if (ct_len < (padlen+1) + 20) { status = STATUS_INVALID_PARAMETER; goto dec_out_cbc; }
        data_plus_mac_len = ct_len - (padlen+1);
        if (data_plus_mac_len < 20) { status = STATUS_INVALID_PARAMETER; goto dec_out_cbc; }
        data_len = data_plus_mac_len - 20;
        /* Verify MAC over seq+hdr(with inner length=data_len)+data */
        {
            DWORD i; BYTE *maccat = HeapAlloc(GetProcessHeap(),0, 13 + data_len);
            if (!maccat) { status = STATUS_NO_MEMORY; goto dec_out_cbc; }
            for (i=0;i<8;i++) seqhdr[i] = (BYTE)((SequenceNumber>>(56-8*i)) & 0xFF);
            seqhdr[8] = header[0]; seqhdr[9]=header[1]; seqhdr[10]=header[2]; seqhdr[11]=(BYTE)(data_len>>8); seqhdr[12]=(BYTE)(data_len&0xFF);
            memcpy(maccat, seqhdr, 13);
            memcpy(maccat+13, plain, data_len);
            if (m && m->hash_len == 32)
                status = hmac_sha256_mac(rk->mac_key, rk->mac_len ? rk->mac_len : 32, maccat, 13 + data_len, mac_calc, 32);
            else
                status = hmac_sha1(rk->mac_key, rk->mac_len ? rk->mac_len : 20, maccat, 13 + data_len, mac_calc, 20);
            HeapFree(GetProcessHeap(),0,maccat);
            if (status) goto dec_out_cbc;
            if (!const_time_eq(mac_calc, plain+data_len, (m && m->hash_len == 32) ? 32 : 20)) { status = STATUS_INVALID_PARAMETER; goto dec_out_cbc; }
        }
    if (!pbOutput || cbOutput < data_len) { if (pcbResult) *pcbResult = data_len; status = STATUS_BUFFER_TOO_SMALL; goto dec_out_cbc; }
    memcpy(pbOutput, plain, data_len);
    if (pcbResult) *pcbResult = data_len;
        status = STATUS_SUCCESS;
dec_out_cbc:
        if (key) BCryptDestroyKey(key); if (keyobj) HeapFree(GetProcessHeap(),0,keyobj); if (alg) BCryptCloseAlgorithmProvider(alg,0); if (plain) { SecureZeroMemory(plain, ct_len); HeapFree(GetProcessHeap(),0,plain);} if (status) return status; return STATUS_SUCCESS;
    }
    else
    {
        /* GCM */
        DWORD hdr=5, expl=8, tag=16; BYTE nonce[12], aad[13]; BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_KEY_HANDLE key=NULL; DWORD obj_len=0, cbres=0, out=0; BYTE *keyobj=NULL; PUCHAR explicit_nonce, ct, t; DWORD ct_len; BYTE header2[5]; NTSTATUS st2;
        if (cbInput < hdr+expl+tag) return STATUS_INVALID_PARAMETER; memcpy(header2, pbInput, 5); if (((pbInput[3]<<8)|pbInput[4]) + hdr != cbInput) return STATUS_INVALID_PARAMETER;
        explicit_nonce = pbInput+5; ct = explicit_nonce+expl; ct_len = cbInput - hdr - expl - tag; t = pbInput + cbInput - tag; if (!pbOutput || cbOutput < ct_len) { if (pcbResult) *pcbResult = ct_len; return STATUS_BUFFER_TOO_SMALL; }
        if (rk->salt_len < 4) return STATUS_INVALID_PARAMETER; memcpy(nonce, rk->salt, 4); memcpy(nonce+4, explicit_nonce, 8);
        { int i; for(i=0;i<8;i++) aad[i]=(BYTE)((SequenceNumber>>(56-8*i))&0xFF); }
        aad[8]=header2[0]; aad[9]=header2[1]; aad[10]=header2[2]; aad[11]=(BYTE)(ct_len>>8); aad[12]=(BYTE)(ct_len&0xFF);
        st2 = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0); if (st2) return st2; st2 = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, (ULONG)((wcslen(BCRYPT_CHAIN_MODE_GCM)+1)*sizeof(WCHAR)), 0); if (st2) goto dec_out_gcm; st2 = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&obj_len, sizeof(obj_len), &cbres, 0); if (st2) goto dec_out_gcm; keyobj = HeapAlloc(GetProcessHeap(),0,obj_len); if (!keyobj) { st2 = STATUS_NO_MEMORY; goto dec_out_gcm; } st2 = BCryptGenerateSymmetricKey(alg, &key, keyobj, obj_len, rk->key, rk->key_len, 0); if (st2) goto dec_out_gcm;
        { BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO a; BCRYPT_INIT_AUTH_MODE_INFO(a); a.pbNonce=nonce; a.cbNonce=12; a.pbAuthData=aad; a.cbAuthData=sizeof(aad); a.pbTag=t; a.cbTag=16; st2 = BCryptDecrypt(key, ct, ct_len, &a, NULL, 0, pbOutput, ct_len, &out, 0); }
dec_out_gcm: if (key) BCryptDestroyKey(key); if (keyobj) HeapFree(GetProcessHeap(),0,keyobj); if (alg) BCryptCloseAlgorithmProvider(alg,0); if (st2) return st2; if (pcbResult) *pcbResult = out; return STATUS_SUCCESS;
    }
}
