//-----------------------------------------------------------------------------
//
//      The Vista-only DWM slave resources. See VistaDwmResources.h for what
//      these are, where the layouts came from, and what they deliberately do
//      not do.
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"
#include <debug.h>   // [RWM] DPRINT1

MtDefine(CMilWindowNodeDuce,          MILRender, "WindowNode Resource");
MtDefine(CMilDesktopRenderTargetDuce, MILRender, "DesktopRenderTarget Resource");
MtDefine(CMilMeshGeometry2DDuce,      MILRender, "MeshGeometry2D Resource");
MtDefine(CMilGeometry2DGroupDuce,     MILRender, "Geometry2DGroup Resource");
MtDefine(CMilScene3DDuce,             MILRender, "Scene3D Resource");
MtDefine(CMilCachedVisualImageDuce,   MILRender, "CachedVisualImage Resource");

//
// Every one of these commands begins {UINT32 Type; HMIL_RESOURCE Handle;}
// and the rest is DWORDs, which is how the senders build them (see
// dwmredir/DuceHelper.cpp). Reading positionally beats declaring nineteen
// structs that would each have to be kept in step with a sender in another
// binary.
//
#define VDWM_HEAD_DWORDS   2u
#define VDWM_DWORD(p, i)   (reinterpret_cast<const UINT32*>(p)[(i)])
#define VDWM_INT32(p, i)   (reinterpret_cast<const INT32*>(p)[(i)])

//
// `>=`, never `==` -- batch records carry alignment padding. See the header.
//
#define VDWM_NEED(cb, dwords)                                                 \
    do {                                                                      \
        if ((cb) < (dwords) * sizeof(UINT32))                                 \
        {                                                                     \
            DPRINT1("[RWM] %s: cmd %u short (%u bytes, need %u)\n",           \
                    pszWho, (unsigned)nCmdType, (unsigned)(cb),               \
                    (unsigned)((dwords) * sizeof(UINT32)));                   \
            return WGXERR_UCE_MALFORMEDPACKET;                                \
        }                                                                     \
    } while (0)

//
// One line the first time each command id is seen. The window-node stream
// runs at frame rate once a desktop is live, so logging every command would
// bury the log; logging each id once answers "what is actually arriving",
// which is the question during bring-up.
//
static bool VdwmFirstSeen(UINT32 nCmdType)
{
    static volatile LONG s_rgSeen[256] = { 0 };

    if (nCmdType >= 256)
        return false;

    return InterlockedCompareExchange(
               const_cast<LONG*>(&s_rgSeen[nCmdType]), 1, 0) == 0;
}

#define VDWM_TRACE(fmt, ...)                                                  \
    do {                                                                      \
        if (VdwmFirstSeen((UINT32)nCmdType))                                  \
        {                                                                     \
            DPRINT1("[RWM] %s cmd %u: " fmt, pszWho,                          \
                    (unsigned)nCmdType, __VA_ARGS__);                         \
        }                                                                     \
    } while (0)

/* ==========================================================================
 *  CMilWindowNodeDuce -- TYPE_WINDOWNODE (42)
 * ========================================================================== */

CMilWindowNodeDuce::CMilWindowNodeDuce(__in_ecount(1) CComposition *pComposition)
    : CMilVisual(pComposition)
{
    m_pCompositionNoRef = pComposition;

    m_fAttached         = false;
    m_fApplySpriteClip  = false;
    m_fComposeOnce      = false;

    RtlZeroMemory(&m_rcBounds, sizeof(m_rcBounds));
    RtlZeroMemory(&m_marAlpha, sizeof(m_marAlpha));
    RtlZeroMemory(&m_marMaximizedClip, sizeof(m_marMaximizedClip));

    m_hSprite       = 0;
    m_hSpriteImage  = 0;
    m_hSpriteClip   = 0;
    m_hDxImage      = 0;
    m_hDxClip       = 0;

    m_dwSourceFlags = 0;
    m_dwColor       = 0;
    m_dwDxAlpha     = 0;

    m_cDirty        = 0;
}

