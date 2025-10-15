/*
 * New cryptographic library (ncrypt.dll)
 *
 * Copyright 2016 Alex Henrie
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

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ncrypt.h"
#include "bcrypt.h"
#include "ncrypt_internal.h"
#include <wincrypt.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ncrypt);

static SECURITY_STATUS map_ntstatus(NTSTATUS status)
{
    switch (status)
    {
    case STATUS_INVALID_HANDLE:    return NTE_INVALID_HANDLE;
    case STATUS_INVALID_SIGNATURE: return NTE_BAD_SIGNATURE;
    case STATUS_SUCCESS:           return ERROR_SUCCESS;
    case STATUS_INVALID_PARAMETER: return NTE_INVALID_PARAMETER;
    case STATUS_NO_MEMORY:         return NTE_NO_MEMORY;
    case STATUS_NOT_SUPPORTED:     return NTE_NOT_SUPPORTED;
    case NTE_BAD_DATA:             return NTE_BAD_DATA;
    case STATUS_BUFFER_TOO_SMALL:  return NTE_BUFFER_TOO_SMALL;
    default:
        FIXME("unhandled status %#lx\n", status);
        return NTE_INTERNAL_ERROR;
    }
}

static void reverse_bytes(BYTE *dst, const BYTE *src, DWORD len)
{
    DWORD i;
    for (i = 0; i < len; ++i) dst[i] = src[len - 1 - i];
}

static DWORD be_uint_len_from_dword(DWORD v)
{
    /* minimal big-endian length without leading zeros; return at least 1 */
    if (!v) return 1;
    if (v <= 0xFF) return 1;
    if (v <= 0xFFFF) return 2;
    if (v <= 0xFFFFFF) return 3;
    return 4;
}

static SECURITY_STATUS free_key_object(struct key *key)
{
    NTSTATUS status;

    if (!key) return NTE_INVALID_HANDLE;
    if (key->bcrypt_key)
    {
        status = BCryptDestroyKey(key->bcrypt_key);
        if (status != STATUS_SUCCESS)
        {
            ERR("Error destroying key %#lx\n", status);
            return map_ntstatus(status);
        }
        key->bcrypt_key = NULL;
    }

    return ERROR_SUCCESS;
}

static struct object *allocate_object(enum object_type type)
{
    struct object *ret;
    if (!(ret = calloc(1, sizeof(*ret)))) return NULL;
    ret->type = type;
    return ret;
}

static struct object_property *get_object_property(struct object *object, const WCHAR *name)
{
    unsigned int i;
    for (i = 0; i < object->num_properties; i++)
    {
        struct object_property *property = &object->properties[i];
        if (!lstrcmpW(property->key, name)) return property;
    }
    return NULL;
}

struct object_property *add_object_property(struct object *object, const WCHAR *name)
{
    struct object_property *property;

    if (!object->num_properties)
    {
        if (!(object->properties = malloc(sizeof(*property))))
        {
            ERR("Error allocating memory.\n");
            return NULL;
        }
        property = &object->properties[object->num_properties++];
    }
    else
    {
        struct object_property *tmp;
        if (!(tmp = realloc(object->properties, sizeof(*property) * (object->num_properties + 1))))
        {
            ERR("Error allocating memory.\n");
            return NULL;
        }
        object->properties = tmp;
        property = &object->properties[object->num_properties++];
    }

    memset(property, 0, sizeof(*property));
    if (!(property->key = wcsdup(name)))
    {
        ERR("Error allocating memory.\n");
        return NULL;
    }

    return property;
}

static SECURITY_STATUS set_object_property(struct object *object, const WCHAR *name, BYTE *value, DWORD value_size)
{
    struct object_property *property = get_object_property(object, name);
    void *tmp;

    if (!property && !(property = add_object_property(object, name))) return NTE_NO_MEMORY;

    property->value_size = value_size;
    if (!(tmp = realloc(property->value, value_size)))
    {
        ERR("Error allocating memory.\n");
        free(property->key);
        property->key = NULL;
        return NTE_NO_MEMORY;
    }

    property->value = tmp;
    memcpy(property->value, value, value_size);

    return ERROR_SUCCESS;
}

static struct object *create_key_object(enum algid algid, NCRYPT_PROV_HANDLE provider)
{
    NCRYPT_SUPPORTED_LENGTHS supported_lengths = {512, 16384, 8, 1024};
    struct object *object;
    DWORD dw_value;

