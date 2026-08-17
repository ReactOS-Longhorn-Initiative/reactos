//-----------------------------------------------------------------------------
//
//  Description:
//      The Vista-only DWM slave resources.
//
//      These six types exist in Vista SP1's milcore and not in WPF, so the
//      generator that produced generated_resource_factory.cpp and
//      generated_process_message.inl never emitted them. uDWM and dwmredir
//      address them constantly; without them the first thing uDWM creates
//      fails the factory.
//
//          TYPE_WINDOWNODE            42   cmds 54-72   dwmredir, per window
//          TYPE_DESKTOPRENDERTARGET   48   cmd  73      uDWM, the output target
//          TYPE_MESHGEOMETRY2D        23   cmds 148,103 uDWM CMeshImage (chrome)
//          TYPE_GEOMETRY2DGROUP       24   cmd  149     uDWM CMesh2DVisual
//          TYPE_SCENE3D                5   cmd  137     uDWM CEnvironmentMap
//          TYPE_CACHEDVISUALIMAGE     65   cmd  161     uDWM CSecondaryWindowRepresentation
//
//      GROUPED IN ONE FILE, against the one-class-per-file convention next
//      door, because "what is Vista-only" is the single most useful boundary
//      to be able to see at a glance here -- these are the resources whose
//      layouts came out of the decompiles rather than out of WPF, and they
//      are the ones to re-check against the reference when something on the
//      wire disagrees.
//
//      WHAT THESE DO AND DO NOT DO. They parse, validate and RETAIN their
//      state faithfully. Nothing here renders: the composition pass does not
//      yet walk window nodes, and the Vista drawing instructions
//      (MilDrawGlass, MilDrawMesh2D, MilDrawBitmap, MilDrawVisual,
//      MilDrawOcclusionRectangle, MilDrawScene3D) have no handler in
//      renderdata_generated.cpp either. Holding the state is the honest half
//      that can be written from the command formats alone; consuming it needs
//      the renderer, which is a separate and much larger port.
//
//      Every layout below is taken from the SENDER in this workspace --
//      dwmredir/DuceHelper.cpp and uDWM's CMeshImage / CDesktopManager /
//      CEnvironmentMap -- each of which is itself 1:1 with a Vista body and
//      carries the decompile line number. See
//      DarkFiresReactOSModules/dwm/docs/NOTES-milcore-ids.md.
//
//      SIZE CHECKS ARE `>=`, NEVER `==`. A batch record carries alignment
//      padding: the glyph upload is 2894 bytes of command and arrives as
//      cbSize 2896. An exact-match check rejects well-formed commands.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  The Vista render-data INSTRUCTIONS.
//
//  These are not channel commands -- they live inside a TYPE_RENDERDATA blob
//  and are walked by CMilSlaveRenderData::Draw, so they are dispatched by
//  renderdata.cpp / renderdata_generated.cpp rather than by
//  generated_process_message.inl. A scan of the command switch alone reports
//  them as missing and is wrong about where they belong.
//
//  FRAMING. CMilDataStreamReader::GetNextItemSafe hands out
//
//      ppItemData  = <start of record> + sizeof(UINT)   // skips the SIZE only
//      pcbItemSize = cbRecord - sizeof(UINT)
//
//  so the pointer lands on the TYPE field and every MILCMD_DRAW_* struct
//  starts with it. uDWM writes `cbInstruction = sizeof(instr)` where instr
//  includes cbInstruction itself, so a record of cb bytes arrives as cb-4.
//
//  That is the arithmetic behind uDWM's own "24, not 20" note on
//  DrawGeometry: MILCMD_DRAW_GEOMETRY is 20 bytes and the record is 24.
//  Getting it wrong truncates every record and milcore then resynchronises
//  on garbage -- which is how an execute of 0x400000004000 was produced once
//  already.
//
//  pack(4) is uDWM's, and it matters for the one struct containing doubles:
//  natural alignment would make MILCMD_DRAW_OCCLUSIONRECTANGLE 40 bytes
//  against the 36 that arrive.
//
//-----------------------------------------------------------------------------

#pragma pack(push, 4)

// Instruction 106. Emitted by uDWM's CMesh2DVisual, one per (image, group).
struct MILCMD_DRAW_MESH2D
{
    MILCMD        type;
    HMIL_RESOURCE hMeshGroup;      // TYPE_GEOMETRY2DGROUP
    HMIL_RESOURCE hImage;          // TYPE_BITMAPSOURCE
};