HRESULT
CMilWindowNodeDuce::ProcessCommand(
    MILCMD nCmdType,
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    static const char * const pszWho = "WindowNode";

    if (pcvData == NULL)
        return E_INVALIDARG;

    VDWM_NEED(cbSize, VDWM_HEAD_DWORDS);

    switch (nCmdType)
    {
    //
    // 54 / 55 -- attach and detach. dwmredir sends Create when the window
    // context gains a root node and Detach when the window goes away.
    //
    case MilCmdWindowNodeCreate:
        m_fAttached = true;
        VDWM_TRACE("create%s", "\n");
        break;

    case MilCmdWindowNodeDetach:
        m_fAttached = false;
        VDWM_TRACE("detach%s", "\n");
        break;

    //
    // 58 -- SetBounds. {Type, Handle, left, top, right, bottom}
    //
    case MilCmdWindowNodeSetBounds:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 4);
        m_rcBounds.left   = VDWM_INT32(pcvData, 2);
        m_rcBounds.top    = VDWM_INT32(pcvData, 3);
        m_rcBounds.right  = VDWM_INT32(pcvData, 4);
        m_rcBounds.bottom = VDWM_INT32(pcvData, 5);
        VDWM_TRACE("bounds %d,%d %dx%d\n",
                   (int)m_rcBounds.left, (int)m_rcBounds.top,
                   (int)(m_rcBounds.right - m_rcBounds.left),
                   (int)(m_rcBounds.bottom - m_rcBounds.top));
        break;

    //
    // 60 -- UpdateSpriteHandle. {Type, Handle, hSprite, padding}
    //
    // hSprite is win32k's sprite id, NOT a MIL resource handle -- it is the
    // same value that travelled the whole way from IntDwmCreateSprite. This
    // is the join between the win32k stream and the composition tree.
    //
    case MilCmdWindowNodeUpdateSpriteHandle:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_hSprite = VDWM_DWORD(pcvData, 2);
        VDWM_TRACE("sprite handle 0x%lx\n", (unsigned long)m_hSprite);
        break;

    //
    // 62 / 63 -- the sprite and DX content bitmaps.
    //
    case MilCmdWindowNodeSetSpriteImage:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_hSpriteImage = VDWM_DWORD(pcvData, 2);
        VDWM_TRACE("sprite image h=0x%lx\n", (unsigned long)m_hSpriteImage);
        break;

    case MilCmdWindowNodeSetDxImage:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_hDxImage = VDWM_DWORD(pcvData, 2);
        VDWM_TRACE("dx image h=0x%lx\n", (unsigned long)m_hDxImage);
        break;

    //
    // 64 / 65 -- clips. The sprite clip carries an explicit apply flag;
    // a null geometry with fApply set is "clip to nothing".
    //
    case MilCmdWindowNodeSetSpriteClip:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 2);
        m_hSpriteClip      = VDWM_DWORD(pcvData, 2);
        m_fApplySpriteClip = (VDWM_INT32(pcvData, 3) != 0);
        VDWM_TRACE("sprite clip h=0x%lx apply=%d\n",
                   (unsigned long)m_hSpriteClip, (int)m_fApplySpriteClip);
        break;

    case MilCmdWindowNodeSetDxClip:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_hDxClip = VDWM_DWORD(pcvData, 2);
        VDWM_TRACE("dx clip h=0x%lx\n", (unsigned long)m_hDxClip);
        break;

    //
    // 66 -- SetSourceModifications. dwmredir's WindowNode_UpdateProperties
    // sends {SourceFlags, Color, Extra} on cmd 63 in the cross-machine path;
    // this is the same triple on its own command.
    //
    case MilCmdWindowNodeSetSourceModifications:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 2);
        m_dwSourceFlags = VDWM_DWORD(pcvData, 2);
        m_dwColor       = VDWM_DWORD(pcvData, 3);
        VDWM_TRACE("source mods flags=0x%lx color=0x%08lx\n",
                   (unsigned long)m_dwSourceFlags, (unsigned long)m_dwColor);
        break;

    //
    // 67 / 70 -- margins. Both are a MARGINS after the head.
    //
    case MilCmdWindowNodeSetAlphaMargins:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 4);
        m_marAlpha.cxLeftWidth    = VDWM_INT32(pcvData, 2);
        m_marAlpha.cxRightWidth   = VDWM_INT32(pcvData, 3);
        m_marAlpha.cyTopHeight    = VDWM_INT32(pcvData, 4);
        m_marAlpha.cyBottomHeight = VDWM_INT32(pcvData, 5);
        VDWM_TRACE("alpha margins %d,%d,%d,%d\n",
                   (int)m_marAlpha.cxLeftWidth, (int)m_marAlpha.cxRightWidth,
                   (int)m_marAlpha.cyTopHeight, (int)m_marAlpha.cyBottomHeight);
        break;

    case MilCmdWindowNodeSetMaximizedClipMargins:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 4);
        m_marMaximizedClip.cxLeftWidth    = VDWM_INT32(pcvData, 2);
        m_marMaximizedClip.cxRightWidth   = VDWM_INT32(pcvData, 3);
        m_marMaximizedClip.cyTopHeight    = VDWM_INT32(pcvData, 4);
        m_marMaximizedClip.cyBottomHeight = VDWM_INT32(pcvData, 5);
        VDWM_TRACE("maximized clip margins%s", "\n");
        break;

    //
    // 68 / 72 -- compose-once latch and the DX alpha.
    //
    case MilCmdWindowNodeSetComposeOnce:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_fComposeOnce = (VDWM_INT32(pcvData, 2) != 0);
        VDWM_TRACE("compose once=%d\n", (int)m_fComposeOnce);
        break;

    case MilCmdWindowNodeSetDxAlpha:
        VDWM_NEED(cbSize, VDWM_HEAD_DWORDS + 1);
        m_dwDxAlpha = VDWM_DWORD(pcvData, 2);
        VDWM_TRACE("dx alpha 0x%lx\n", (unsigned long)m_dwDxAlpha);
        break;

    //
    // 57 / 59 -- dirty notifications. Counted rather than accumulated: the
    // dirty-region machinery belongs to the composition pass, which does not
    // walk these nodes yet, and inventing a region list here would be a
    // structure nothing reads.
    //
    case MilCmdWindowNodeNotifyDirty:
    case MilCmdWindowNodeAddDirtyRegion:
        m_cDirty++;
        VDWM_TRACE("dirty (first of many)%s", "\n");
        break;

    //
    // 56 / 61 / 69 / 71 -- accepted and recorded as seen. Each needs a
    // subsystem that is not ported (DX update flushing, the shared-surface
    // update path, compositor-owned resource copying, visible-region
    // tracking). Failing them would zombie the partition over something
    // Vista treats as routine.
    //
    case MilCmdWindowNodeFlushDxUpdates:
    case MilCmdWindowNodeNotifyDxUpdate:
    case MilCmdWindowNodeCopyCompositorOwnedResources:
    case MilCmdWindowNodeNotifyVisRgnUpdate:
        VDWM_TRACE("accepted, subsystem not ported (cb=%u)\n", (unsigned)cbSize);
        break;

    default:
        DPRINT1("[RWM] WindowNode: unexpected cmd %u\n", (unsigned)nCmdType);
        return WGXERR_UCE_MALFORMEDPACKET;
    }

    return S_OK;
}