    switch (algid)
    {
    case RSA:
        if (!(object = allocate_object(KEY))) return NULL;

        object->key.algid = RSA;
        set_object_property(object, NCRYPT_ALGORITHM_PROPERTY, (BYTE *)BCRYPT_RSA_ALGORITHM,
                            sizeof(BCRYPT_RSA_ALGORITHM));
        set_object_property(object, NCRYPT_ALGORITHM_GROUP_PROPERTY, (BYTE *)BCRYPT_RSA_ALGORITHM,
                            sizeof(BCRYPT_RSA_ALGORITHM));
        set_object_property(object, NCRYPT_LENGTHS_PROPERTY, (BYTE *)&supported_lengths,
                            sizeof(supported_lengths));
        dw_value = 128;
        set_object_property(object, NCRYPT_BLOCK_LENGTH_PROPERTY, (BYTE *)&dw_value, sizeof(dw_value));
        dw_value = 128;
        set_object_property(object, BCRYPT_SIGNATURE_LENGTH, (BYTE *)&dw_value, sizeof(dw_value));
        break;

    default:
        ERR("Invalid algid %#x\n", algid);
        return NULL;
    }

    dw_value = 0;
    set_object_property(object, NCRYPT_EXPORT_POLICY_PROPERTY, (BYTE *)&dw_value, sizeof(dw_value));
    dw_value = NCRYPT_ALLOW_ALL_USAGES;
    set_object_property(object, NCRYPT_KEY_USAGE_PROPERTY, (BYTE *)&dw_value, sizeof(dw_value));
    dw_value = 0;
    set_object_property(object, NCRYPT_KEY_TYPE_PROPERTY, (BYTE *)&dw_value, sizeof(dw_value));
    set_object_property(object, NCRYPT_PROVIDER_HANDLE_PROPERTY, (BYTE *)&provider, sizeof(provider));
    return object;
}