// Instruction 105. Defined by uDWM's CRenderData but not yet emitted by
// anything, so the four edge handles' resource types are unconfirmed.
struct MILCMD_DRAW_GLASS
{
    MILCMD        type;
    HMIL_RESOURCE hTop;
    HMIL_RESOURCE hLeft;
    HMIL_RESOURCE hRight;
    HMIL_RESOURCE hBottom;
    float         reserved[4];
    HMIL_RESOURCE hColorization;
};

// Instruction 107. A culling hint: the region is opaque, so what is behind
// it need not be drawn.
struct MILCMD_DRAW_OCCLUSIONRECTANGLE
{
    MILCMD type;
    double X;
    double Y;
    double Width;
    double Height;
};

// Instruction 124. uDWM calls it DrawViewport3D; the resource is TYPE_SCENE3D.
struct MILCMD_DRAW_SCENE3D
{
    MILCMD        type;
    HMIL_RESOURCE hViewport;       // TYPE_SCENE3D
    UINT32        Flags;
};

#pragma pack(pop)

//
// Sizes are the record size minus the 4-byte length header, matching what
// uDWM emits. If one of these fires, the sender and the receiver disagree
// about the layout and every following instruction in the blob is misread.
//
C_ASSERT(sizeof(MILCMD_DRAW_MESH2D)               == 16 - 4);
C_ASSERT(sizeof(MILCMD_DRAW_GLASS)                == 44 - 4);
C_ASSERT(sizeof(MILCMD_DRAW_OCCLUSIONRECTANGLE)   == 40 - 4);
C_ASSERT(sizeof(MILCMD_DRAW_SCENE3D)              == 16 - 4);

MtExtern(CMilWindowNodeDuce);
MtExtern(CMilDesktopRenderTargetDuce);
MtExtern(CMilMeshGeometry2DDuce);
MtExtern(CMilGeometry2DGroupDuce);
MtExtern(CMilScene3DDuce);
MtExtern(CMilCachedVisualImageDuce);

class CMilCameraDuce;
class CMilModel3DGroupDuce;

//
// Cmd 137, MilCmdScene3D. Recovered field-for-field from Vista's
// CMilScene3DDuce::ProcessUpdate (milcore.dll.c:70532), which copies the
// whole 52-byte record into a local int[13] and then reads:
//
//    v13[2..9]  -> qmemcpy(this + 24, .., 0x20)   the viewport rect
//    v13[10]    -> GetResource(.., 12)            TYPE_MODEL3DGROUP
//    v13[11]    -> GetResource(.., 6)             the camera, as its BASE
//                                                 type -- uDWM creates a
//                                                 MatrixCamera (10), which
//                                                 satisfies IsOfType(6)
//    v13[12]    -> GetResource(.., 56)            TYPE_RECTRESOURCE, optional
//
// The trailing rect resource is the animation slot for Viewport: the base
// value arrives inline as four doubles and a RectResource handle may override
// it. uDWM sends 0 there, so nothing exercises it today.
//
// This matches uDWM's sender (CEnvironmentMap::EmitViewport) exactly, which is
// itself decoded from a stock wire capture -- the doubles are recognisable on
// the wire as 0x405f0000_00000000 = 124.0 and 0x40420000_00000000 = 36.0.
//
#pragma pack(push, 1)
struct MILCMD_SCENE3D
{
    MILCMD           Type;              // +0
    HMIL_RESOURCE    Handle;            // +4
    MilPointAndSizeD Viewport;          // +8   X, Y, Width, Height as doubles
    HMIL_RESOURCE    hModel3DGroup;     // +40
    HMIL_RESOURCE    hCamera;           // +44
    HMIL_RESOURCE    hViewportAnimation; // +48  TYPE_RECTRESOURCE, may be 0
};                                      // = 52 (0x34)
#pragma pack(pop)

C_ASSERT(sizeof(MILCMD_SCENE3D) == 0x34);

//+----------------------------------------------------------------------------
//
//  CMilWindowNodeDuce -- TYPE_WINDOWNODE (42)
//
//  One redirected top-level window, as the compositor sees it. dwmredir's
//  CMilWindowContext creates one per window and drives it with commands
//  54-72; this is where win32k's sprite stream finally lands.
//
//-----------------------------------------------------------------------------