/* ==========================================================================
 *  CMilDesktopRenderTargetDuce -- TYPE_DESKTOPRENDERTARGET (48)
 * ========================================================================== */

CMilDesktopRenderTargetDuce::CMilDesktopRenderTargetDuce(
    __in_ecount(1) CComposition *pComposition
    )
{
    m_pCompositionNoRef = pComposition;
    m_hRootVisual       = 0;
    m_fCreated          = false;
}

HRESULT
CMilDesktopRenderTargetDuce::ProcessCreate(
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    if (pcvData == NULL || cbSize < VDWM_HEAD_DWORDS * sizeof(UINT32))
        return WGXERR_UCE_MALFORMEDPACKET;

    m_fCreated = true;

    DPRINT1("[RWM] DesktopRenderTarget: created (cb=%u)\n", (unsigned)cbSize);
    return S_OK;
}

HRESULT
CMilDesktopRenderTargetDuce::ProcessSetRoot(HMIL_RESOURCE hRoot)
{
    m_hRootVisual = hRoot;
    DPRINT1("[RWM] DesktopRenderTarget: root visual h=0x%lx\n",
            (unsigned long)hRoot);
    return S_OK;
}

/* ==========================================================================
 *  CMilMeshGeometry2DDuce -- TYPE_MESHGEOMETRY2D (23)
 * ========================================================================== */

