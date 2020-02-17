#ifndef TG_MATH_DEFINES
#define TG_MATH_DEFINES

#define TG_MATH_USE_FLOAT
#define TG_MATH_USE_ROW_MAJOR

#ifdef TG_MATH_USE_FLOAT
typedef float tg_math_float_t;
#define TG_MATH_FLOAT(x) x ## f
#elif defined(TG_MATH_USE_DOUBLE)
typedef double tg_math_float_t;
#define TG_MATH_FLOAT(x) x
#endif

#define TG_MAT4F_SIZE 16
#define TG_MAT4F_ROWS 4
#define TG_MAT4F_COLUMNS 4

#define TG_MAT3F_SIZE 9
#define TG_MAT3F_ROWS 3
#define TG_MAT3F_COLUMNS 3

#endif
