
#include <stdint.h>
#include <intrin.h>
#include <malloc.h>
#define _USE_MATH_DEFINES
#include <math.h>

// atexit is needed by libsupc++
extern int __cdecl _crt_atexit(void (__cdecl*)(void));
int __cdecl atexit(void (__cdecl* function)(void))
{
    return _crt_atexit(function);
}

void* __cdecl operator_new(size_t size)
{
    return malloc(size);
}

void _cdecl operator_delete(void *mem)
{
    free(mem);
}

#ifdef _M_IX86
void _chkesp_failed(void)
{
    __debugbreak();
}
#endif

int __cdecl __acrt_initialize_sse2(void)
{
    return 0;
}


double round(double arg)
{
    if (arg < 0.0)
        return ceil(arg - 0.5);
    else
        return floor(arg + 0.5);
}



float roundf(float arg)
{
    if (arg < 0.0)
        return ceilf(arg - 0.5);
    else
        return floorf(arg + 0.5);
}


float __cdecl truncf(_In_ float _X)
{
    if (_X < 0.0)
        return ceilf(_X - 0.5);
    else
        return floorf(_X + 0.5);
}

double __cdecl trunc(_In_ double _X)
{
    if (_X < 0.0)
        return ceil(_X - 0.5);
    else
        return floor(_X + 0.5);
}

long      __cdecl lround(_In_ double _X)
{
 return trunc(_X);
}
long lroundf(
   float x
)
{
  return truncf(x);
}
long lroundl(
   long double x
)
{
 return lround(x);
}

// The following stubs cannot be implemented as stubs by spec2def, because they are intrinsics

#ifdef _MSC_VER
#pragma warning(disable:4163) // not available as an intrinsic function
#pragma warning(disable:4164) // intrinsic function not declared
#pragma function(fma)
#pragma function(fmaf)
#pragma function(log2)
#pragma function(log2f)
#pragma function(lrint)
#pragma function(lrintf)
#endif

double fma(double x, double y, double z)
{
    // Simplistic implementation
    return (x * y) + z;
}

float fmaf(float x, float y, float z)
{
    // Simplistic implementation
    return (x * y) + z;
}

double log2(double x)
{
    // Simplistic implementation: log2(x) = log(x) / log(2)
    return log(x) * M_LOG2E;
}

float log2f(float x)
{
    return (float)log2((double)x);
}

long int lrint(double x)
{
    __debugbreak();
    return 0;
}

long int lrintf(float x)
{
    __debugbreak();
    return 0;
}


int __cdecl _fdsign(float x)
{
    if (x > 0.0f)
        return 1;
    else if (x < 0.0f)
        return -1;
    else
        return 0;
}

float nexttowardf(float x, long double y)
{
    if (isnan(x) || isnan(y))
        return x + (float)y;
    if (x == (float)y)
        return (float)y;
    if (x == 0.0f) {
        union { float f; uint32_t u; } u = { 0.0f };
        u.u = 1;
        return (y > 0.0) ? u.f : -u.f;
    }
    union { float f; uint32_t u; } u = { x };
    if ((x > (float)y) == (x > 0.0f))
        u.u--;
    else
        u.u++;
    return u.f;
}

double nexttoward(double x, long double y)
{
    if (isnan(x) || isnan(y))
        return x + y;
    if (x == (double)y)
        return y;
    if (x == 0.0) {
        union { double d; uint64_t u; } u = { 0.0 };
        u.u = 1;
        return (y > 0.0) ? u.d : -u.d;
    }
    union { double d; uint64_t u; } u = { x };
    if ((x > (double)y) == (x > 0.0))
        u.u--;
    else
        u.u++;
    return u.d;
}

int __cdecl _dsign(double x)
{
    if (x > 0.0)
        return 1;
    else if (x < 0.0)
        return -1;
    else
        return 0;
}