CMilMeshGeometry2DDuce::CMilMeshGeometry2DDuce(
    __in_ecount(1) CComposition *pComposition
    )
{
    m_pCompositionNoRef = pComposition;

    m_pbPositions          = NULL; m_cbPositions          = 0;
    m_pbTextureCoordinates = NULL; m_cbTextureCoordinates = 0;
    m_pbVertexOpacities    = NULL; m_cbVertexOpacities    = 0;
    m_pbTriangleIndices    = NULL; m_cbTriangleIndices    = 0;

    m_dwConstantOpacity    = 0;
}

CMilMeshGeometry2DDuce::~CMilMeshGeometry2DDuce()
{
    FreeBuffers();
}

void CMilMeshGeometry2DDuce::FreeBuffers()
{
    delete [] m_pbPositions;          m_pbPositions          = NULL; m_cbPositions          = 0;
    delete [] m_pbTextureCoordinates; m_pbTextureCoordinates = NULL; m_cbTextureCoordinates = 0;
    delete [] m_pbVertexOpacities;    m_pbVertexOpacities    = NULL; m_cbVertexOpacities    = 0;
    delete [] m_pbTriangleIndices;    m_pbTriangleIndices    = NULL; m_cbTriangleIndices    = 0;
}

//
// Cmd 148. uDWM CMeshImage::Validate (1:1 with uDWM.dll.c:21038):
//
//     { MILCMD Type; HMIL_RESOURCE Handle;
//       UINT32 PositionsSize, TextureCoordinatesSize,
//              VertexOpacitiesSize, TriangleIndicesSize; }
//     then those four buffers concatenated, in that order.
//
HRESULT
CMilMeshGeometry2DDuce::ProcessUpdate(
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    if (pcvData == NULL || cbSize < 6 * sizeof(UINT32))
        return WGXERR_UCE_MALFORMEDPACKET;

    const UINT cbPos  = VDWM_DWORD(pcvData, 2);
    const UINT cbTex  = VDWM_DWORD(pcvData, 3);
    const UINT cbOpa  = VDWM_DWORD(pcvData, 4);
    const UINT cbIdx  = VDWM_DWORD(pcvData, 5);
    const UINT cbHead = 6 * sizeof(UINT32);

    //
    // Overflow-safe: each addend is bounded by cbSize before it is added, so
    // a hostile or corrupt header cannot wrap the sum past the check.
    //
    if (cbPos > cbSize || cbTex > cbSize || cbOpa > cbSize || cbIdx > cbSize)
        return WGXERR_UCE_MALFORMEDPACKET;

    const UINT cbNeed = cbHead + cbPos + cbTex + cbOpa + cbIdx;
    if (cbNeed < cbHead || cbSize < cbNeed)
    {
        DPRINT1("[RWM] MeshGeometry2D: record %u < needed %u"
                " (pos=%u tex=%u opa=%u idx=%u)\n",
                (unsigned)cbSize, (unsigned)cbNeed,
                (unsigned)cbPos, (unsigned)cbTex,
                (unsigned)cbOpa, (unsigned)cbIdx);
        return WGXERR_UCE_MALFORMEDPACKET;
    }

    //
    // Allocate everything before publishing any of it, so a failure part way
    // through leaves the previous mesh intact rather than half-replaced.
    //
    BYTE *pbPos = NULL, *pbTex = NULL, *pbOpa = NULL, *pbIdx = NULL;
    HRESULT hr = S_OK;

    if (cbPos) { pbPos = new BYTE[cbPos]; IFCOOM(pbPos); }
    if (cbTex) { pbTex = new BYTE[cbTex]; IFCOOM(pbTex); }
    if (cbOpa) { pbOpa = new BYTE[cbOpa]; IFCOOM(pbOpa); }
    if (cbIdx) { pbIdx = new BYTE[cbIdx]; IFCOOM(pbIdx); }

    {
        const BYTE *pbSrc = reinterpret_cast<const BYTE*>(pcvData) + cbHead;

        if (cbPos) { RtlCopyMemory(pbPos, pbSrc, cbPos); pbSrc += cbPos; }
        if (cbTex) { RtlCopyMemory(pbTex, pbSrc, cbTex); pbSrc += cbTex; }
        if (cbOpa) { RtlCopyMemory(pbOpa, pbSrc, cbOpa); pbSrc += cbOpa; }
        if (cbIdx) { RtlCopyMemory(pbIdx, pbSrc, cbIdx); }
    }

    FreeBuffers();

    m_pbPositions          = pbPos; m_cbPositions          = cbPos;
    m_pbTextureCoordinates = pbTex; m_cbTextureCoordinates = cbTex;
    m_pbVertexOpacities    = pbOpa; m_cbVertexOpacities    = cbOpa;
    m_pbTriangleIndices    = pbIdx; m_cbTriangleIndices    = cbIdx;

    pbPos = pbTex = pbOpa = pbIdx = NULL;

    {
        static LONG s_cLogged = 0;
        if (InterlockedIncrement(&s_cLogged) <= 8)
        {
            DPRINT1("[RWM] MeshGeometry2D: %u verts, %u indices"
                    " (pos=%u tex=%u opa=%u idx=%u)\n",
                    VertexCount(), IndexCount(),
                    m_cbPositions, m_cbTextureCoordinates,
                    m_cbVertexOpacities, m_cbTriangleIndices);
        }
    }

Cleanup:
    delete [] pbPos;
    delete [] pbTex;
    delete [] pbOpa;
    delete [] pbIdx;

    RRETURN(hr);
}