//
// A WINDOW NODE IS A VISUAL. Vista is explicit about it -- CWindowNode::IsOfType
// (milcore.dll.c:21504) is one line:
//
//     return a1 == 42 || a1 == 39;      // TYPE_WINDOWNODE || TYPE_VISUAL
//
// and that dual identity is load bearing, not a convenience. uDWM parents its
// chrome visuals under the window node and drives the tree with the ordinary
// visual commands -- MilCmdVisualInsertChildAt (43) looks the PARENT up as
// TYPE_VISUAL. A window node that answers only TYPE_WINDOWNODE fails that
// lookup and the whole window tree fails to assemble.
//
// The base class has to change with the type claim, not just the type claim.
// Answering TYPE_VISUAL while deriving from CMilSlaveResource would satisfy
// GetResource and then hand ProcessInsertChildAt a static_cast'd pointer to an
// object that is not a CMilVisual -- a call through a wrong vtable, which is
// far worse than the assert it replaces.
//
class CMilWindowNodeDuce : public CMilVisual
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilWindowNodeDuce));

    CMilWindowNodeDuce(__in_ecount(1) CComposition *pComposition);
    ~CMilWindowNodeDuce() { }

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        /* Vista's exact test. CMilVisual::IsOfType would answer TYPE_VISUAL
         * on its own, but spelling both out keeps this readable next to the
         * decompile and independent of the base's implementation. */
        return type == TYPE_WINDOWNODE || type == TYPE_VISUAL;
    }

    //
    // One entry for all of 54-72. The commands share a {Type, Handle} head
    // and differ only in what follows, so a single switch keeps the dispatch
    // arm in generated_process_message.inl to one grouped case list instead
    // of nineteen near-identical copies.
    //
    HRESULT ProcessCommand(
        MILCMD nCmdType,
        __in_bcount(cbSize) const void *pcvData,
        UINT cbSize
        );

    // ---- retained state, for the composition pass that does not exist yet ----
    bool          IsAttached() const     { return m_fAttached; }
    const RECT   &Bounds() const         { return m_rcBounds; }
    UINT32        SpriteHandle() const   { return m_hSprite; }
    HMIL_RESOURCE SpriteImage() const    { return m_hSpriteImage; }
    HMIL_RESOURCE SpriteClip() const     { return m_hSpriteClip; }
    bool          ApplySpriteClip() const{ return m_fApplySpriteClip; }
    const MARGINS &AlphaMargins() const  { return m_marAlpha; }
    UINT32        SourceFlags() const    { return m_dwSourceFlags; }
    UINT32        Color() const          { return m_dwColor; }
    UINT32        DxAlpha() const        { return m_dwDxAlpha; }
    bool          ComposeOnce() const    { return m_fComposeOnce; }
    UINT          DirtyCount() const     { return m_cDirty; }

private:
    CComposition *m_pCompositionNoRef;

    bool          m_fAttached;
    bool          m_fApplySpriteClip;
    bool          m_fComposeOnce;

    RECT          m_rcBounds;
    MARGINS       m_marAlpha;
    MARGINS       m_marMaximizedClip;

    UINT32        m_hSprite;          // HSPRITE, win32k's id -- not a MIL handle
    HMIL_RESOURCE m_hSpriteImage;
    HMIL_RESOURCE m_hSpriteClip;
    HMIL_RESOURCE m_hDxImage;
    HMIL_RESOURCE m_hDxClip;

    UINT32        m_dwSourceFlags;
    UINT32        m_dwColor;
    UINT32        m_dwDxAlpha;

    UINT          m_cDirty;           // NotifyDirty count, for tracing
};

//
// CMilDesktopRenderTargetDuce (TYPE_DESKTOPRENDERTARGET, 48) is NOT here.
//
// It is a CRenderTarget, and CRenderTarget lives under uce/, which several
// precomps that include this header (swlib, glyph, hw) never pull in. It sits
// in uce/desktoptarget.h beside the other render targets instead -- which is
// also where Vista keeps its cmd-73 slave resource.
//


//+----------------------------------------------------------------------------
//
//  CMilMeshGeometry2DDuce -- TYPE_MESHGEOMETRY2D (23)
//
//  uDWM's CMeshImage uploads chrome geometry here with cmd 148: a header of
//  four byte-counts followed by four concatenated buffers (positions,
//  texture coordinates, per-vertex opacities, triangle indices). Cmd 103
//  updates the packed vertex diffuse on its own when only opacity moved.
//
//-----------------------------------------------------------------------------

