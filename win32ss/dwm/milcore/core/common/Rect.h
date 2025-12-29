// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Basic rectangle types with notion of coordinate space and compile
//      and/or debug runtime compatibility checking.
//
//-----------------------------------------------------------------------------

#pragma once


//+----------------------------------------------------------------------------
//
//  Class:
//      TRect_<typename BaseRectType, CoordinateSpace>
//
//  Synopsis:
//      Rectangle class templated by a base rectangle type and a coordinate
//      space.  The class prevents unique coordinate spaces from erroneously
//      being copied from one space to another by generating compile time
//      errors.  To convert from one space to another a CMatrix of the proper
//      type should be used.
//
//  Notes:
//      Normally type CRectF<CoordinateSpace> should be used instead of TRect_
//
//-----------------------------------------------------------------------------

template <typename TBaseMILRect, typename Space>
class TRect_ : public TBaseMILRect
{

    //+------------------------------------------------------------------------
    //
    //  Type:
    //      BaseMILRectType
    //
    //  Synopsis:
    //      Convenience reference type to this classes base class for any
    //      derived classes
    //
    //-------------------------------------------------------------------------


public:
 typedef TBaseMILRect BaseMILRectType;

    //=========================================================================
    // Constructors
    //
    //  Note: These definitions are needed because inherited constructors are
    //        not directly visible to users of this type.  Conveniently this
    //        allows the "undefinition" of constructors that are not desired.

    //+------------------------------------------------------------------------
    //
    //  Member:    TRect_
    //
    //  Synopsis:  Default ctor leaving members uninitialized
    //
    //-------------------------------------------------------------------------

    TRect_()
    {
        // We require that you can typecast between BaseMILRectType and TRect_.
        // To achieve this, TRect_ must have no data members or virtual functions.

        // This is a compile time assert so we only need it once here, but no
        // where else.
        static_assert( sizeof(TBaseMILRect) == sizeof(TRect_) , " sizeof(BaseMILRectType) == sizeof(TRect_) ");
    }


    //+------------------------------------------------------------------------
    //
    //  Member:    Other TRect_ ctors
    //
    //  Synopsis:  Delegate to base class ctors
    //
    //-------------------------------------------------------------------------

