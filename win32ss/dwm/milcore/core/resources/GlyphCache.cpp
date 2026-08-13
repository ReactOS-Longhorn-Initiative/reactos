//-----------------------------------------------------------------------------
//
//      TYPE_GLYPHCACHE slave resource. See GlyphCache.h.
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"
#include <debug.h>   // [RWM] DPRINT1

MtDefine(CMilGlyphCacheDuce, MILRender, "GlyphCache Resource");

//
// The wire format, 1:1 with uDWM's CGlyphCache::UpdateCache, which is itself
// 1:1 with Vista's uDWM.dll.c:18416:
//
//     MilChannel_BeginCommand(hChannel, &cmd, 0x10, cbAttach);
//       AppendCommandData(metrics, 0x14);
//       AppendCommandData(bits, cy * stride);
//       AppendCommandData(&u16zero, 2);
//     MilChannel_EndCommand(hChannel);
//
// so the record is a 16-byte header followed by 20 bytes of metrics, the
// bitmap, and a 2-byte zero tail.
//
#pragma pack(push, 1)

struct MILCMD_GLYPHCACHE_ADDBITMAPS_HDR
{
    UINT32 Type;        // 83
    UINT32 hCache;      // this resource
    UINT32 hFace;       // which face slot the bitmap belongs to
    UINT16 a;           // 1
    UINT16 b;           // 0
};

struct MILCMD_GLYPHCACHE_METRICS
{
    UINT32 Reserved;    // uDWM zeroes this; Vista leaves it uninitialised
    INT16  f0;
    INT16  iNegYOffset;
    INT16  f2;
    INT16  f3;
    INT16  f4;
    INT16  cx;
    INT16  cy;
    INT16  cbStride;
};

#pragma pack(pop)

C_ASSERT(sizeof(MILCMD_GLYPHCACHE_ADDBITMAPS_HDR) == 16);
C_ASSERT(sizeof(MILCMD_GLYPHCACHE_METRICS) == 20);

//+----------------------------------------------------------------------------
//
//  CMilGlyphCacheDuce::~CMilGlyphCacheDuce
//
//-----------------------------------------------------------------------------

CMilGlyphCacheDuce::~CMilGlyphCacheDuce()
{
    FaceNode *pNode = m_pFaces;

    while (pNode != NULL)
    {
        FaceNode *pNext = pNode->pNext;
        FreeFaceNode(pNode);
        pNode = pNext;
    }

    m_pFaces = NULL;
    m_cFaces = 0;
}

/* static */ void
CMilGlyphCacheDuce::FreeFaceNode(__inout_ecount(1) FaceNode *pNode)
{
    delete [] pNode->face.pbBits;
    delete pNode;
}

//+----------------------------------------------------------------------------
//
//  CMilGlyphCacheDuce::FindFace
//
//-----------------------------------------------------------------------------