class CMilMeshGeometry2DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMeshGeometry2DDuce));

    CMilMeshGeometry2DDuce(__in_ecount(1) CComposition *pComposition);
    ~CMilMeshGeometry2DDuce();

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MESHGEOMETRY2D;
    }

    HRESULT ProcessUpdate(__in_bcount(cbSize) const void *pcvData, UINT cbSize);
    HRESULT ProcessSetConstantOpacity(__in_bcount(cbSize) const void *pcvData, UINT cbSize);

    UINT   VertexCount() const   { return m_cbPositions / sizeof(float) / 2; }
    UINT   IndexCount() const    { return m_cbTriangleIndices / sizeof(UINT16); }
    UINT32 ConstantOpacity() const { return m_dwConstantOpacity; }

private:
    void FreeBuffers();

    CComposition *m_pCompositionNoRef;

    BYTE  *m_pbPositions;          UINT m_cbPositions;
    BYTE  *m_pbTextureCoordinates; UINT m_cbTextureCoordinates;
    BYTE  *m_pbVertexOpacities;    UINT m_cbVertexOpacities;
    BYTE  *m_pbTriangleIndices;    UINT m_cbTriangleIndices;

    //
    // Cmd 103's payload. NOT the hidden-margin mask -- uDWM's CMeshImage
    // carries those in a different field, and sending the mask here is a bug
    // that has already been made once (see CMeshImage::Validate).
    //
    UINT32 m_dwConstantOpacity;
};

//+----------------------------------------------------------------------------
//
//  CMilGeometry2DGroupDuce -- TYPE_GEOMETRY2DGROUP (24)
//
//  An ordered set of TYPE_MESHGEOMETRY2D children. Cmd 149 is a 12-byte
//  header followed by the child handles.
//
//-----------------------------------------------------------------------------

class CMilGeometry2DGroupDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometry2DGroupDuce));

    CMilGeometry2DGroupDuce(__in_ecount(1) CComposition *pComposition);
    ~CMilGeometry2DGroupDuce();

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRY2DGROUP;
    }

    HRESULT ProcessSetChildren(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_bcount(cbSize) const void *pcvData,
        UINT cbSize
        );

    /* override */ void UnRegisterNotifiers();

    //
    // The draw pass needs the MESHES, not their handles -- this used to store
    // raw HMIL_RESOURCEs, which a render walk has no way to resolve (it holds
    // rgpResources, not the handle table). They are resolved once here, at
    // command time, which is also where the handle is guaranteed to still mean
    // something.
    //
    UINT ChildCount() const { return m_rgpChildren.GetCount(); }

    __outro_ecount_opt(1) CMilMeshGeometry2DDuce *Child(UINT i) const
    {
        return (i < m_rgpChildren.GetCount()) ? m_rgpChildren[i] : NULL;
    }

private:
    CComposition *m_pCompositionNoRef;

    /* RegisterNotifier'd, so the group holds a reference and hears about
     * changes; UnRegisterNotifiers drops them. */
    DynArray<CMilMeshGeometry2DDuce*> m_rgpChildren;
};

//+----------------------------------------------------------------------------
//
//  CMilScene3DDuce -- TYPE_SCENE3D (5)
//
//  uDWM's CEnvironmentMap and CFlip3D build a 3D scene on one of these and
//  configure it with cmd 137. uDWM calls it a Viewport3DVisual; Vista's
//  resource-type name is TYPE_SCENE3D. Same slot, two names -- worth knowing
//  when grepping.
//
//-----------------------------------------------------------------------------

class CMilScene3DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilScene3DDuce));

    CMilScene3DDuce(__in_ecount(1) CComposition *pComposition);
    ~CMilScene3DDuce();

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SCENE3D;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_bcount(cbSize) const void *pcvData,
        UINT cbSize
        );

    // ---- resolved scene, for the 3D draw pass ----
    CMilModel3DGroupDuce *ModelGroup() const { return m_pModelGroup; }
    CMilCameraDuce       *Camera()     const { return m_pCamera; }
    const MilPointAndSizeD &Viewport() const { return m_viewport; }

private:
    void UnRegisterNotifiers();

    CComposition *m_pCompositionNoRef;

    //
    // Resolved at command time, not stored as handles. The draw pass gets no
    // handle table (same reason spelled out on CMilGeometry2DGroupDuce), and
    // command time is the only moment a handle is guaranteed to still name
    // this resource.
    //
    CMilModel3DGroupDuce *m_pModelGroup;
    CMilCameraDuce       *m_pCamera;
    CMilSlaveResource    *m_pViewportAnimation;   // TYPE_RECTRESOURCE, usually NULL

    MilPointAndSizeD      m_viewport;
};

