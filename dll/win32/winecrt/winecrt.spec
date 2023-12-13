# winecrt - Wine -> ReactOS crt vista+ Dll
@ stub -arch=i386 __libm_sse2_acos
@ stub -arch=i386 __libm_sse2_acosf
@ stub -arch=i386 __libm_sse2_asin
@ stub -arch=i386 __libm_sse2_asinf
@ stub -arch=i386 __libm_sse2_atan
@ stub -arch=i386 __libm_sse2_atan2
@ stub -arch=i386 __libm_sse2_atanf
@ stub -arch=i386 __libm_sse2_cos
@ stub -arch=i386 __libm_sse2_cosf
@ stub -arch=i386 __libm_sse2_exp
@ stub -arch=i386 __libm_sse2_expf
@ stub -arch=i386 __libm_sse2_log
@ stub -arch=i386 __libm_sse2_log10
@ stub -arch=i386 __libm_sse2_log10f
@ stub -arch=i386 __libm_sse2_logf
@ stub -arch=i386 __libm_sse2_pow
@ stub -arch=i386 __libm_sse2_powf
@ stub -arch=i386 __libm_sse2_sin
@ stub -arch=i386 __libm_sse2_sinf
@ stub -arch=i386 __libm_sse2_tan
@ stub -arch=i386 __libm_sse2_tanf

@ cdecl wctype(ptr)
@ cdecl _get_stream_buffer_pointers(ptr ptr ptr ptr)

@ cdecl _configure_narrow_argv(long)
@ cdecl _configure_wide_argv(long)
@ cdecl _get_initial_narrow_environment()
@ cdecl _get_initial_wide_environment()
@ cdecl _initialize_narrow_environment()
@ cdecl _initialize_wide_environment()

@ cdecl -stub  _register_thread_local_exe_atexit_callback(ptr)

@ cdecl -stub _invalid_parameter_noinfo()
@ cdecl -stub _invalid_parameter_noinfo_noreturn()
@ cdecl _set_fmode(long)