__outro_ecount_opt(1) const CMilGlyphCacheDuce::GlyphFace *
CMilGlyphCacheDuce::FindFace(UINT32 uFaceHandle) const
{
    for (const FaceNode *pNode = m_pFaces; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->face.uFaceHandle == uFaceHandle)
        {
            return &pNode->face;
        }
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
//  CMilGlyphCacheDuce::ProcessAddBitmaps
//
//  Synopsis:
//      Takes one uploaded glyph bitmap and stores it against its face handle,
//      replacing any previous upload for that face.
//
//-----------------------------------------------------------------------------

HRESULT
CMilGlyphCacheDuce::ProcessAddBitmaps(
    __in_bcount(cbSize) const void *pcvData,
    UINT cbSize
    )
{
    HRESULT hr = S_OK;
    const BYTE *pbData = reinterpret_cast<const BYTE *>(pcvData);
    FaceNode *pNew = NULL;
    BYTE *pbBits = NULL;

    if (pcvData == NULL ||
        cbSize < sizeof(MILCMD_GLYPHCACHE_ADDBITMAPS_HDR) +
                 sizeof(MILCMD_GLYPHCACHE_METRICS))
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    {
        const MILCMD_GLYPHCACHE_ADDBITMAPS_HDR *pHdr =
            reinterpret_cast<const MILCMD_GLYPHCACHE_ADDBITMAPS_HDR *>(pbData);

        const MILCMD_GLYPHCACHE_METRICS *pMet =
            reinterpret_cast<const MILCMD_GLYPHCACHE_METRICS *>(
                pbData + sizeof(MILCMD_GLYPHCACHE_ADDBITMAPS_HDR));

        //
        // The metrics describe the bitmap; they are NOT derived from cbSize.
        //
        // cbSize is the batch record's size and carries alignment padding --
        // an observed 17-row upload at stride 168 is 16 + 20 + 2856 + 2 = 2894
        // bytes and arrives as cbSize 2896. Deriving the bitmap length from
        // cbSize would therefore read two bytes of padding as image data, and
        // requiring an exact match would reject every well-formed command.
        //
        if (pMet->cx < 0 || pMet->cy < 0 || pMet->cbStride < 0)
        {
            DPRINT1("[RWM] GlyphCache: negative metrics cx=%d cy=%d stride=%d\n",
                    (int)pMet->cx, (int)pMet->cy, (int)pMet->cbStride);
            IFC(WGXERR_MALFORMEDGLYPHCACHE);
        }

        //
        // A stride that cannot hold cx pixels at 1bpp means the sender and the
        // receiver disagree about the layout. Worth failing on rather than
        // storing: the field ORDER here is the exact thing that has been got
        // backwards before (see the note in uDWM's CGlyphCache::UpdateCache --
        // swapping cy and cbStride yields a correctly SIZED payload whose
        // description is wrong, which then faults deeper in).
        //
        if (pMet->cbStride < ((pMet->cx + 7) / 8))
        {
            DPRINT1("[RWM] GlyphCache: stride %d too small for %d px at 1bpp"
                    " (cy=%d) -- metrics field order?\n",
                    (int)pMet->cbStride, (int)pMet->cx, (int)pMet->cy);
            IFC(WGXERR_MALFORMEDGLYPHCACHE);
        }

        const UINT cbBits = static_cast<UINT>(pMet->cy) *
                            static_cast<UINT>(pMet->cbStride);

        //
        // >= and not ==, for the padding reason above. The trailing 2-byte
        // zero is not required to be present: it is the last thing appended,
        // so a short record would already have failed this test.
        //
        if (cbSize < sizeof(MILCMD_GLYPHCACHE_ADDBITMAPS_HDR) +
                     sizeof(MILCMD_GLYPHCACHE_METRICS) + cbBits)
        {
            DPRINT1("[RWM] GlyphCache: record %u too small for %u bytes of bits"
                    " (cx=%d cy=%d stride=%d)\n",
                    cbSize, cbBits,
                    (int)pMet->cx, (int)pMet->cy, (int)pMet->cbStride);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        if (cbBits != 0)
        {
            pbBits = new BYTE[cbBits];
            IFCOOM(pbBits);

            RtlCopyMemory(pbBits,
                          pbData + sizeof(MILCMD_GLYPHCACHE_ADDBITMAPS_HDR) +
                                   sizeof(MILCMD_GLYPHCACHE_METRICS),
                          cbBits);
        }

        //
        // Replace any previous upload for this face rather than accumulating.
        // uDWM re-uploads a face whenever the realization changes, so keeping
        // the old one would grow without bound.
        //
        FaceNode **ppLink = &m_pFaces;
        while (*ppLink != NULL)
        {
            if ((*ppLink)->face.uFaceHandle == pHdr->hFace)
            {
                FaceNode *pOld = *ppLink;
                *ppLink = pOld->pNext;
                FreeFaceNode(pOld);
                m_cFaces--;
                break;
            }
            ppLink = &(*ppLink)->pNext;
        }

        pNew = new FaceNode;
        IFCOOM(pNew);

        pNew->pNext                = m_pFaces;
        pNew->face.uFaceHandle     = pHdr->hFace;
        pNew->face.iNegYOffset     = pMet->iNegYOffset;
        pNew->face.cx              = pMet->cx;
        pNew->face.cy              = pMet->cy;
        pNew->face.cbStride        = pMet->cbStride;
        pNew->face.cbBits          = cbBits;
        pNew->face.pbBits          = pbBits;

        m_pFaces = pNew;
        m_cFaces++;

        // Owned by the node now.
        pNew   = NULL;
        pbBits = NULL;

        //
        // One line per face, not per upload: uDWM re-uploads on every
        // realization change, which is frame-rate on an animating caption.
        //
        {
            static LONG s_cLogged = 0;
            if (s_cLogged < 16)
            {
                InterlockedIncrement(&s_cLogged);
                DPRINT1("[RWM] GlyphCache: face 0x%lx %dx%d stride=%d bits=%u"
                        " negY=%d (faces=%u)\n",
                        (unsigned long)pHdr->hFace,
                        (int)pMet->cx, (int)pMet->cy, (int)pMet->cbStride,
                        cbBits, (int)pMet->iNegYOffset, m_cFaces);
            }
        }
    }

Cleanup:
    delete [] pbBits;
    delete pNew;

    RRETURN(hr);
}
