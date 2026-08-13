//-----------------------------------------------------------------------------
//
//  Description:
//      TYPE_GLYPHCACHE slave resource -- the server side of uDWM's caption
//      glyph cache.
//
//      Vista's uDWM rasterises caption text itself and uploads the resulting
//      1bpp bitmaps here, one per "face" handle, with MilCmdGlyphCacheAddBitmaps
//      (command 83). This is the resource those uploads address; see
//      DarkFiresReactOSModules/dwm/docs/NOTES-milcore-ids.md for how
//      TYPE_GLYPHCACHE = 38 was recovered, and uDWM's CGlyphCache::UpdateCache
//      (1:1 of uDWM.dll.c:18416) for the sending side.
//
//      NOT A WPF GLYPH CACHE. CMilSlaveGlyphCache in core/uce is WPF's
//      realization manager, driven by DWrite; it has nothing to do with this
//      and does not receive these commands.
//
//-----------------------------------------------------------------------------

MtExtern(CMilGlyphCacheDuce);

class CMilGlyphCacheDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

public:
    //
    // One uploaded face. The fields are the ones uDWM actually sends; the
    // metrics block has three more INT16s that Vista leaves at zero and
    // nothing has been observed reading, so they are not stored.
    //
    struct GlyphFace
    {
        UINT32  uFaceHandle;

        // Baseline adjustment. uDWM sends the NEGATED y offset (met.yNegOffset
        // = -holder.GetYOffset()), and it is kept in the form it arrived in so
        // this does not silently disagree with the sender about the sign.
        INT16   iNegYOffset;

        INT16   cx;         // width in pixels
        INT16   cy;         // height in pixels (rows)
        INT16   cbStride;   // bytes per row

        UINT    cbBits;     // cy * cbStride
        BYTE   *pbBits;     // owned; 1bpp, cbBits bytes
    };

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGlyphCacheDuce));

    CMilGlyphCacheDuce(__in_ecount(1) CComposition *pComposition)
    {
        m_pCompositionNoRef = pComposition;
        m_pFaces = NULL;
        m_cFaces = 0;
    }

    ~CMilGlyphCacheDuce();

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GLYPHCACHE;
    }

    //
    // Handles MilCmdGlyphCacheAddBitmaps. pcvData points at the whole command
    // record -- the 16-byte header AND the attachment -- and cbSize is its
    // size as the batch recorded it.
    //
    HRESULT ProcessAddBitmaps(
        __in_bcount(cbSize) const void *pcvData,
        UINT cbSize
        );

    // Lookup for the draw path, once there is one. NULL if never uploaded.
    __outro_ecount_opt(1) const GlyphFace *FindFace(UINT32 uFaceHandle) const;

private:
    struct FaceNode
    {
        FaceNode  *pNext;
        GlyphFace  face;
    };

    static void FreeFaceNode(__inout_ecount(1) FaceNode *pNode);

    CComposition *m_pCompositionNoRef;

    //
    // A list, not an array indexed by handle: uDWM's face handles run to
    // sc_uFaceHandleLimit (0x100000), so a flat table is out of the question,
    // and the number of live faces is small -- one per distinct caption font
    // realization.
    //
    FaceNode     *m_pFaces;
    UINT          m_cFaces;
};
