
# The ac97 driver's WaveRT (real-time audio) path is incomplete and does not build
# at NT6.0 (missing class declarations, IPortWaveRTStream methods called on IUnknown,
# etc. - a pre-existing driver gap, not an NT6.0 retarget issue). Exclude it on NT6.0+.
if(DLL_EXPORT_VERSION LESS 0x600)
    add_subdirectory(ac97)
endif()
add_subdirectory(CMIDriver)