SECURITY_STATUS WINAPI NCryptCreatePersistedKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE *key,
                                                const WCHAR *algid, const WCHAR *name, DWORD keyspec, DWORD flags)
{
    struct object *object;

    TRACE("(%#Ix, %p, %s, %s, %#lx, %#lx)\n", provider, key, wine_dbgstr_w(algid),
          wine_dbgstr_w(name), keyspec, flags);

    if (!provider) return NTE_INVALID_HANDLE;
    if (!algid) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (name) FIXME("Persistent keys are not supported\n");

    if (!lstrcmpiW(algid, BCRYPT_RSA_ALGORITHM))
    {
        NTSTATUS status;
        DWORD default_bitlen = 1024;

        if (!(object = create_key_object(RSA, provider)))
        {
            ERR("Error allocating memory\n");
            return NTE_NO_MEMORY;
        }

        status = BCryptGenerateKeyPair(BCRYPT_RSA_ALG_HANDLE, &object->key.bcrypt_key, default_bitlen, 0);
        if (status != STATUS_SUCCESS)
        {
            ERR("Error generating key pair %#lx\n", status);
            free(object);
            return map_ntstatus(status);
        }

        set_object_property(object, NCRYPT_LENGTH_PROPERTY, (BYTE *)&default_bitlen, sizeof(default_bitlen));
        set_object_property(object, BCRYPT_PUBLIC_KEY_LENGTH, (BYTE *)&default_bitlen, sizeof(default_bitlen));
    }
    else
    {
        FIXME("Algorithm not handled %s\n", wine_dbgstr_w(algid));
        return NTE_NOT_SUPPORTED;
    }

    *key = (NCRYPT_KEY_HANDLE)object;
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptDecrypt(NCRYPT_KEY_HANDLE key, BYTE *input, DWORD insize, void *padding,
                                     BYTE *output, DWORD outsize, DWORD *result, DWORD flags)
{
    struct object *key_object = (struct object *)key;
    ULONG bflags = 0;
    NTSTATUS status;
    DWORD needed = 0;
    DWORD key_bits;
    ULONG cb;

    TRACE("(%#Ix, %p, %lu, %p, %p, %lu, %p, %#lx)\n", key, input, insize, padding,
          output, outsize, result, flags);

    if (!key_object || key_object->type != KEY) return NTE_INVALID_HANDLE;
    if (!input && insize) return NTE_INVALID_PARAMETER;
    if (!result) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (output == NULL && outsize) return NTE_INVALID_PARAMETER;

    /* Validate and translate flags */
    if (flags & ~(NCRYPT_NO_PADDING_FLAG | NCRYPT_PAD_OAEP_FLAG | NCRYPT_PAD_PKCS1_FLAG | NCRYPT_SILENT_FLAG))
    {
        WARN("Invalid flags %#lx\n", flags);
        return NTE_BAD_FLAGS;
    }
    if ((flags & NCRYPT_PAD_OAEP_FLAG) && (flags & NCRYPT_PAD_PKCS1_FLAG))
    {
        WARN("Mutually exclusive flags OAEP and PKCS1 set\n");
        return NTE_BAD_FLAGS;
    }

    if (flags & NCRYPT_PAD_OAEP_FLAG) bflags |= BCRYPT_PAD_OAEP;
    if (flags & NCRYPT_PAD_PKCS1_FLAG) bflags |= BCRYPT_PAD_PKCS1;
    /* NCRYPT_NO_PADDING_FLAG implies no bcrypt padding flag */

    /* Optional size query: try provider; if not supported, fallback to modulus size */
    if (!output || !outsize)
    {
        status = BCryptDecrypt(key_object->key.bcrypt_key, input, insize, padding,
                               NULL, 0, NULL, 0, &needed, bflags);
        if (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL)
        {
            *result = needed;
            return ERROR_SUCCESS;
        }

        /* Fallback: use modulus size as upper bound */
        status = BCryptGetProperty(key_object->key.bcrypt_key, BCRYPT_KEY_LENGTH, (UCHAR *)&key_bits,
                                   sizeof(key_bits), &cb, 0);
        if (status == STATUS_SUCCESS)
        {
            needed = (key_bits + 7) / 8;
            *result = needed; /* best-effort */
            return ERROR_SUCCESS;
        }
        return map_ntstatus(status);
    }

    /* If caller provided buffer, attempt decrypt; STATUS_BUFFER_TOO_SMALL maps accordingly */
    status = BCryptDecrypt(key_object->key.bcrypt_key, input, insize, padding,
                           NULL, 0, output, outsize, result, bflags);
    return map_ntstatus(status);
}

SECURITY_STATUS WINAPI NCryptDeleteKey(NCRYPT_KEY_HANDLE key, DWORD flags)
{
    TRACE("(%#Ix, %#lx)\n", key, flags);
    /* No persistence supported; delete behaves as free. */
    return NCryptFreeObject((NCRYPT_HANDLE)key);
}

SECURITY_STATUS WINAPI NCryptEncrypt(NCRYPT_KEY_HANDLE key, BYTE *input, DWORD insize, void *padding,
                                     BYTE *output, DWORD outsize, DWORD *result, DWORD flags)
{
    struct object *key_object = (struct object *)key;
    ULONG bflags = 0;
    NTSTATUS status;
    DWORD needed = 0;
    DWORD key_bits;
    ULONG cb;

    TRACE("(%#Ix, %p, %lu, %p, %p, %lu, %p, %#lx)\n", key, input, insize, padding,
          output, outsize, result, flags);

    if (!key_object || key_object->type != KEY) return NTE_INVALID_HANDLE;
    if (!input && insize) return NTE_INVALID_PARAMETER;
    if (!result) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (output == NULL && outsize) return NTE_INVALID_PARAMETER;

    /* Validate and translate flags */
    if (flags & ~(NCRYPT_NO_PADDING_FLAG | NCRYPT_PAD_OAEP_FLAG | NCRYPT_PAD_PKCS1_FLAG | NCRYPT_SILENT_FLAG))
    {
        WARN("Invalid flags %#lx\n", flags);
        return NTE_BAD_FLAGS;
    }
    if ((flags & NCRYPT_PAD_OAEP_FLAG) && (flags & NCRYPT_PAD_PKCS1_FLAG))
    {
        WARN("Mutually exclusive flags OAEP and PKCS1 set\n");
        return NTE_BAD_FLAGS;
    }

    if (flags & NCRYPT_PAD_OAEP_FLAG) bflags |= BCRYPT_PAD_OAEP;
    if (flags & NCRYPT_PAD_PKCS1_FLAG) bflags |= BCRYPT_PAD_PKCS1;
    /* NCRYPT_NO_PADDING_FLAG implies no bcrypt padding flag */

    /* Size-query: derive required size without invoking provider (some backends cannot probe) */
    if (!output || !outsize)
    {
        /* Prefer bcrypt key length; fallback to stored NCrypt property */
        status = BCryptGetProperty(key_object->key.bcrypt_key, BCRYPT_KEY_LENGTH, (UCHAR *)&key_bits,
                                   sizeof(key_bits), &cb, 0);
        if (status != STATUS_SUCCESS)
        {
            struct object_property *prop = get_object_property(key_object, NCRYPT_LENGTH_PROPERTY);
            if (!prop || prop->value_size < sizeof(DWORD))
            {
                TRACE("failed to obtain key length, status %#lx\n", status);
                return map_ntstatus(status);
            }
            key_bits = *(DWORD *)prop->value;
        }
        needed = (key_bits + 7) / 8; /* RSA ciphertext size equals modulus size */
        *result = needed;
        return ERROR_SUCCESS;
    }

    if (outsize < needed)
    {
        *result = needed;
        return NTE_BUFFER_TOO_SMALL;
    }

    status = BCryptEncrypt(key_object->key.bcrypt_key, input, insize, padding,
                           NULL, 0, output, outsize, result, bflags);
    return map_ntstatus(status);
}

SECURITY_STATUS WINAPI NCryptEnumAlgorithms(NCRYPT_PROV_HANDLE provider, DWORD alg_ops,
                                            DWORD *alg_count, NCryptAlgorithmName **alg_list, DWORD flags)
{
    BCRYPT_ALGORITHM_IDENTIFIER *list = NULL;
    ULONG count = 0, i;
    NTSTATUS status;
    SIZE_T total;
    BYTE *mem;
    NCryptAlgorithmName *out;
    WCHAR *str_pool;

    TRACE("(%#Ix, %#lx, %p, %p, %#lx)\n", provider, alg_ops, alg_count, alg_list, flags);
    if (!provider) return NTE_INVALID_HANDLE;
    if (!alg_count || !alg_list) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (flags)
    {
        if (flags == NCRYPT_SILENT_FLAG) FIXME("Silent flag not implemented\n");
        else { WARN("Invalid flags %#lx\n", flags); return NTE_BAD_FLAGS; }
    }

    status = BCryptEnumAlgorithms(alg_ops, &count, &list, 0);
    if (status != STATUS_SUCCESS)
    {
        ERR("BCryptEnumAlgorithms failed %#lx\n", status);
        return map_ntstatus(status);
    }

    if (!count)
    {
        *alg_count = 0;
        *alg_list = NULL;
        BCryptFreeBuffer(list);
        return ERROR_SUCCESS;
    }

    /* compute total size for contiguous array + strings */
    total = sizeof(NCryptAlgorithmName) * count;
    for (i = 0; i < count; ++i)
    {
        if (list[i].pszName) total += (lstrlenW(list[i].pszName) + 1) * sizeof(WCHAR);
        else total += sizeof(WCHAR); /* empty */
    }

    mem = malloc(total);
    if (!mem)
    {
        BCryptFreeBuffer(list);
        return NTE_NO_MEMORY;
    }

    out = (NCryptAlgorithmName *)mem;
    str_pool = (WCHAR *)(mem + sizeof(NCryptAlgorithmName) * count);

    for (i = 0; i < count; ++i)
    {
        out[i].dwClass = list[i].dwClass;
        out[i].dwFlags = list[i].dwFlags;
        out[i].dwAlgOperations = alg_ops; /* best-effort */
        if (list[i].pszName)
        {
            lstrcpyW(str_pool, list[i].pszName);
        }
        else
        {
            *str_pool = 0;
        }
        out[i].pszName = str_pool;
        str_pool += lstrlenW(out[i].pszName) + 1;
    }

    *alg_count = count;
    *alg_list = out;
    BCryptFreeBuffer(list);
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptEnumKeys(NCRYPT_PROV_HANDLE provider, const WCHAR *scope,
                                      NCryptKeyName **key_name, PVOID *enum_state, DWORD flags)
{
    FIXME("(%#Ix, %p, %p, %p, %#lx): stub\n", provider, scope, key_name, enum_state, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptFinalizeKey(NCRYPT_KEY_HANDLE handle, DWORD flags)
{
    struct object *object = (struct object *)handle;
    DWORD key_length;
    struct object_property *prop;
    NTSTATUS status;

    TRACE("(%#Ix, %#lx)\n", handle, flags);

    if (!object || object->type != KEY) return NTE_INVALID_HANDLE;

    if (!(prop = get_object_property(object, NCRYPT_LENGTH_PROPERTY))) return NTE_INVALID_HANDLE;
    key_length = *(DWORD *)prop->value;
    status = BCryptSetProperty(object->key.bcrypt_key, BCRYPT_KEY_LENGTH, (UCHAR *)&key_length, sizeof(key_length), 0);
    if (status != STATUS_SUCCESS)
    {
        ERR("Error setting key length property\n");
        return map_ntstatus(status);
    }

    status = BCryptFinalizeKeyPair(object->key.bcrypt_key, 0);
    if (status != STATUS_SUCCESS)
    {
        ERR("Error finalizing key pair\n");
        return map_ntstatus(status);
    }

    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptFreeBuffer(PVOID buf)
{
    TRACE("(%p)\n", buf);
    if (buf) free(buf);
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptFreeObject(NCRYPT_HANDLE handle)
{
    struct object *object = (struct object *)handle;
    SECURITY_STATUS ret = STATUS_SUCCESS;
    unsigned int i;

    TRACE("(%#Ix)\n", handle);

    if (!object)
    {
        WARN("invalid handle %#Ix\n", handle);
        return NTE_INVALID_HANDLE;
    }

    switch (object->type)
    {
    case KEY:
    {
        if ((ret = free_key_object(&object->key))) return ret;
        break;
    }
    case STORAGE_PROVIDER:
        break;

    default:
        WARN("invalid handle %#Ix\n", handle);
        return NTE_INVALID_HANDLE;
    }

    for (i = 0; i < object->num_properties; i++)
    {
        free(object->properties[i].key);
        free(object->properties[i].value);
    }
    free(object->properties);
    free(object);
    return ret;
}

SECURITY_STATUS WINAPI NCryptGetProperty(NCRYPT_HANDLE handle, const WCHAR *name, BYTE *output,
                                         DWORD outsize, DWORD *result, DWORD flags)
{
    struct object *object = (struct object *)handle;
    const struct object_property *property;

    TRACE("(%#Ix, %s, %p, %lu, %p, %#lx)\n", handle, wine_dbgstr_w(name), output, outsize, result, flags);
    if (flags) FIXME("flags %#lx not supported\n", flags);

    if (!object) return NTE_INVALID_HANDLE;
    if (!(property = get_object_property(object, name))) return NTE_INVALID_PARAMETER;

    *result = property->value_size;
    if (!output) return ERROR_SUCCESS;
    if (outsize < property->value_size) return NTE_BUFFER_TOO_SMALL;

    memcpy(output, property->value, property->value_size);
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptImportKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE decrypt_key,
                                       const WCHAR *type, NCryptBufferDesc *params, NCRYPT_KEY_HANDLE *handle,
                                       BYTE *data, DWORD datasize, DWORD flags)
{
    BCRYPT_KEY_BLOB *header = (BCRYPT_KEY_BLOB *)data;
    struct object *object;

    TRACE("(%#Ix, %#Ix, %s, %p, %p, %p, %lu, %#lx)\n", provider, decrypt_key, wine_dbgstr_w(type),
          params, handle, data, datasize, flags);

    if (decrypt_key)
    {
        FIXME("Key blob decryption not implemented\n");
        return NTE_NOT_SUPPORTED;
    }
    if (params)
    {
        FIXME("Parameter information not implemented\n");
        return NTE_NOT_SUPPORTED;
    }
    if (flags == NCRYPT_SILENT_FLAG)
    {
        FIXME("Silent flag not implemented\n");
    }
    else if (flags)
    {
        WARN("Invalid flags %#lx\n", flags);
        return NTE_BAD_FLAGS;
    }

    switch(header->Magic)
    {
    case BCRYPT_RSAFULLPRIVATE_MAGIC:
    case BCRYPT_RSAPRIVATE_MAGIC:
    case BCRYPT_RSAPUBLIC_MAGIC:
    {
        NTSTATUS status;
        BCRYPT_RSAKEY_BLOB *rsablob = (BCRYPT_RSAKEY_BLOB *)data;

        if (!(object = create_key_object(RSA, provider)))
        {
            ERR("Error allocating memory\n");
            return NTE_NO_MEMORY;
        }

        status = BCryptImportKeyPair(BCRYPT_RSA_ALG_HANDLE, NULL, type, &object->key.bcrypt_key, data, datasize, 0);
        if (status != STATUS_SUCCESS)
        {
            WARN("Error importing key pair %#lx\n", status);
            free(object);
            return map_ntstatus(status);
        }

        set_object_property(object, NCRYPT_LENGTH_PROPERTY, (BYTE *)&rsablob->BitLength, sizeof(rsablob->BitLength));
        set_object_property(object, BCRYPT_PUBLIC_KEY_LENGTH, (BYTE *)&rsablob->BitLength, sizeof(rsablob->BitLength));
        break;
    }
    default:
        FIXME("Unhandled key magic %#lx\n", header->Magic);
        return NTE_INVALID_PARAMETER;
    }

    *handle = (NCRYPT_KEY_HANDLE)object;
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptExportKey(NCRYPT_KEY_HANDLE key, NCRYPT_KEY_HANDLE encrypt_key, const WCHAR *type,
                                       NCryptBufferDesc *params, BYTE *output, DWORD output_len, DWORD *ret_len,
                                       DWORD flags)
{
    struct object *object = (struct object *)key;

    TRACE("(%#Ix, %#Ix, %s, %p, %p, %lu, %p, %#lx)\n", key, encrypt_key, wine_dbgstr_w(type), params, output,
          output_len, ret_len, flags);

    if (encrypt_key)
    {
        FIXME("Key blob encryption not implemented\n");
        return NTE_NOT_SUPPORTED;
    }
    if (params)
    {
        FIXME("Parameter information not implemented\n");
        return NTE_NOT_SUPPORTED;
    }
    if (flags == NCRYPT_SILENT_FLAG)
    {
        FIXME("Silent flag not implemented\n");
    }

    return map_ntstatus(BCryptExportKey(object->key.bcrypt_key, NULL, type, output, output_len, ret_len, 0));
}

SECURITY_STATUS WINAPI NCryptIsAlgSupported(NCRYPT_PROV_HANDLE provider, const WCHAR *algid, DWORD flags)
{
    static const ULONG supported = BCRYPT_CIPHER_OPERATION |\
                                   BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION |\
                                   BCRYPT_SIGNATURE_OPERATION |\
                                   BCRYPT_SECRET_AGREEMENT_OPERATION;
    BCRYPT_ALGORITHM_IDENTIFIER *list;
    ULONG i, count;
    NTSTATUS status;

    TRACE("(%#Ix, %s, %#lx)\n", provider, wine_dbgstr_w(algid), flags);

    if (!provider) return NTE_INVALID_HANDLE;
    if (!algid) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (flags == NCRYPT_SILENT_FLAG)
    {
        FIXME("Silent flag not implemented\n");
    }
    else if (flags)
    {
        WARN("Invalid flags %#lx\n", flags);
        return NTE_BAD_FLAGS;
    }
    if (!lstrcmpiW(BCRYPT_RSA_SIGN_ALGORITHM, algid)) return NTE_NOT_SUPPORTED;

    status = BCryptEnumAlgorithms(supported, &count, &list, 0);
    if (status != STATUS_SUCCESS)
    {
        ERR("Error retrieving algorithm list %#lx\n", status);
        return map_ntstatus(status);
    }

    status = STATUS_NOT_SUPPORTED;
    for (i = 0; i < count; i++)
    {
        if (!lstrcmpiW(list[i].pszName, algid))
        {
            status = STATUS_SUCCESS;
            break;
        }
    }

    BCryptFreeBuffer(list);
    return map_ntstatus(status);
}

BOOL WINAPI NCryptIsKeyHandle(NCRYPT_KEY_HANDLE hKey)
{
    struct object *obj = (struct object *)hKey;
    TRACE("(%#Ix)\n", hKey);
    if (!obj) return FALSE;
    return obj->type == KEY;
}
SECURITY_STATUS WINAPI NCryptOpenKey(NCRYPT_PROV_HANDLE provider, NCRYPT_KEY_HANDLE *key,
                                     const WCHAR *name, DWORD keyspec, DWORD flags)
{
    FIXME("(%#Ix, %p, %s, %#lx, %#lx): stub\n", provider, key, wine_dbgstr_w(name), keyspec, flags);
    return NTE_NOT_SUPPORTED;
}

SECURITY_STATUS WINAPI NCryptOpenStorageProvider(NCRYPT_PROV_HANDLE *provider, const WCHAR *name, DWORD flags)
{
    struct object *object;

    FIXME("(%p, %s, %#lx): stub\n", provider, wine_dbgstr_w(name), flags);

    if (!(object = allocate_object(STORAGE_PROVIDER)))
    {
        ERR("Error allocating memory.\n");
        return NTE_NO_MEMORY;
    }
    *provider = (NCRYPT_PROV_HANDLE)object;
    return ERROR_SUCCESS;
}

SECURITY_STATUS WINAPI NCryptSetProperty(NCRYPT_HANDLE handle, const WCHAR *name, BYTE *input, DWORD insize, DWORD flags)
{
    struct object *object = (struct object *)handle;

    TRACE("(%#Ix, %s, %p, %lu, %#lx)\n", handle, wine_dbgstr_w(name), input, insize, flags);
    if (flags) FIXME("flags %#lx not supported\n", flags);

    if (!object) return NTE_INVALID_HANDLE;
    return set_object_property(object, name, input, insize);
}

SECURITY_STATUS WINAPI NCryptSignHash(NCRYPT_KEY_HANDLE handle, void *padding, BYTE *value, DWORD value_len,
                                      BYTE *sig, DWORD sig_len, DWORD *ret_len, DWORD flags)
{
    struct object *object = (struct object *)handle;

    TRACE("(%#Ix, %p, %p, %lu, %p, %lu, %#lx)\n", handle, padding, value, value_len, sig, sig_len, flags);
    if (flags & NCRYPT_SILENT_FLAG) FIXME("Silent flag not implemented\n");

    if (!object || object->type != KEY) return NTE_INVALID_HANDLE;

    return map_ntstatus(BCryptSignHash(object->key.bcrypt_key, padding, value, value_len, sig, sig_len,
                                       ret_len, flags & ~NCRYPT_SILENT_FLAG));
}

SECURITY_STATUS WINAPI NCryptVerifySignature(NCRYPT_KEY_HANDLE handle, void *padding, BYTE *hash, DWORD hash_size,
                                             BYTE *signature, DWORD signature_size, DWORD flags)
{
    struct object *key_object = (struct object *)handle;

    TRACE("(%#Ix, %p, %p, %lu, %p, %lu, %#lx)\n", handle, padding, hash, hash_size, signature,
          signature_size, flags);

    if (!hash_size || !signature_size) return NTE_INVALID_PARAMETER;
    if (!hash || !signature) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (!handle || key_object->type != KEY) return NTE_INVALID_HANDLE;

    if (key_object->key.algid < RSA)
    {
        FIXME("Symmetric keys not supported.\n");
        return NTE_NOT_SUPPORTED;
    }

    return map_ntstatus(BCryptVerifySignature(key_object->key.bcrypt_key, padding, hash, hash_size, signature,
                                              signature_size, flags));
}

SECURITY_STATUS WINAPI NCryptTranslateHandle(NCRYPT_PROV_HANDLE *phProvider,
                                             NCRYPT_KEY_HANDLE *phKey,
                                             HCRYPTPROV hLegacyProv,
                                             HCRYPTKEY hLegacyKey,
                                             DWORD dwLegacyKeySpec,
                                             DWORD dwFlags)
{
    struct object *object = NULL;
    NCRYPT_PROV_HANDLE cng_provider = 0;
    HCRYPTKEY legacy_key = hLegacyKey;
    DWORD keyspec = dwLegacyKeySpec;
    DWORD err;

    TRACE("(%p, %p, %#Ix, %#Ix, %#lx, %#lx)\n", phProvider, phKey, hLegacyProv, hLegacyKey,
          dwLegacyKeySpec, dwFlags);

    if (!phKey) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);
    if (!hLegacyProv) return NTE_INVALID_HANDLE;
    if (dwFlags) { WARN("Invalid flags %#lx\n", dwFlags); return NTE_BAD_FLAGS; }

    /* Open a CNG storage provider (name is ignored in our implementation) */
    if (phProvider)
    {
        SECURITY_STATUS s = NCryptOpenStorageProvider(&cng_provider, NULL, 0);
        if (s != ERROR_SUCCESS) return s;
        *phProvider = cng_provider;
    }

    /* Determine legacy keyspec if not provided */
    if (!keyspec)
    {
        if (legacy_key)
        {
            DWORD algid, sz = sizeof(algid);
            if (!CryptGetKeyParam(legacy_key, KP_ALGID, (BYTE *)&algid, &sz, 0))
            {
                err = GetLastError();
                return HRESULT_FROM_WIN32(err);
            }
            if (algid == CALG_RSA_SIGN) keyspec = AT_SIGNATURE;
            else if (algid == CALG_RSA_KEYX) keyspec = AT_KEYEXCHANGE;
            else
            {
                FIXME("Unsupported legacy ALG_ID %#lx\n", algid);
                return NTE_NOT_SUPPORTED;
            }
        }
        else
        {
            DWORD sz = sizeof(keyspec);
            if (!CryptGetProvParam(hLegacyProv, PP_KEYSPEC, (BYTE *)&keyspec, &sz, 0))
            {
                /* If not available, attempt default to signature */
                keyspec = AT_SIGNATURE;
            }
        }
    }

    /* Obtain legacy key handle if not supplied */
    if (!legacy_key)
    {
        if (!CryptGetUserKey(hLegacyProv, keyspec, &legacy_key))
        {
            err = GetLastError();
            return HRESULT_FROM_WIN32(err);
        }
    }

    /* Try to export private key first; fallback to public key */
    BYTE *legacy_blob = NULL;
    DWORD legacy_blob_len = 0;
    BOOL have_private = FALSE;
    if (CryptExportKey(legacy_key, 0, PRIVATEKEYBLOB, 0, NULL, &legacy_blob_len))
    {
        if (!(legacy_blob = malloc(legacy_blob_len)))
        {
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_NO_MEMORY;
        }
    if (!CryptExportKey(legacy_key, 0, PRIVATEKEYBLOB, 0, legacy_blob, &legacy_blob_len))
        {
            err = GetLastError();
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return HRESULT_FROM_WIN32(err);
        }
        have_private = TRUE;
    }
    else
    {
        legacy_blob_len = 0;
    if (!CryptExportKey(legacy_key, 0, PUBLICKEYBLOB, 0, NULL, &legacy_blob_len))
        {
            err = GetLastError();
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return HRESULT_FROM_WIN32(err);
        }
        legacy_blob = malloc(legacy_blob_len);
        if (!legacy_blob)
        {
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_NO_MEMORY;
        }
    if (!CryptExportKey(legacy_key, 0, PUBLICKEYBLOB, 0, legacy_blob, &legacy_blob_len))
        {
            err = GetLastError();
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return HRESULT_FROM_WIN32(err);
        }
    }

    /* Convert legacy RSA blob to CNG blob and import */
    {
        BLOBHEADER *hdr = (BLOBHEADER *)legacy_blob;
        BYTE *p = legacy_blob + sizeof(BLOBHEADER);
        DWORD remaining = legacy_blob_len - sizeof(BLOBHEADER);

        if (!legacy_blob_len || legacy_blob_len < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY))
        {
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_INVALID_PARAMETER;
        }

        if (hdr->aiKeyAlg != CALG_RSA_KEYX && hdr->aiKeyAlg != CALG_RSA_SIGN)
        {
            FIXME("Only RSA keys are supported (aiKeyAlg=%#lx)\n", hdr->aiKeyAlg);
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_NOT_SUPPORTED;
        }

        RSAPUBKEY *rsapub = (RSAPUBKEY *)p;
        DWORD bitlen = rsapub->bitlen;
        DWORD modlen = bitlen / 8;
        if (remaining < sizeof(RSAPUBKEY) + modlen)
        {
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_INVALID_PARAMETER;
        }

        /* Build CNG blob */
    DWORD e_len = be_uint_len_from_dword(rsapub->pubexp);
    BCRYPT_RSAKEY_BLOB keyblob;
    BYTE *cng_blob = NULL;
    DWORD cng_blob_len = 0;
    const WCHAR *import_type = NULL;
    int i;

        if (have_private && rsapub->magic == 0x32415352 /* 'RSA2' */)
        {
            DWORD prime_len = modlen / 2;
            /* legacy layout after modulus: P, Q, dp, dq, iq, d (all little-endian) */
            DWORD need = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + modlen /*n*/
                        + prime_len /*p*/ + prime_len /*q*/ + prime_len /*dp*/ + prime_len /*dq*/
                        + prime_len /*iq*/ + modlen /*d*/;
            if (legacy_blob_len < need)
            {
                free(legacy_blob);
                if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                return NTE_INVALID_PARAMETER;
            }

            keyblob.Magic = BCRYPT_RSAPRIVATE_MAGIC;
            keyblob.BitLength = bitlen;
            keyblob.cbPublicExp = e_len;
            keyblob.cbModulus = modlen;
            keyblob.cbPrime1 = prime_len;
            keyblob.cbPrime2 = prime_len;

            cng_blob_len = sizeof(BCRYPT_RSAKEY_BLOB) + e_len + modlen + prime_len + prime_len;
            cng_blob = malloc(cng_blob_len);
            if (!cng_blob)
            {
                free(legacy_blob);
                if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                return NTE_NO_MEMORY;
            }

            {
                BYTE *q;
                memcpy(cng_blob, &keyblob, sizeof(keyblob));
                q = cng_blob + sizeof(BCRYPT_RSAKEY_BLOB);

                /* public exponent, big-endian minimal */
                {
                    DWORD v = rsapub->pubexp;
                    for (i = e_len - 1; i >= 0; --i) { q[i] = (BYTE)(v & 0xFF); v >>= 8; }
                    q += e_len;
                }

                /* modulus (reverse from little-endian to big-endian) */
                reverse_bytes(q, (BYTE *)(rsapub + 1), modlen);
                q += modlen;

                /* prime1 and prime2 */
                reverse_bytes(q, (BYTE *)(rsapub + 1) + modlen, prime_len); q += prime_len;
                reverse_bytes(q, (BYTE *)(rsapub + 1) + modlen + prime_len, prime_len); q += prime_len;
            }

            import_type = BCRYPT_RSAPRIVATE_BLOB;
        }
        else if (!have_private && rsapub->magic == 0x31415352 /* 'RSA1' */)
        {
            keyblob.Magic = BCRYPT_RSAPUBLIC_MAGIC;
            keyblob.BitLength = bitlen;
            keyblob.cbPublicExp = e_len;
            keyblob.cbModulus = modlen;
            keyblob.cbPrime1 = 0;
            keyblob.cbPrime2 = 0;

            cng_blob_len = sizeof(BCRYPT_RSAKEY_BLOB) + e_len + modlen;
            cng_blob = malloc(cng_blob_len);
            if (!cng_blob)
            {
                free(legacy_blob);
                if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                return NTE_NO_MEMORY;
            }

            {
                BYTE *q;
                memcpy(cng_blob, &keyblob, sizeof(keyblob));
                q = cng_blob + sizeof(BCRYPT_RSAKEY_BLOB);
                {
                    DWORD v = rsapub->pubexp;
                    for (i = e_len - 1; i >= 0; --i) { q[i] = (BYTE)(v & 0xFF); v >>= 8; }
                    q += e_len;
                }
                reverse_bytes(q, (BYTE *)(rsapub + 1), modlen);
            }
            import_type = BCRYPT_RSAPUBLIC_BLOB;
        }
        else
        {
            FIXME("Unsupported or mismatched RSA magic: %#lx (private=%d)\n", rsapub->magic, have_private);
            free(legacy_blob);
            if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
            return NTE_NOT_SUPPORTED;
        }

        /* Build NCrypt key object and import */
        {
            NCRYPT_PROV_HANDLE prov_for_key = cng_provider;
            if (!prov_for_key)
            {
                /* If caller didn't request provider, still create one to attach to key */
                SECURITY_STATUS s = NCryptOpenStorageProvider(&prov_for_key, NULL, 0);
                if (s != ERROR_SUCCESS)
                {
                    free(cng_blob);
                    free(legacy_blob);
                    if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                    return s;
                }
            }

            object = create_key_object(RSA, prov_for_key);
            if (!object)
            {
                free(cng_blob);
                free(legacy_blob);
                if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                return NTE_NO_MEMORY;
            }

            {
                NTSTATUS status = BCryptImportKeyPair(BCRYPT_RSA_ALG_HANDLE, NULL, import_type,
                                                      &object->key.bcrypt_key, cng_blob, cng_blob_len, 0);
                if (status != STATUS_SUCCESS)
                {
                    WARN("BCryptImportKeyPair failed %#lx\n", status);
                    free(cng_blob);
                    free(legacy_blob);
                    free(object);
                    if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
                    return map_ntstatus(status);
                }
            }

            /* Set properties */
            set_object_property(object, NCRYPT_LENGTH_PROPERTY, (BYTE *)&bitlen, sizeof(bitlen));
            set_object_property(object, BCRYPT_PUBLIC_KEY_LENGTH, (BYTE *)&bitlen, sizeof(bitlen));
            set_object_property(object, NCRYPT_PROVIDER_HANDLE_PROPERTY, (BYTE *)&prov_for_key, sizeof(prov_for_key));

            *phKey = (NCRYPT_KEY_HANDLE)object;

            free(cng_blob);
            free(legacy_blob);
        }
    }

    if (hLegacyKey == 0) CryptDestroyKey(legacy_key);
    return ERROR_SUCCESS;
}