    TRect_(
        typename TBaseMILRect::BaseUnitType _left,
        typename TBaseMILRect::BaseUnitType _top,
        typename TBaseMILRect::BaseUnitType _right,
        typename TBaseMILRect::BaseUnitType _bottom,
        LTRB ltrb
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TBaseMILRect(_left, _top, _right, _bottom, ltrb)
#endif // !_PREFIX_
    {}


    TRect_(
        typename TBaseMILRect::BaseUnitType x,
        typename TBaseMILRect::BaseUnitType y,
        typename TBaseMILRect::BaseUnitType width,
        typename TBaseMILRect::BaseUnitType height,
        XYWH xywh
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TBaseMILRect(x, y, width, height, xywh)
#endif // !_PREFIX_
    {}


    //
    // !!! No automatic conversion from generic rectangle of the base type !!!
    //
#if NEVER
    TRect_(__in_ecount(1) const typename TBaseMILRect::BaseRectType &rc)
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : BaseMILRectType(rc)
#endif // !_PREFIX_
    {}
#endif /* NEVER */


    template<typename TPoint>
    TRect_(
        TPoint pt1,
        TPoint pt2
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : BaseMILRectType(pt1, pt2)
#endif // !_PREFIX_
    {}


    //=========================================================================
    // Casting Helper Routines
    //

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretBaseType
    //
    //  Synopsis:
    //      Helpers to reinterpret from base classes of the same coordinate
    //      space.  These are useful when more simple though still space
    //      specific rectangles are passed around from, but then a routine
    //      wants to access the additional member routines provided by a
    //      derived wrapper class - like CMilRectF wraps MilRectF.
    //
    //-------------------------------------------------------------------------

    static TRect_ * ReinterpretBaseType(__in_ecount_opt(1) TRect_<typename BaseMILRectType::BaseRectType, Space> *base)
    {
        return reinterpret_cast<TRect_ *>(base);
    }

    static const TRect_ * ReinterpretBaseType(__in_ecount_opt(1) const TRect_<typename BaseMILRectType::BaseRectType, Space> *base)
    {
        return reinterpret_cast<const TRect_ *>(base);
    }

};


//+----------------------------------------------------------------------------
//
//  Class:
//      CRectF<CoordinateSpace>
//
//  Synopsis:
//      Floating point base rectangle class templated by coordinate space.  The
//      class prevents unique coordinate spaces from erroneously being copied
//      from one space to another by generating compile time errors.  To
//      convert from one space to another a CMatrix of the proper type should
//      be used.
//
//-----------------------------------------------------------------------------

template <typename Space>
class CRectF : public TRect_<CMilRectF, Space>
{
public:

    //=========================================================================
    // Constructors
    //
    //  Note: These definitions are needed because inherited constructors are
    //        not directly visible to users of this type.  Conveniently this
    //        allows the "undefinition" of constructors that are not desired.
    //        Specifically automatic conversion from generic rects is not
    //        desired and not redefined here.

    //+------------------------------------------------------------------------
    //
    //  Member:    CRectF
    //
    //  Synopsis:  Default ctor leaving members uninitialized
    //
    //-------------------------------------------------------------------------

    CRectF()
    {
        // We require that you can typecast between MilRectF and CRectF.
        // To achieve this, CRectF must have no data members or virtual functions.

        // This is a compile time assert so we only need it once here, but no where else.
        static_assert( sizeof(MilRectF) == sizeof(CRectF) , " sizeof(MilRectF) == sizeof(CRectF) ");
    }


    //+------------------------------------------------------------------------
    //
    //  Member:    Other CRectF ctors
    //
    //  Synopsis:  Delegate to base class ctors
    //
    //-------------------------------------------------------------------------

    CRectF(
        typename CMilRectF::BaseUnitType _left,
        typename CMilRectF::BaseUnitType _top,
        typename CMilRectF::BaseUnitType _right,
        typename CMilRectF::BaseUnitType _bottom,
        LTRB ltrb
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TRect_<CMilRectF, Space>(_left, _top, _right, _bottom, ltrb)
#endif // !_PREFIX_
    {}


    CRectF(
        typename CMilRectF::BaseUnitType x,
        typename CMilRectF::BaseUnitType y,
        typename CMilRectF::BaseUnitType width,
        typename CMilRectF::BaseUnitType height,
        XYWH xywh
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TRect_<CMilRectF, Space>(x, y, width, height, xywh)
#endif // !_PREFIX_
    {}

    //
    // !!! No automatic conversion from generic rectangle of the base type !!!
    //
#if NEVER
    CRectF(__in_ecount(1) const typename BaseMILRectType::BaseRectType &rc)
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TRect_<CMilRectF, Space>(rc)
#endif // !_PREFIX_
    {}
#endif /* NEVER */

    template<typename TPoint>
    CRectF(
        TPoint pt1,
        TPoint pt2
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TRect_<CMilRectF, Space>(pt1, pt2)
#endif // !_PREFIX_
    {}


    //=========================================================================
    // Casting Helper Routines
    //

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretAsVariantOut
    //
    //  Synopsis:
    //      Reinterpret this rectangle as having Variant coordinate space.  Use
    //      should be limited.
    //
    //-------------------------------------------------------------------------

    __returnro CRectF<CoordinateSpace::Variant> const &ReinterpretAsVariant(
        ) const
    {
        static_assert(Space::Id != CoordinateSpaceId::Invalid, "Space::Id != CoordinateSpaceId::Invalid");
        static_assert(sizeof(*this) == sizeof(CRectF<CoordinateSpace::Variant>), "sizeof(*this) == sizeof(CRectF<CoordinateSpace::Variant>)");
        return reinterpret_cast<CRectF<CoordinateSpace::Variant> const &>(*this);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretBaseType
    //
    //  Synopsis:
    //      Helpers to reinterpret from base classes of the same coordinate
    //      space.  These are useful when more simple though still space
    //      specific rectangles are passed around, but then a routine
    //      wants to access the additional member routines provided by a
    //      derived wrapper class - like CMilRectF wraps MilRectF.
    //
    //-------------------------------------------------------------------------

    static CRectF * ReinterpretBaseType(__in_ecount_opt(1) TRect_<typename CMilRectF::BaseRectType, Space> *base)
    {
        return reinterpret_cast<CRectF *>(base);
    }

    static const CRectF * ReinterpretBaseType(__in_ecount_opt(1) const TRect_<typename CMilRectF::BaseRectType, Space> *base)
    {
        return reinterpret_cast<const CRectF *>(base);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretNonSpaceTyped
    //
    //  Synopsis:
    //      Helper to reinterpret non-space-specific rectangle as a rectangle
    //      in this coordinate space.
    //
    //  Notes:
    //      These members should not be used to reinterpret one coordinate
    //      space transform to another.  See Reinterpret*Space1*As*Space2*
    //      methods below for such reinterpretation needs.
    //
    //-------------------------------------------------------------------------

    static const CRectF * ReinterpretNonSpaceTyped(
        __in_ecount_opt(1) const MilRectF *prc
        )
    {
        static_assert(sizeof(*prc) == sizeof(CRectF), "sizeof(*prc) == sizeof(CRectF)");
        return static_cast<const CRectF *>(prc);
    }

    static const CRectF & ReinterpretNonSpaceTyped(
        __in_ecount_opt(1) const MilRectF &rc
        )
    {
        static_assert(sizeof(rc) == sizeof(CRectF), "sizeof(rc) == sizeof(CRectF)");
        return static_cast<const CRectF &>(rc);
    }
};



//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretPageInPixelsAsDevice
//      ReinterpretRealizationSamplingAsLocalRendering
//
//  Synopsis:
//      Helper methods to reinterpret one coordinate space as another
//      coordinate space.  Use of helpers are preferred over direct
//      reinterpret_cast, becacuse reinterpret_cast is dangerous and at least
//      here sizes can be asserted.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CRectF<CoordinateSpace::Device> &
ReinterpretPageInPixelsAsDevice(
    __in_ecount(1) const CRectF<CoordinateSpace::PageInPixels> &rc
    )
{
    static_assert(sizeof(rc) == sizeof(CRectF<CoordinateSpace::Device>), "sizeof(rc) == sizeof(CRectF<CoordinateSpace::Device>)");
    return reinterpret_cast<const CRectF<CoordinateSpace::Device> &>(rc);
}

MIL_FORCEINLINE __returnro const CRectF<CoordinateSpace::LocalRendering> &
ReinterpretRealizationSamplingAsLocalRendering(
    __in_ecount(1) const CRectF<CoordinateSpace::RealizationSampling> &rc
    )
{
    static_assert(sizeof(rc) == sizeof(CRectF<CoordinateSpace::LocalRendering>), "sizeof(rc) == sizeof(CRectF<CoordinateSpace::LocalRendering>)");
    return reinterpret_cast<const CRectF<CoordinateSpace::LocalRendering> &>(rc);
}


//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretNonSpaceTypeDUCERectAsLocalRenderingRect
//
//  Synopsis:
//      Helper methods to reinterpret note coordinate space specific DUCE
//      rectangle type as LocalRendering bounds.  Use of helpers are preferred
//      over direct reinterpret_cast, becacuse reinterpret_cast is dangerous
//      and at least here sizes can be asserted.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __ecount(1) CRectF<CoordinateSpace::LocalRendering> *
ReinterpretNonSpaceTypeDUCERectAsLocalRenderingRect(
    __ecount(1) MilRectF *prc
    )
{
    static_assert(sizeof(*prc) == sizeof(CRectF<CoordinateSpace::LocalRendering>), "sizeof(*prc) == sizeof(CRectF<CoordinateSpace::LocalRendering>)");
    static_assert(offsetof(MilRectF, left) == offsetof(CRectF<CoordinateSpace::LocalRendering>, left), "offsetof(MilRectF, left) == offsetof(CRectF<CoordinateSpace::LocalRendering>, left)");
    static_assert(offsetof(MilRectF, top) == offsetof(CRectF<CoordinateSpace::LocalRendering>, top), "offsetof(MilRectF, top) == offsetof(CRectF<CoordinateSpace::LocalRendering>, top)");
    static_assert(offsetof(MilRectF, right) == offsetof(CRectF<CoordinateSpace::LocalRendering>, right), "offsetof(MilRectF, right) == offsetof(CRectF<CoordinateSpace::LocalRendering>, right)");
    static_assert(offsetof(MilRectF, bottom) == offsetof(CRectF<CoordinateSpace::LocalRendering>, bottom), "offsetof(MilRectF, bottom) == offsetof(CRectF<CoordinateSpace::LocalRendering>, bottom)");
    return reinterpret_cast<CRectF<CoordinateSpace::LocalRendering> *>(prc);
}



template <typename CoordSpace1, typename CoordSpace2>
class CMultiSpaceRectF;

// define CMultiSpaceRectF<BaseSampling, Device>
#define CoordSpace1 BaseSampling
#define CoordSpace2 Device
#include "MultiSpaceRectF.inl"

// define CMultiSpaceRectF<PageInPixels, Device>
#undef CoordSpace1
#undef CoordSpace2
#define CoordSpace1 PageInPixels
#define CoordSpace2 Device
#include "MultiSpaceRectF.inl"