//
// Cmd 103. {Type, Handle, Opacity} -- the packed vertex diffuse, sent alone
// when only the opacity moved. See CMeshImage::Validate for why this is the
// opacity and not the hidden-margin mask.
//
HRESULT
CMilMeshGeometry2DDuce::ProcessSetConstantOpacity(
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    if (pcvData == NULL || cbSize < 3 * sizeof(UINT32))
        return WGXERR_UCE_MALFORMEDPACKET;

    m_dwConstantOpacity = VDWM_DWORD(pcvData, 2);
    return S_OK;
}

/* ==========================================================================
 *  CMilGeometry2DGroupDuce -- TYPE_GEOMETRY2DGROUP (24)
 * ========================================================================== */

CMilGeometry2DGroupDuce::CMilGeometry2DGroupDuce(
    __in_ecount(1) CComposition *pComposition
    )
{
    m_pCompositionNoRef = pComposition;
}

CMilGeometry2DGroupDuce::~CMilGeometry2DGroupDuce()
{
    UnRegisterNotifiers();
}

void CMilGeometry2DGroupDuce::UnRegisterNotifiers()
{
    for (UINT i = 0; i < m_rgpChildren.GetCount(); i++)
    {
        CMilMeshGeometry2DDuce *pChild = m_rgpChildren[i];
        UnRegisterNotifier(pChild);
    }

    m_rgpChildren.SetCount(0);
}