//+----------------------------------------------------------------------------
//
//  CMilCachedVisualImageDuce -- TYPE_CACHEDVISUALIMAGE (65)
//
//  A cached rasterization of a visual subtree, used by uDWM's
//  CSecondaryWindowRepresentation (thumbnails / live previews). Cmd 161 is
//  0x48 bytes and carries the source visual plus the source rect.
//
//-----------------------------------------------------------------------------

class CMilCachedVisualImageDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilCachedVisualImageDuce));

    CMilCachedVisualImageDuce(__in_ecount(1) CComposition *pComposition);
    ~CMilCachedVisualImageDuce() { }

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_CACHEDVISUALIMAGE;
    }

    HRESULT ProcessUpdate(__in_bcount(cbSize) const void *pcvData, UINT cbSize);

private:
    CComposition *m_pCompositionNoRef;

    HMIL_RESOURCE m_hSourceVisual;
    UINT32        m_rgdwPayload[16];   // retained verbatim; layout part-decoded
    UINT          m_cbPayload;
};


/*
 * ---------------------------------------------------------------------------
 * CMilGdiSpriteBitmap -- a window's redirection bitmap, as seen by the
 * compositor.
 * ---------------------------------------------------------------------------
 *
 * This is the receiving half of the surface handoff. win32k allocates a
 * section-backed bitmap per redirected window and hands dwmredir a
 * DWM_SURFACE_DATA describing it; dwmredir creates one of these and sends the
 * section across. Mapping that section is what lets the compositor read live
 * window content -- everything else in the chrome pipeline draws art we
 * generate ourselves.
 *
 * Derives from CMilSlaveBitmap because Vista's does: CMilGdiSpriteBitmap::
 * IsOfType (milcore.dll.c:17782) answers `a1 == 100 || CMilSlaveBitmap::
 * IsOfType(a1)`. That inheritance is what makes a sprite bitmap usable
 * anywhere an image source is, with no special-casing in the draw path.
 *
 * FIELD MAP, recovered from the decompile (dword indices off `this`):
 *
 *      +4/+5   visible width / height      +14..17  margins L,R,T,B
 *      +6      stride                      +18      hSprite
 *      +7      byte offset to first pixel  +19      cmd 90 trailing dword
 *      +8      pixel format                +20      large-surface counted flag
 *      +9      section handle              +21      composition
 *      +10     mapped base
 *      +11     IWGXBitmap
 */
MtExtern(CMilGdiSpriteBitmap);

class CMilGdiSpriteBitmap : public CMilSlaveBitmap
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilGdiSpriteBitmap));

    CMilGdiSpriteBitmap(__in_ecount(1) CComposition *pComposition);
    virtual ~CMilGdiSpriteBitmap();

public:

    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GDISPRITEBITMAP || CMilSlaveBitmap::IsOfType(type);
    }

    /* cmd 90 -- bind to the sprite. Vista stores and returns S_OK. */
    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_GDISPRITEBITMAP *pCmd
        );

    /* cmd 91 -- the non-client crop. */
    HRESULT ProcessUpdateMargins(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_GDISPRITEBITMAP_UPDATEMARGINS *pCmd
        );

    /* cmd 17 -- the local (non-Terminal-Services) section handoff. */
    HRESULT ProcessSection(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_BITMAP_SECTION *pCmd
        );

    /* cmd 93 -- drop the mapping. */
    HRESULT ProcessUnmapSection(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_GDISPRITEBITMAP_UNMAPSECTION *pCmd
        );

    UINT32 GetSpriteHandle() const { return m_hSprite; }

private:

    HRESULT HandleSectionChange(HANDLE hSection, MilPixelFormat::Enum fmt);
    HRESULT RecreateBitmap();
    void    UnmapAndDispose();

    CComposition *m_pCompositionNoRef;

    UINT   m_nVisibleWidth;
    UINT   m_nVisibleHeight;
    UINT   m_nStride;
    UINT   m_cbFirstPixelOffset;
    MilPixelFormat::Enum m_fmt;

    HANDLE m_hSection;
    void  *m_pvMappedBase;

    UINT   m_nFullWidth;
    UINT   m_nFullHeight;

    INT    m_cxLeft;
    INT    m_cxRight;
    INT    m_cyTop;
    INT    m_cyBottom;

    UINT32 m_hSprite;
    UINT32 m_dwReserved0;
};
