
#ifndef __D3DXOF_PRIVATE_INCLUDED__
#define __D3DXOF_PRIVATE_INCLUDED__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dxfile.h"

#define MAX_NAME_LEN 40
#define MAX_ARRAY_DIM 4
#define MAX_MEMBERS 50
#define MAX_CHILDREN 200
#define MAX_TEMPLATES 200
#define MAX_OBJECTS 500
#define MAX_SUBOBJECTS 2000
#define MAX_STRINGS_BUFFER 10000

typedef struct {
    DWORD type;
    LONG idx_template;
    char name[MAX_NAME_LEN];
    ULONG nb_dims;
    BOOL  dim_fixed[MAX_ARRAY_DIM]; /* fixed or variable */
    ULONG dim_value[MAX_ARRAY_DIM]; /* fixed value or member index */
} member;

typedef struct {
    char name[MAX_NAME_LEN];
    GUID class_id;
    BOOL open;
    BOOL binary;
    ULONG nb_children;
    char children[MAX_CHILDREN][MAX_NAME_LEN];
    ULONG nb_members;
    member members[MAX_MEMBERS];
} xtemplate;

typedef struct {
    char* name;
    ULONG start;
    ULONG size;
} xobject_member;

struct _xobject {
   BOOL binary;
   struct _xobject* ptarget;
   char name[MAX_NAME_LEN];
   GUID class_id;
   GUID type;
   LPBYTE pdata;
   ULONG pos_data;
   DWORD size;
   ULONG nb_members;
   xobject_member members[MAX_MEMBERS];
   ULONG nb_children;
   ULONG nb_subobjects;
   struct _xobject * children[MAX_CHILDREN];
   struct _xobject * root;
};

typedef struct _xobject xobject;

typedef struct {
    IDirectXFile IDirectXFile_iface;
    LONG ref;
    ULONG nb_xtemplates;
    xtemplate xtemplates[MAX_TEMPLATES];
} IDirectXFileImpl;

typedef struct {
    IDirectXFileBinary IDirectXFileBinary_iface;
    LONG ref;
} IDirectXFileBinaryImpl;

typedef struct {
    IDirectXFileData IDirectXFileData_iface;
    LONG ref;
    xobject* pobj;
    int cur_enum_object;
    BOOL from_ref;
    ULONG level;
    LPBYTE pstrings;
} IDirectXFileDataImpl;

typedef struct {
    IDirectXFileDataReference IDirectXFileDataReference_iface;
    LONG ref;
    xobject* ptarget;
} IDirectXFileDataReferenceImpl;

typedef struct {
    IDirectXFileObject IDirectXFileObject_iface;
    LONG ref;
} IDirectXFileObjectImpl;

typedef struct {
  /* Buffer to parse */
  LPBYTE buffer;
  DWORD rem_bytes;
  /* Misc info */
  WORD current_token;
  BOOL token_present;
  BOOL txt;
  DWORD list_nb_elements;
  BOOL list_type_float;
  BOOL list_separator;
  ULONG cur_pos_data;
  LPBYTE cur_pstrings;
  BYTE value[512];
  xobject** pxo_globals;
  ULONG nb_pxo_globals;
  xobject* pxo_tab;
  IDirectXFileImpl* pdxf;
  xobject* pxo;
  xtemplate* pxt[MAX_SUBOBJECTS];
  ULONG level;
  LPBYTE pdata;
  ULONG capacity;
  LPBYTE pstrings;
} parse_buffer;

typedef struct {
    IDirectXFileEnumObject IDirectXFileEnumObject_iface;
    LONG ref;
    LPBYTE mapped_memory;
    LPBYTE decomp_buffer;
    parse_buffer buf;
    IDirectXFileImpl* pDirectXFile;
    ULONG nb_xobjects;
    xobject* xobjects[MAX_OBJECTS];
    IDirectXFileData* pRefObjects[MAX_OBJECTS];
} IDirectXFileEnumObjectImpl;

typedef struct {
    IDirectXFileSaveObject IDirectXFileSaveObject_iface;
    LONG ref;
} IDirectXFileSaveObjectImpl;

HRESULT IDirectXFileImpl_Create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;

HRESULT parse_header(parse_buffer *buf, BYTE **decomp_buffer_ptr) DECLSPEC_HIDDEN;
BOOL parse_object(parse_buffer * buf) DECLSPEC_HIDDEN;
BOOL parse_templates(parse_buffer * buf, BOOL templates_only) DECLSPEC_HIDDEN;

int mszip_decompress(int inlen, int outlen, char* inbuffer, char* outbuffer) DECLSPEC_HIDDEN;

#endif /* __D3DXOF_PRIVATE_INCLUDED__ */