//
// Cmd 149. uDWM CMesh2DVisual: a 0x0C header then 4 bytes per child handle.
// The count is not in the header -- it is whatever fits in the record, which
// is why this derives it rather than reading it.
//
// The handles are RESOLVED HERE. Storing them raw was useless to the draw
// pass: CMilSlaveRenderData::Draw carries rgpResources and no handle table, so
// there is nothing there to resolve a handle against. Command time is also the
// only moment the handle is guaranteed to still name this resource.
//
// RegisterNotifier is what gives the group its reference on each child and
// subscribes it to their changes, and UnRegisterNotifiers above is its
// counterpart -- called on replace and from the destructor.
//
HRESULT
CMilGeometry2DGroupDuce::ProcessSetChildren(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    HRESULT hr = S_OK;
    const UINT cbHead = 3 * sizeof(UINT32);

    if (pHandleTable == NULL || pcvData == NULL || cbSize < cbHead)
        return WGXERR_UCE_MALFORMEDPACKET;

    const UINT cChildren = (cbSize - cbHead) / sizeof(HMIL_RESOURCE);
    const HMIL_RESOURCE *ph =
        reinterpret_cast<const HMIL_RESOURCE*>(
            reinterpret_cast<const BYTE*>(pcvData) + cbHead);

    //
    // Drop the old set first. A group is re-sent wholesale rather than
    // edited, so accumulating would both leak references and leave stale
    // meshes in the draw order.
    //
    UnRegisterNotifiers();

    for (UINT i = 0; i < cChildren; i++)
    {
        CMilMeshGeometry2DDuce *pChild =
            static_cast<CMilMeshGeometry2DDuce*>(pHandleTable->GetResource(
                ph[i],
                TYPE_MESHGEOMETRY2D
                ));

        if (pChild == NULL)
        {
            DPRINT1("[RWM] Geometry2DGroup: child %u handle 0x%lx is not a"
                    " TYPE_MESHGEOMETRY2D\n", i, (unsigned long)ph[i]);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        IFC(RegisterNotifier(pChild));

        hr = m_rgpChildren.Add(pChild);
        if (FAILED(hr))
        {
            /* Not yet in the array, so the notifier it just took would be
             * lost to UnRegisterNotifiers. Drop it here. */
            UnRegisterNotifier(pChild);
            IFC(hr);
        }
    }

    {
        static LONG s_cLogged = 0;
        if (InterlockedIncrement(&s_cLogged) <= 8)
        {
            DPRINT1("[RWM] Geometry2DGroup: %u children resolved\n",
                    m_rgpChildren.GetCount());
        }
    }

Cleanup:
    if (FAILED(hr))
        UnRegisterNotifiers();

    RRETURN(hr);
}

/* ==========================================================================
 *  CMilScene3DDuce -- TYPE_SCENE3D (5)
 * ========================================================================== */

CMilScene3DDuce::CMilScene3DDuce(__in_ecount(1) CComposition *pComposition)
{
    m_pCompositionNoRef  = pComposition;
    m_pModelGroup        = NULL;
    m_pCamera            = NULL;
    m_pViewportAnimation = NULL;
    RtlZeroMemory(&m_viewport, sizeof(m_viewport));
}

CMilScene3DDuce::~CMilScene3DDuce()
{
    UnRegisterNotifiers();
}

void CMilScene3DDuce::UnRegisterNotifiers()
{
    UnRegisterNotifier(m_pViewportAnimation);
    UnRegisterNotifier(m_pCamera);
    UnRegisterNotifier(m_pModelGroup);

    m_pViewportAnimation = NULL;
    m_pCamera            = NULL;
    m_pModelGroup        = NULL;
}

//
// Cmd 137, a fixed 52-byte record. See MILCMD_SCENE3D for where each field
// came from.
//
// The previous decode read the camera and model handles out of dwords 2 and
// 3, i.e. offsets 8 and 12 -- which are the two halves of the viewport's X
// double. Both are 0.0, so it reported camera=0x0 model=0x0 on every scene
// and its "viewport" was Y plus half of Width. It also described cmd 137 as
// having two forms, a 0x34 one from CFlip3D and a 44-byte one from
// CEnvironmentMap; those are the same record counted with and without the
// {Type, Handle} head. Vista has one form and requires exactly 52 bytes.
//
HRESULT
CMilScene3DDuce::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    HRESULT hr = S_OK;

    CMilModel3DGroupDuce *pModelGroup = NULL;
    CMilCameraDuce       *pCamera     = NULL;
    CMilSlaveResource    *pAnimation  = NULL;

    if (pHandleTable == NULL || pcvData == NULL || cbSize != sizeof(MILCMD_SCENE3D))
        return WGXERR_UCE_MALFORMEDPACKET;

    const MILCMD_SCENE3D *pCmd =
        reinterpret_cast<const MILCMD_SCENE3D*>(pcvData);

    //
    // Resolve everything BEFORE dropping the old set, so a malformed record
    // leaves the scene as it was rather than emptied. Vista unregisters first
    // and bails to NotifyOnChanged on a bad handle; the difference only shows
    // on a packet that cannot arrive from a correct sender, and holding the
    // last good scene is the safer of the two.
    //
    if (pCmd->hModel3DGroup != HMIL_RESOURCE_NULL)
    {
        pModelGroup =
            static_cast<CMilModel3DGroupDuce*>(pHandleTable->GetResource(
                pCmd->hModel3DGroup,
                TYPE_MODEL3DGROUP
                ));

        if (pModelGroup == NULL)
        {
            DPRINT1("[RWM] Scene3D: model group handle 0x%lx is not a"
                    " TYPE_MODEL3DGROUP\n", (unsigned long)pCmd->hModel3DGroup);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    //
    // Looked up as the camera BASE type, exactly as Vista does. uDWM creates a
    // TYPE_MATRIXCAMERA and a lookup for TYPE_MATRIXCAMERA here would reject
    // every other camera kind Vista accepts.
    //
    if (pCmd->hCamera != HMIL_RESOURCE_NULL)
    {
        pCamera =
            static_cast<CMilCameraDuce*>(pHandleTable->GetResource(
                pCmd->hCamera,
                TYPE_CAMERA
                ));

        if (pCamera == NULL)
        {
            DPRINT1("[RWM] Scene3D: camera handle 0x%lx is not a camera\n",
                    (unsigned long)pCmd->hCamera);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (pCmd->hViewportAnimation != HMIL_RESOURCE_NULL)
    {
        pAnimation =
            static_cast<CMilSlaveResource*>(pHandleTable->GetResource(
                pCmd->hViewportAnimation,
                TYPE_RECTRESOURCE
                ));

        if (pAnimation == NULL)
        {
            DPRINT1("[RWM] Scene3D: viewport animation handle 0x%lx is not a"
                    " TYPE_RECTRESOURCE\n",
                    (unsigned long)pCmd->hViewportAnimation);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    UnRegisterNotifiers();

    IFC(RegisterNotifier(pModelGroup));
    m_pModelGroup = pModelGroup;

    IFC(RegisterNotifier(pCamera));
    m_pCamera = pCamera;

    IFC(RegisterNotifier(pAnimation));
    m_pViewportAnimation = pAnimation;

    m_viewport = pCmd->Viewport;

    {
        static LONG s_cLogged = 0;
        if (InterlockedIncrement(&s_cLogged) <= 8)
        {
            DPRINT1("[RWM] Scene3D: model=%p camera=%p viewport=(%d,%d %dx%d)\n",
                    m_pModelGroup, m_pCamera,
                    (int)m_viewport.X,     (int)m_viewport.Y,
                    (int)m_viewport.Width, (int)m_viewport.Height);
        }
    }

Cleanup:
    if (FAILED(hr))
        UnRegisterNotifiers();

    NotifyOnChanged(this);

    RRETURN(hr);
}

/* ==========================================================================
 *  CMilCachedVisualImageDuce -- TYPE_CACHEDVISUALIMAGE (65)
 * ========================================================================== */

CMilCachedVisualImageDuce::CMilCachedVisualImageDuce(
    __in_ecount(1) CComposition *pComposition
    )
{
    m_pCompositionNoRef = pComposition;
    m_hSourceVisual     = 0;
    RtlZeroMemory(m_rgdwPayload, sizeof(m_rgdwPayload));
    m_cbPayload         = 0;
}

//
// Cmd 161, 0x48 bytes. uDWM's CSecondaryWindowRepresentation sends the
// source visual and a source rect; the full field breakdown is not recovered,
// so the payload is retained whole and only the source visual is named.
//
HRESULT
CMilCachedVisualImageDuce::ProcessUpdate(
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    if (pcvData == NULL || cbSize < 3 * sizeof(UINT32))
        return WGXERR_UCE_MALFORMEDPACKET;

    m_hSourceVisual = VDWM_DWORD(pcvData, 2);

    m_cbPayload = min(cbSize, (UINT)sizeof(m_rgdwPayload));
    RtlCopyMemory(m_rgdwPayload, pcvData, m_cbPayload);

    DPRINT1("[RWM] CachedVisualImage: source visual h=0x%lx cb=%u\n",
            (unsigned long)m_hSourceVisual, (unsigned)cbSize);
    return S_OK;
}
