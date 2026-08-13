// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

class CMilGeometryDuce;
class CMilPenDuce;
class CMilBrushDuce;
class CGlyphRunResource;
class CMilGeometry2DDuce;
class CMilImageSource;
class CMilTransformDuce;
class CMilGuidelineSetDuce;
class CMilSlaveVideo;
class CMilDrawingDuce;
class CMilSlaveRenderData;
class CMilVisual;
/* [RWM] Vista-only resources, see core/resources/VistaDwmResources.h. */
class CMilGeometry2DGroupDuce;
class CMilScene3DDuce;


//---------------------------------------------------------------------------------
// IDrawingContext
//---------------------------------------------------------------------------------

interface IDrawingContext
{
    //
    // Drawing interface.
    //

    virtual HRESULT DrawLine(
        const MilPoint2D &point0,
        const MilPoint2D &point1,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilSlavePoint *point0Animations,
        __in_ecount_opt(1) CMilSlavePoint *point1Animations
        ) = 0;

    virtual HRESULT DrawRectangle(
        const MilPointAndSizeD &rect,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlaveRect *pRectAnimations
        ) = 0;

    virtual HRESULT DrawRoundedRectangle(
        const MilPointAndSizeD &rect,
        const double &radiusX,
        const double &radiusY,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlaveRect *pRectangleAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
        ) = 0;

    virtual HRESULT DrawEllipse(
        const MilPoint2D &center,
        const double &radiusX,
        const double &radiusY,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlavePoint *pCenterAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
        ) = 0;

    virtual HRESULT DrawGeometry(
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilGeometryDuce *pGeometry
        ) = 0;

    virtual HRESULT DrawImage(
        __in_ecount(1) CMilSlaveResource *pImageResource,
        __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
        __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
        ) = 0;

    virtual HRESULT DrawVideo(
        __in_ecount(1) CMilSlaveVideo *pMediaClock,
        __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
        __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
        ) = 0;

    virtual HRESULT DrawGlyphRun(
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CGlyphRunResource *pGlyphRun
        ) = 0;
    
    virtual HRESULT DrawDrawing(
        __in_ecount_opt(1) CMilDrawingDuce *pDrawing
        ) = 0;

    //
    // [RWM] Vista's drawing primitives.
    //
    // Not pure: CDrawingContext is the only implementer today, but these are
    // being ported one at a time and a default that a partly-ported context
    // inherits is better than one that will not compile. The defaults are in
    // drawingcontext.cpp and every one of them is LOUD -- a silent default
    // here is indistinguishable from a working renderer, which is the exact
    // trap these commands have already sprung once (accepted, hr=S_OK,
    // renders nothing).
    //
    virtual HRESULT DrawMesh2D(
        __in_ecount_opt(1) CMilGeometry2DGroupDuce *pMeshGroup,
        __in_ecount_opt(1) CMilSlaveResource *pImage
        );

    virtual HRESULT DrawGlass(
        __in_ecount_opt(1) CMilSlaveResource *pTop,
        __in_ecount_opt(1) CMilSlaveResource *pLeft,
        __in_ecount_opt(1) CMilSlaveResource *pRight,
        __in_ecount_opt(1) CMilSlaveResource *pBottom,
        __in_ecount_opt(1) CMilSlaveResource *pColorization
        );

    virtual HRESULT DrawOcclusionRectangle(
        __in_ecount(1) const MilPointAndSizeD *prc
        );

    virtual HRESULT DrawScene3D(
        __in_ecount_opt(1) CMilScene3DDuce *pScene,
        UINT32 dwFlags
        );

    //
    // State stack.
    //
    virtual HRESULT PushClip(
        __in_ecount_opt(1) CMilGeometryDuce *pClipGeometry
        ) = 0;

    virtual HRESULT Pop() = 0;

    virtual HRESULT PushOpacity(
        const double &opacity,
        __in_ecount_opt(1) CMilSlaveDouble *pOpacityAnimation
        ) = 0;

    virtual HRESULT PushOpacityMask(
        __in_ecount_opt(1) CMilBrushDuce *pOpacityMask,
        __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
        ) = 0;

    virtual HRESULT PushTransform(
        __in_ecount_opt(1) CMilTransformDuce *pTransform
        ) = 0;

    virtual HRESULT PushGuidelineCollection(
        __in_ecount_opt(1) CGuidelineCollection *pGuidelineCollection,
        __out_ecount(1) bool & fNeedMoreCycles
        ) = 0;

    virtual HRESULT PushGuidelineCollection(
        __in_ecount_opt(1) CMilGuidelineSetDuce *pGuidelines
        ) = 0;

    //
    // Utility function for bounds render pass check.
    //

    virtual BOOL IsBounding()
    {
        return false;
    };

    //
    // This function is an implementation detail on the surface contexts. It is
    // used to lazily apply the clip realizations so that multiple chained
    // PushClip calls aren't a performance bottleneck.
    //

    virtual void ApplyRenderState() = 0;

};





