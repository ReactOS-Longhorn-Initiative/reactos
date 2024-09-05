/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellLibrary header
 * COPYRIGHT:   Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#ifndef _CSHELLLIBRARY_H_
#define _CSHELLLIBRARY_H_

class CShellLibrary :
    public CComCoClass<CShellLibrary, &CLSID_ShellLibrary>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellLibrary
{
public:
    CShellLibrary();
    ~CShellLibrary();

    /*** IShellLibrary methods ***/
    virtual HRESULT WINAPI AddFolder(IShellItem *psiLocation);
    virtual HRESULT WINAPI Commit();
    virtual HRESULT WINAPI GetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, REFIID riid, void **ppv);
    virtual HRESULT WINAPI GetFolders(LIBRARYFOLDERFILTER lff, REFIID riid, void **ppv);
    virtual HRESULT WINAPI GetFolderType(FOLDERTYPEID *pftid);
    virtual HRESULT WINAPI GetIcon(LPWSTR *ppszIcon);
    virtual HRESULT WINAPI GetOptions(LIBRARYOPTIONFLAGS *plofOptions);
    virtual HRESULT WINAPI LoadLibraryFromItem(IShellItem *psiLibrary, DWORD grfMode);
    virtual HRESULT WINAPI LoadLibraryFromKnownFolder(REFKNOWNFOLDERID kfidLibrary, DWORD grfMode);
    virtual HRESULT WINAPI RemoveFolder(IShellItem *psiLocation);
    virtual HRESULT WINAPI ResolveFolder(IShellItem *psiFolderToResolve, DWORD dwTimeout, REFIID riid, void **ppv);
    virtual HRESULT WINAPI Save(IShellItem *psiFolderToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo);
    virtual HRESULT WINAPI SaveInKnownFolder(REFKNOWNFOLDERID kfidToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo);
    virtual HRESULT WINAPI SetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, IShellItem *psi);
    virtual HRESULT WINAPI SetFolderType(REFFOLDERTYPEID ftid);
    virtual HRESULT WINAPI SetIcon(LPCWSTR pszIcon);
    virtual HRESULT WINAPI SetOptions(LIBRARYOPTIONFLAGS lofMask, LIBRARYOPTIONFLAGS lofOptions);


DECLARE_REGISTRY_RESOURCEID(IDR_SHELLLIBRARY)
DECLARE_NOT_AGGREGATABLE(CShellLibrary)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellLibrary)
    COM_INTERFACE_ENTRY_IID(IID_IShellLibrary, IShellLibrary)
END_COM_MAP()
};

#endif // _CSHELLLIBRARY_H_
