#ifndef TG_VARIADIC_H
#define TG_VARIADIC_H



#define _ADDRESSOF(v) (&(v))



#if (defined _M_ARM || defined _M_HYBRID_X86_ARM64) && !defined _M_CEE_PURE

#define _VA_ALIGN                   4
#define _SLOTSIZEOF(t)              ((sizeof(t) + _VA_ALIGN - 1) & ~(_VA_ALIGN - 1))
#define _APALIGN(t,ap)              (((va_list)0 - (ap)) & (__alignof(t) - 1))

#elif (defined _M_ARM64 || defined _M_ARM64EC) && !defined _M_CEE_PURE

#define _VA_ALIGN                   8
#define _SLOTSIZEOF(t)              ((sizeof(t) + _VA_ALIGN - 1) & ~(_VA_ALIGN - 1))
#define _APALIGN(t,ap)              (((va_list)0 - (ap)) & (__alignof(t) - 1))

#else

#define _SLOTSIZEOF(t)              (sizeof(t))
#define _APALIGN(t,ap)              (__alignof(t))

#endif



#if defined _M_CEE_PURE || (defined _M_CEE && !defined _M_ARM && !defined _M_ARM64)

void  __cdecl __va_start(char**, ...);
void* __cdecl __va_arg(char**, ...);
void  __cdecl __va_end(char**);
#define tg_variadic_start(ap, v)    ((void)(__va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), __alignof(v), _ADDRESSOF(v))))
#define tg_variadic_arg(ap, t)      (*(t *)__va_arg(&ap, _SLOTSIZEOF(t), _APALIGN(t,ap), (t*)0))
#define tg_variadic_end(ap)         ((void)(__va_end(&ap)))

#elif defined _M_IX86 && !defined _M_HYBRID_X86_ARM64

#define _INTSIZEOF(n)                    ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#define tg_variadic_start(ap, v)    ((void)(ap = (char*)_ADDRESSOF(v) + _INTSIZEOF(v)))
#define tg_variadic_arg(ap, t)      (*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#elif defined _M_ARM

#define tg_variadic_start(ap, v)    ((void)(ap = (char*)_ADDRESSOF(v) + _SLOTSIZEOF(v)))
#define tg_variadic_arg(ap, t)      (*(t*)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap)) - _SLOTSIZEOF(t)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#elif defined _M_HYBRID_X86_ARM64

void __cdecl __va_start(char**, ...);
#define tg_variadic_start(ap,v)     ((void)(__va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), __alignof(v), _ADDRESSOF(v))))
#define tg_variadic_arg(ap, t)      (*(t*)((ap += _SLOTSIZEOF(t)) - _SLOTSIZEOF(t)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#elif defined(_M_ARM64)

void __cdecl __va_start(char**, ...);
#define tg_variadic_start(ap,v)     ((void)(__va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), __alignof(v), _ADDRESSOF(v))))
#define tg_variadic_arg(ap, t)      ((sizeof(t) > (2 * sizeof(__int64))) ? **(t**)((ap += sizeof(__int64)) - sizeof(__int64)) : *(t*)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap)) - _SLOTSIZEOF(t)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#elif defined _M_ARM64EC

void __cdecl __va_start(char**, ...);
#define tg_variadic_start(ap,v)     ((void)(__va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), __alignof(v), _ADDRESSOF(v))))
#define tg_variadic_arg(ap, t)      ((sizeof(t) > sizeof(__int64) || (sizeof(t) & (sizeof(t) - 1)) != 0) ? **(t**)((ap += sizeof(__int64)) - sizeof(__int64)) : *(t*)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap)) - _SLOTSIZEOF(t)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#elif defined _M_X64

void __cdecl __va_start(char**, ...);
#define tg_variadic_start(ap, x)    ((void)(__va_start(&ap, x)))
#define tg_variadic_arg(ap, t)      ((sizeof(t) > sizeof(__int64) || (sizeof(t) & (sizeof(t) - 1)) != 0) ? **(t**)((ap += sizeof(__int64)) - sizeof(__int64)) :  *(t* )((ap += sizeof(__int64)) - sizeof(__int64)))
#define tg_variadic_end(ap)         ((void)(ap = (char*)0))

#endif



#endif
