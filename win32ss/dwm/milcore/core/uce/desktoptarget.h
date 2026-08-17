//-----------------------------------------------------------------------------
//
//  Description:
//      uDWM's desktop render target -- the composition's only output.
//
//      Lives here rather than in resources/VistaDwmResources.h with the other
//      Vista-only slave resources because it derives from CRenderTarget, and
//      the precomps for swlib, glyph and hw include resources.h without ever
//      including uce/uce.h. Vista keeps its cmd-73 slave resource beside the
//      other render targets too.
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  CMilDesktopRenderTargetDuce -- TYPE_DESKTOPRENDERTARGET (48)
//
//  uDWM's output target: the whole desktop, and the only render target in the
//  composition. CDesktopManager::EnableRenderTargetImpl creates one and
//  configures it with cmd 73, whose payload is 0x5C bytes.
//
//  IT IS A REAL RENDER TARGET, and that is the point of this class. Vista's
//  cmd-73 handler accepts resource type 0x2F (HWND) and 0x30 (DESKTOP) through
//  one path, builds the render target, and finishes with
//  CRenderTargetManager::AddRenderTarget for both -- only diverging to record
//  the desktop one in a dedicated slot. (Recovered by disassembling
//  0x7424FE76; that arm is a JUMPOUT in the decompile and appears nowhere in
//  milcore.dll.c.)
//
//  This used to be a plain CMilSlaveResource whose ProcessCreate set a bool.
//  Nothing that AddRenderTarget would accept was ever built, so
//  CRenderTargetManager::Render walked an empty list every frame -- the
//  compositor ran, dispatched every command with hr=0, and drew nothing.
//
//  WHY NOT JUST REUSE CSlaveHWndRenderTarget: because the PAYLOADS DIFFER.
//  Both commands are 92 bytes, but uDWM's desktop form is
//  MILCMD_TARGET_CREATE_VSP1 -- {Type, Handle, Data[21]} with a 16.16 scale at
//  Data[6..7], mode flags at Data[12] and a constant 1 at Data[13]
//  (uDWM.dll.c:8425-8437) -- and shares no field offsets with WPF's
//  MILCMD_HWNDTARGET_CREATE. Feeding one to the other's ProcessCreate reads
//  hwnd out of two zeroed dwords and takes the clear colour and flags from
//  unrelated bytes.
//
//-----------------------------------------------------------------------------

class CMilDesktopRenderTargetDuce : public CRenderTarget
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDesktopRenderTargetDuce));

    CMilDesktopRenderTargetDuce(__in_ecount(1) CComposition *pComposition);
    virtual ~CMilDesktopRenderTargetDuce();

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        /* TYPE_RENDERTARGET too, via the base -- cmd 77 (TargetSetRoot) and
         * the render-target manager both address it as one. */
        return type == TYPE_DESKTOPRENDERTARGET ||
               CRenderTarget::IsOfType(type);
    }

    /* ---- CRenderTarget ---- */

    /* override */ virtual HRESULT Render(__out_ecount(1) bool *pfNeedsPresent);
    /* override */ virtual HRESULT Present();

    /* override */ virtual HRESULT GetBaseRenderTargetInternal(
        __deref_out_opt IRenderTargetInternal **ppIRT
        );

    //
    // Cmd 73, the VSP1 form. Named apart from CRenderTarget's command handlers
    // because it does NOT take a MILCMD_HWNDTARGET_CREATE -- see the note above.
    //
    HRESULT ProcessCreate(__in_bcount(cbSize) const void *pcvData, UINT cbSize);

    bool IsCreated() const { return m_fCreated; }

private:
    HRESULT EnsureRenderTargetInternal();
    void    ReleaseResources();

    CComposition *m_pCompositionNoRef;

    IMILRenderTargetHWND *m_pRenderTarget;

    //
    // Data[12] of the create payload. uDWM sends 66842, plus bit 2 when
    // SyncToVBlank is off. Retained rather than interpreted: only the VBlank
    // bit has been recovered, and inventing meanings for the rest would be
    // worse than carrying the word.
    //
    UINT32 m_dwModeFlags;

    /* Data[6..7], a 16.16 fixed-point scale. Identity in every capture so far. */
    UINT32 m_dwScaleX;
    UINT32 m_dwScaleY;

    MilColorF m_clearColor;

    bool m_fCreated;
    bool m_fNeedsFullRender;
};
