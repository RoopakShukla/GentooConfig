
/*
    Part of the Austral project, under the Apache License v2.0 with LLVM Exceptions.
    See LICENSE file for details.

    SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/*
 * Austral types
 */

typedef uint8_t   au_unit_t;
typedef uint8_t   au_bool_t;
typedef uint8_t   au_nat8_t;
typedef int8_t    au_int8_t;
typedef uint16_t  au_nat16_t;
typedef int16_t   au_int16_t;
typedef uint32_t  au_nat32_t;
typedef int32_t   au_int32_t;
typedef uint64_t  au_nat64_t;
typedef int64_t   au_int64_t;
typedef size_t    au_index_t;
typedef void*     au_fnptr_t;
typedef uint8_t   au_region_t;

#define nil   0
#define false 0
#define true  1

/*
 * A little hack
 */

#define AU_STORE(ptr, val) (*(ptr) = (val), nil)

/*
 * Pervasive
 */

typedef struct {
  void* data;
  size_t size;
} au_span_t;

au_span_t au_make_span(void* data, size_t size) {
  return (au_span_t){ .data = data, .size = size };
}

au_span_t au_make_span_from_string(const char* data, size_t size) {
  return (au_span_t){ .data = (void*) data, .size = size };
}

void* au_stdout() {
#if defined(__APPLE__)
  extern void* __stdoutp;
  return __stdoutp;
#else
  extern void* stdout;
  return stdout;
#endif
}

void* au_stderr() {
#if defined(__APPLE__)
  extern void* __stderrp;
  return __stderrp;
#elif defined(__FreeBSD__)
  extern void* __stderrp;
  return __stderrp;
#elif defined(__OpenBSD__)
  extern void *__sF;
  return &__sF[2];
#else
  extern void* stderr;
  return stderr;
#endif
}

extern void* au_stdin() {
#if defined(__APPLE__)
  extern void* __stdinp;
  return __stdinp;
#else
  extern void* stdin;
  return stdin;
#endif
}

au_unit_t au_abort_internal(const char* message) {
  extern int fprintf(void* stream, const char* format, ...);
  extern int fflush(void* stream);
  extern void _Exit(int status);

  void* stderr = au_stderr();

  fprintf(stderr, "%s\n", message);
  fflush(stderr);
  _Exit(-1);

  return nil;
}

au_unit_t au_abort(au_span_t message) {
  extern int fprintf(void* stream, const char* format, ...);
  extern int fflush(void* stream);
  extern void _Exit(int status);

  void* stderr = au_stderr();

  fprintf(stderr, "%s\n", (char*) message.data);
  fflush(stderr);
  _Exit(-1);
  return nil;
}

au_unit_t au_printf(const char* format, ...) {
  extern int vprintf(const char* format, va_list arg);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  return nil;
}

void* au_array_index(au_span_t* array, size_t index, size_t elem_size) {
  if (index >= array->size) {
    au_abort_internal("Array index out of bounds.");
  }

  au_index_t offset = 0;
  if (__builtin_mul_overflow(index, elem_size, &offset)) {
    au_abort_internal("Multiplication overflow in array indexing operation.");
  }

  char* data = (char*) array->data;
  char* ptr = data + offset;
  return (void*)(ptr);
}

/*
 * Memory functions
 */

void* au_calloc(size_t size, size_t count) {
  extern void* calloc(size_t count, size_t size);

  return calloc(size, count);
}

void* au_realloc(void* ptr, size_t count) {
  extern void* realloc(void *ptr, size_t size);

  return realloc(ptr, count);
}

void* au_memmove(void* destination, void* source, size_t count) {
  extern void* memmove(void* destination, void* source, size_t count);

  return memmove(destination, source, count);
}

void* au_memcpy(void* destination, void* source, size_t count) {
  extern void* memcpy(void* destination, void* source, size_t count);

  return memcpy(destination, source, count);
}

au_unit_t au_free(void* ptr) {
  extern void free(void* ptr);

  free(ptr);
  return nil;
};

/*
 * CLI functions
 */

static int _au_argc = -1;

static char** _au_argv = NULL;

void au_store_cli_args(int argc, char** argv) {
  // Sanity checks.
  if (argc < 0) {
    au_abort_internal("Entrypoint error: argc is negative.");
  }
  if (argv == NULL) {
    au_abort_internal("Entrypoint error: argv is NULL.");
  }
  // Store values.
  _au_argc = argc;
  _au_argv = argv;
}

size_t au_get_argc() {
  // Sanity check.
  if (_au_argc == -1) {
    au_abort_internal("Prelude error: argc was not set.");
  }
  // Correctness argument: if _au_argc is non-negative, being an `int`, it
  // should fit inside `size_t`.
  size_t argc = (size_t)(_au_argc);
  return argc;
}

size_t _au_bounded_strlen(char* string, size_t bound) {
  size_t size = 0;
  for(size_t idx = 0; idx <= bound; idx++) {
    if (string[idx] == '\0') {
      return size;
    }
    size++;
  }
  au_abort_internal("Command line argument exceeds maximum length of 10 kibibytes.");
}

/* One kibibyte in bytes. */
#define AU_KIBIBYTE 1024
/* The maximum size of each CLI arg. */
#define AU_MAX_ARG_SIZE (10*AU_KIBIBYTE)

au_span_t au_get_nth_arg(size_t n) {
  // Sanity check.
  if (_au_argv == NULL) {
    au_abort_internal("Prelude error: argv was not set.");
  }
  size_t argc = au_get_argc();
  // Check array bounds.
  if (n >= argc) {
    au_abort_internal("Command line argument access out of bounds.");
  }
  // Retrieve the nth argument.
  char* arg = _au_argv[n];
  // Check non-null.
  if (arg == NULL) {
    au_abort_internal("Prelude error: command-line argument is NULL.");
  }
  // Measure the length.
  size_t size = _au_bounded_strlen(arg, AU_MAX_ARG_SIZE);
  // Otherwise, return it.
  au_span_t arg_array = ((au_span_t){ .data = (void*)arg, .size = size });
  return arg_array;
}


/* --- BEGIN translation unit for module 'Austral.Pervasive' --- */
/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
typedef enum {
    mono_2_tag_Some, mono_2_tag_None
} mono_2_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width16))]
*/
typedef enum {
    mono_3_tag_Some, mono_3_tag_None
} mono_3_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width32))]
*/
typedef enum {
    mono_4_tag_Some, mono_4_tag_None
} mono_4_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width64))]
*/
typedef enum {
    mono_5_tag_Some, mono_5_tag_None
} mono_5_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Signed, Type.Width8))]
*/
typedef enum {
    mono_6_tag_Some, mono_6_tag_None
} mono_6_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Signed, Type.Width16))]
*/
typedef enum {
    mono_7_tag_Some, mono_7_tag_None
} mono_7_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Signed, Type.Width32))]
*/
typedef enum {
    mono_8_tag_Some, mono_8_tag_None
} mono_8_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Signed, Type.Width64))]
*/
typedef enum {
    mono_9_tag_Some, mono_9_tag_None
} mono_9_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoInteger (Type.Unsigned, Type.WidthIndex))]
*/
typedef enum {
    mono_10_tag_Some, mono_10_tag_None
} mono_10_tag;

/*
  Union monomorph tag enum: Option[MonoType.MonoSingleFloat]
*/
typedef enum {
    mono_11_tag_Some, mono_11_tag_None
} mono_11_tag;

/*
  Union monomorph tag enum: Option[MonoType.MonoDoubleFloat]
*/
typedef enum {
    mono_12_tag_Some, mono_12_tag_None
} mono_12_tag;

/*
  Record monomorph: RootCapability[]
*/
struct mono_1;
typedef struct mono_1 mono_1;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
struct mono_2;
typedef struct mono_2 mono_2;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width16))]
*/
struct mono_3;
typedef struct mono_3 mono_3;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width32))]
*/
struct mono_4;
typedef struct mono_4 mono_4;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width64))]
*/
struct mono_5;
typedef struct mono_5 mono_5;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width8))]
*/
struct mono_6;
typedef struct mono_6 mono_6;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width16))]
*/
struct mono_7;
typedef struct mono_7 mono_7;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width32))]
*/
struct mono_8;
typedef struct mono_8 mono_8;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width64))]
*/
struct mono_9;
typedef struct mono_9 mono_9;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.WidthIndex))]
*/
struct mono_10;
typedef struct mono_10 mono_10;

/*
  Union monomorph: Option[MonoType.MonoSingleFloat]
*/
struct mono_11;
typedef struct mono_11 mono_11;

/*
  Union monomorph: Option[MonoType.MonoDoubleFloat]
*/
struct mono_12;
typedef struct mono_12 mono_12;

/*
  Record monomorph: RootCapability[]
*/
typedef struct mono_1 {au_unit_t value;} mono_1;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
typedef struct mono_2 {mono_2_tag tag;union { struct  {au_nat8_t value;} Some;struct  {} None;} data;} mono_2;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width16))]
*/
typedef struct mono_3 {mono_3_tag tag;union { struct  {au_nat16_t value;} Some;struct  {} None;} data;} mono_3;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width32))]
*/
typedef struct mono_4 {mono_4_tag tag;union { struct  {au_nat32_t value;} Some;struct  {} None;} data;} mono_4;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.Width64))]
*/
typedef struct mono_5 {mono_5_tag tag;union { struct  {au_nat64_t value;} Some;struct  {} None;} data;} mono_5;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width8))]
*/
typedef struct mono_6 {mono_6_tag tag;union { struct  {au_int8_t value;} Some;struct  {} None;} data;} mono_6;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width16))]
*/
typedef struct mono_7 {mono_7_tag tag;union { struct  {au_int16_t value;} Some;struct  {} None;} data;} mono_7;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width32))]
*/
typedef struct mono_8 {mono_8_tag tag;union { struct  {au_int32_t value;} Some;struct  {} None;} data;} mono_8;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Signed, Type.Width64))]
*/
typedef struct mono_9 {mono_9_tag tag;union { struct  {au_int64_t value;} Some;struct  {} None;} data;} mono_9;

/*
  Union monomorph: Option[(MonoType.MonoInteger (Type.Unsigned, Type.WidthIndex))]
*/
typedef struct mono_10 {mono_10_tag tag;union { struct  {au_index_t value;} Some;struct  {} None;} data;} mono_10;

/*
  Union monomorph: Option[MonoType.MonoSingleFloat]
*/
typedef struct mono_11 {mono_11_tag tag;union { struct  {float value;} Some;struct  {} None;} data;} mono_11;

/*
  Union monomorph: Option[MonoType.MonoDoubleFloat]
*/
typedef struct mono_12 {mono_12_tag tag;union { struct  {double value;} Some;struct  {} None;} data;} mono_12;

/*
  Constant
*/
#define Austral__Pervasive____maximum_nat8 ((au_nat8_t)(UINT8_MAX))

/*
  Constant
*/
#define Austral__Pervasive____maximum_nat16 ((au_nat16_t)(UINT16_MAX))

/*
  Constant
*/
#define Austral__Pervasive____maximum_nat32 ((au_nat32_t)(UINT32_MAX))

/*
  Constant
*/
#define Austral__Pervasive____maximum_nat64 ((au_nat64_t)(UINT64_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_int8 ((au_int8_t)(INT8_MIN))

/*
  Constant
*/
#define Austral__Pervasive____maximum_int8 ((au_int8_t)(INT8_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_int16 ((au_int16_t)(INT16_MIN))

/*
  Constant
*/
#define Austral__Pervasive____maximum_int16 ((au_int16_t)(INT16_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_int32 ((au_int32_t)(INT32_MIN))

/*
  Constant
*/
#define Austral__Pervasive____maximum_int32 ((au_int32_t)(INT32_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_int64 ((au_int64_t)(INT64_MIN))

/*
  Constant
*/
#define Austral__Pervasive____maximum_int64 ((au_int64_t)(INT64_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_index 0

/*
  Constant
*/
#define Austral__Pervasive____maximum_index ((au_index_t)(SIZE_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_bytesize 0

/*
  Constant
*/
#define Austral__Pervasive____maximum_bytesize ((size_t)(SIZE_MAX))

/*
  Constant
*/
#define Austral__Pervasive____minimum_nat8 0

/*
  Constant
*/
#define Austral__Pervasive____minimum_nat16 0

/*
  Constant
*/
#define Austral__Pervasive____minimum_nat32 0

/*
  Constant
*/
#define Austral__Pervasive____minimum_nat64 0

/*
  Function forward declaratioon
*/
au_unit_t decl_9(au_span_t message);

/*
  Function forward declaratioon
*/
au_unit_t decl_11(mono_1 cap);

/*
  Function forward declaratioon
*/
au_index_t decl_12();

/*
  Function forward declaratioon
*/
au_span_t decl_13(au_index_t n);

/*
  Method forward declaration
*/
au_nat8_t meth_1(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_2(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_3(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_4(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_5(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_6(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_7(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_8(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_9(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_10(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_11(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_12(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_13(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_14(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_15(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_16(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_17(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_18(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_19(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_20(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_21(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_22(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_23(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_24(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_25(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_26(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_27(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_28(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_29(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_30(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_31(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_32(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_33(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_34(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_35(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_36(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
size_t meth_37(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_38(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_39(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_40(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
double meth_41(double lhs, double rhs);

/*
  Method forward declaration
*/
double meth_42(double lhs, double rhs);

/*
  Method forward declaration
*/
double meth_43(double lhs, double rhs);

/*
  Method forward declaration
*/
double meth_44(double lhs, double rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_45(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_46(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_47(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_48(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_49(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_50(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_51(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_52(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_53(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_54(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_55(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_56(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_57(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_58(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_59(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_60(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_61(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_62(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_63(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_64(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_65(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_66(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_67(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_68(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_69(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_70(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_71(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_72(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_73(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_74(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_75(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_76(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_77(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_78(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_79(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_80(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
size_t meth_81(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_82(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_83(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_84(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_85(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_86(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_87(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat8_t meth_88(au_nat8_t value);

/*
  Method forward declaration
*/
au_int8_t meth_89(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_90(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_91(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_92(au_int8_t value);

/*
  Method forward declaration
*/
au_nat16_t meth_93(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_94(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_95(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_96(au_nat16_t value);

/*
  Method forward declaration
*/
au_int16_t meth_97(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_98(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_99(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_100(au_int16_t value);

/*
  Method forward declaration
*/
au_nat32_t meth_101(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_102(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_103(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_104(au_nat32_t value);

/*
  Method forward declaration
*/
au_int32_t meth_105(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_106(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_107(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_108(au_int32_t value);

/*
  Method forward declaration
*/
au_nat64_t meth_109(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_110(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_111(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_112(au_nat64_t value);

/*
  Method forward declaration
*/
au_int64_t meth_113(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_114(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_115(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_116(au_int64_t value);

/*
  Method forward declaration
*/
au_index_t meth_117(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_118(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_119(au_index_t lhs, au_index_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_120(au_index_t value);

/*
  Method forward declaration
*/
size_t meth_121(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_122(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_123(size_t lhs, size_t rhs);

/*
  Method forward declaration
*/
size_t meth_124(size_t value);

/*
  Method forward declaration
*/
au_unit_t meth_125(au_unit_t value);

/*
  Method forward declaration
*/
au_unit_t meth_126(au_bool_t value);

/*
  Method forward declaration
*/
au_unit_t meth_127(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_128(au_nat8_t value);

/*
  Method forward declaration
*/
au_unit_t meth_129(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_130(au_int8_t value);

/*
  Method forward declaration
*/
au_unit_t meth_131(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_132(au_nat16_t value);

/*
  Method forward declaration
*/
au_unit_t meth_133(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_134(au_int16_t value);

/*
  Method forward declaration
*/
au_unit_t meth_135(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_136(au_nat32_t value);

/*
  Method forward declaration
*/
au_unit_t meth_137(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_138(au_int32_t value);

/*
  Method forward declaration
*/
au_unit_t meth_139(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_140(au_nat64_t value);

/*
  Method forward declaration
*/
au_unit_t meth_141(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_142(au_int64_t value);

/*
  Method forward declaration
*/
au_unit_t meth_143(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_144(au_index_t value);

/*
  Method forward declaration
*/
au_unit_t meth_145(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_146(size_t value);

/*
  Method forward declaration
*/
au_unit_t meth_147(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_148(float value);

/*
  Method forward declaration
*/
au_unit_t meth_149(au_span_t array);

/*
  Method forward declaration
*/
au_unit_t meth_150(double value);

/*
  Method forward declaration
*/
au_unit_t meth_151(au_span_t array);

/*
  Method forward declaration
*/
mono_2 meth_154(au_nat8_t value);

/*
  Method forward declaration
*/
mono_2 meth_155(au_nat16_t value);

/*
  Method forward declaration
*/
mono_2 meth_156(au_nat32_t value);

/*
  Method forward declaration
*/
mono_2 meth_157(au_nat64_t value);

/*
  Method forward declaration
*/
mono_2 meth_158(au_int8_t value);

/*
  Method forward declaration
*/
mono_2 meth_159(au_int16_t value);

/*
  Method forward declaration
*/
mono_2 meth_160(au_int32_t value);

/*
  Method forward declaration
*/
mono_2 meth_161(au_int64_t value);

/*
  Method forward declaration
*/
mono_2 meth_162(au_index_t value);

/*
  Method forward declaration
*/
mono_2 meth_163(float value);

/*
  Method forward declaration
*/
mono_2 meth_164(double value);

/*
  Method forward declaration
*/
mono_3 meth_165(au_nat8_t value);

/*
  Method forward declaration
*/
mono_3 meth_166(au_nat16_t value);

/*
  Method forward declaration
*/
mono_3 meth_167(au_nat32_t value);

/*
  Method forward declaration
*/
mono_3 meth_168(au_nat64_t value);

/*
  Method forward declaration
*/
mono_3 meth_169(au_int8_t value);

/*
  Method forward declaration
*/
mono_3 meth_170(au_int16_t value);

/*
  Method forward declaration
*/
mono_3 meth_171(au_int32_t value);

/*
  Method forward declaration
*/
mono_3 meth_172(au_int64_t value);

/*
  Method forward declaration
*/
mono_3 meth_173(au_index_t value);

/*
  Method forward declaration
*/
mono_3 meth_174(float value);

/*
  Method forward declaration
*/
mono_3 meth_175(double value);

/*
  Method forward declaration
*/
mono_4 meth_176(au_nat8_t value);

/*
  Method forward declaration
*/
mono_4 meth_177(au_nat16_t value);

/*
  Method forward declaration
*/
mono_4 meth_178(au_nat32_t value);

/*
  Method forward declaration
*/
mono_4 meth_179(au_nat64_t value);

/*
  Method forward declaration
*/
mono_4 meth_180(au_int8_t value);

/*
  Method forward declaration
*/
mono_4 meth_181(au_int16_t value);

/*
  Method forward declaration
*/
mono_4 meth_182(au_int32_t value);

/*
  Method forward declaration
*/
mono_4 meth_183(au_int64_t value);

/*
  Method forward declaration
*/
mono_4 meth_184(au_index_t value);

/*
  Method forward declaration
*/
mono_4 meth_185(float value);

/*
  Method forward declaration
*/
mono_4 meth_186(double value);

/*
  Method forward declaration
*/
mono_5 meth_187(au_nat8_t value);

/*
  Method forward declaration
*/
mono_5 meth_188(au_nat16_t value);

/*
  Method forward declaration
*/
mono_5 meth_189(au_nat32_t value);

/*
  Method forward declaration
*/
mono_5 meth_190(au_nat64_t value);

/*
  Method forward declaration
*/
mono_5 meth_191(au_int8_t value);

/*
  Method forward declaration
*/
mono_5 meth_192(au_int16_t value);

/*
  Method forward declaration
*/
mono_5 meth_193(au_int32_t value);

/*
  Method forward declaration
*/
mono_5 meth_194(au_int64_t value);

/*
  Method forward declaration
*/
mono_5 meth_195(au_index_t value);

/*
  Method forward declaration
*/
mono_5 meth_196(float value);

/*
  Method forward declaration
*/
mono_5 meth_197(double value);

/*
  Method forward declaration
*/
mono_6 meth_198(au_nat8_t value);

/*
  Method forward declaration
*/
mono_6 meth_199(au_nat16_t value);

/*
  Method forward declaration
*/
mono_6 meth_200(au_nat32_t value);

/*
  Method forward declaration
*/
mono_6 meth_201(au_nat64_t value);

/*
  Method forward declaration
*/
mono_6 meth_202(au_int8_t value);

/*
  Method forward declaration
*/
mono_6 meth_203(au_int16_t value);

/*
  Method forward declaration
*/
mono_6 meth_204(au_int32_t value);

/*
  Method forward declaration
*/
mono_6 meth_205(au_int64_t value);

/*
  Method forward declaration
*/
mono_6 meth_206(au_index_t value);

/*
  Method forward declaration
*/
mono_6 meth_207(float value);

/*
  Method forward declaration
*/
mono_6 meth_208(double value);

/*
  Method forward declaration
*/
mono_7 meth_209(au_nat8_t value);

/*
  Method forward declaration
*/
mono_7 meth_210(au_nat16_t value);

/*
  Method forward declaration
*/
mono_7 meth_211(au_nat32_t value);

/*
  Method forward declaration
*/
mono_7 meth_212(au_nat64_t value);

/*
  Method forward declaration
*/
mono_7 meth_213(au_int8_t value);

/*
  Method forward declaration
*/
mono_7 meth_214(au_int16_t value);

/*
  Method forward declaration
*/
mono_7 meth_215(au_int32_t value);

/*
  Method forward declaration
*/
mono_7 meth_216(au_int64_t value);

/*
  Method forward declaration
*/
mono_7 meth_217(au_index_t value);

/*
  Method forward declaration
*/
mono_7 meth_218(float value);

/*
  Method forward declaration
*/
mono_7 meth_219(double value);

/*
  Method forward declaration
*/
mono_8 meth_220(au_nat8_t value);

/*
  Method forward declaration
*/
mono_8 meth_221(au_nat16_t value);

/*
  Method forward declaration
*/
mono_8 meth_222(au_nat32_t value);

/*
  Method forward declaration
*/
mono_8 meth_223(au_nat64_t value);

/*
  Method forward declaration
*/
mono_8 meth_224(au_int8_t value);

/*
  Method forward declaration
*/
mono_8 meth_225(au_int16_t value);

/*
  Method forward declaration
*/
mono_8 meth_226(au_int32_t value);

/*
  Method forward declaration
*/
mono_8 meth_227(au_int64_t value);

/*
  Method forward declaration
*/
mono_8 meth_228(au_index_t value);

/*
  Method forward declaration
*/
mono_8 meth_229(float value);

/*
  Method forward declaration
*/
mono_8 meth_230(double value);

/*
  Method forward declaration
*/
mono_9 meth_231(au_nat8_t value);

/*
  Method forward declaration
*/
mono_9 meth_232(au_nat16_t value);

/*
  Method forward declaration
*/
mono_9 meth_233(au_nat32_t value);

/*
  Method forward declaration
*/
mono_9 meth_234(au_nat64_t value);

/*
  Method forward declaration
*/
mono_9 meth_235(au_int8_t value);

/*
  Method forward declaration
*/
mono_9 meth_236(au_int16_t value);

/*
  Method forward declaration
*/
mono_9 meth_237(au_int32_t value);

/*
  Method forward declaration
*/
mono_9 meth_238(au_int64_t value);

/*
  Method forward declaration
*/
mono_9 meth_239(au_index_t value);

/*
  Method forward declaration
*/
mono_9 meth_240(float value);

/*
  Method forward declaration
*/
mono_9 meth_241(double value);

/*
  Method forward declaration
*/
mono_10 meth_242(au_nat8_t value);

/*
  Method forward declaration
*/
mono_10 meth_243(au_nat16_t value);

/*
  Method forward declaration
*/
mono_10 meth_244(au_nat32_t value);

/*
  Method forward declaration
*/
mono_10 meth_245(au_nat64_t value);

/*
  Method forward declaration
*/
mono_10 meth_246(au_int8_t value);

/*
  Method forward declaration
*/
mono_10 meth_247(au_int16_t value);

/*
  Method forward declaration
*/
mono_10 meth_248(au_int32_t value);

/*
  Method forward declaration
*/
mono_10 meth_249(au_int64_t value);

/*
  Method forward declaration
*/
mono_10 meth_250(au_index_t value);

/*
  Method forward declaration
*/
mono_10 meth_251(float value);

/*
  Method forward declaration
*/
mono_10 meth_252(double value);

/*
  Method forward declaration
*/
mono_11 meth_253(au_nat8_t value);

/*
  Method forward declaration
*/
mono_11 meth_254(au_nat16_t value);

/*
  Method forward declaration
*/
mono_11 meth_255(au_nat32_t value);

/*
  Method forward declaration
*/
mono_11 meth_256(au_nat64_t value);

/*
  Method forward declaration
*/
mono_11 meth_257(au_int8_t value);

/*
  Method forward declaration
*/
mono_11 meth_258(au_int16_t value);

/*
  Method forward declaration
*/
mono_11 meth_259(au_int32_t value);

/*
  Method forward declaration
*/
mono_11 meth_260(au_int64_t value);

/*
  Method forward declaration
*/
mono_11 meth_261(au_index_t value);

/*
  Method forward declaration
*/
mono_11 meth_262(float value);

/*
  Method forward declaration
*/
mono_11 meth_263(double value);

/*
  Method forward declaration
*/
mono_12 meth_264(au_nat8_t value);

/*
  Method forward declaration
*/
mono_12 meth_265(au_nat16_t value);

/*
  Method forward declaration
*/
mono_12 meth_266(au_nat32_t value);

/*
  Method forward declaration
*/
mono_12 meth_267(au_nat64_t value);

/*
  Method forward declaration
*/
mono_12 meth_268(au_int8_t value);

/*
  Method forward declaration
*/
mono_12 meth_269(au_int16_t value);

/*
  Method forward declaration
*/
mono_12 meth_270(au_int32_t value);

/*
  Method forward declaration
*/
mono_12 meth_271(au_int64_t value);

/*
  Method forward declaration
*/
mono_12 meth_272(au_index_t value);

/*
  Method forward declaration
*/
mono_12 meth_273(float value);

/*
  Method forward declaration
*/
mono_12 meth_274(double value);

/*
  Method forward declaration
*/
au_nat8_t meth_275(au_nat8_t lhs, au_nat8_t rhs);

/*
  Method forward declaration
*/
au_nat16_t meth_276(au_nat16_t lhs, au_nat16_t rhs);

/*
  Method forward declaration
*/
au_nat32_t meth_277(au_nat32_t lhs, au_nat32_t rhs);

/*
  Method forward declaration
*/
au_nat64_t meth_278(au_nat64_t lhs, au_nat64_t rhs);

/*
  Method forward declaration
*/
au_int8_t meth_279(au_int8_t lhs, au_int8_t rhs);

/*
  Method forward declaration
*/
au_int16_t meth_280(au_int16_t lhs, au_int16_t rhs);

/*
  Method forward declaration
*/
au_int32_t meth_281(au_int32_t lhs, au_int32_t rhs);

/*
  Method forward declaration
*/
au_int64_t meth_282(au_int64_t lhs, au_int64_t rhs);

/*
  Method forward declaration
*/
au_index_t meth_283(au_index_t lhs, au_index_t rhs);

/*
  Function
*/
au_unit_t decl_9(au_span_t message) {
    return ((au_unit_t)(au_abort(message)));
}

/*
  Function
*/
au_unit_t decl_11(mono_1 cap) {
    mono_1 tmp0 = cap;
    au_unit_t value = (tmp0).value;
    return false;
}

/*
  Function
*/
au_index_t decl_12() {
    return ((au_index_t)(au_get_argc()));
}

/*
  Function
*/
au_span_t decl_13(au_index_t n) {
    return ((au_span_t)(au_get_nth_arg(n)));
}

/*
  Method
*/
au_nat8_t meth_1(au_nat8_t lhs, au_nat8_t rhs) {
    au_nat8_t result;
    result = ((au_nat8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t0 = did_overflow;
    if (_t0) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Nat8)", 30));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat8_t meth_2(au_nat8_t lhs, au_nat8_t rhs) {
    au_nat8_t result;
    result = ((au_nat8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t1 = did_overflow;
    if (_t1) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Nat8)", 35));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat8_t meth_3(au_nat8_t lhs, au_nat8_t rhs) {
    au_nat8_t result;
    result = ((au_nat8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t2 = did_overflow;
    if (_t2) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Nat8)", 35));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat8_t meth_4(au_nat8_t lhs, au_nat8_t rhs) {
    au_bool_t _t3 = (rhs == 0);
    if (_t3) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Nat8)", 41));
    } else {
    }
    return ((au_nat8_t)(lhs / rhs));
}

/*
  Method
*/
au_int8_t meth_5(au_int8_t lhs, au_int8_t rhs) {
    au_int8_t result;
    result = ((au_int8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t4 = did_overflow;
    if (_t4) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Int8)", 30));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int8_t meth_6(au_int8_t lhs, au_int8_t rhs) {
    au_int8_t result;
    result = ((au_int8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t5 = did_overflow;
    if (_t5) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Int8)", 35));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int8_t meth_7(au_int8_t lhs, au_int8_t rhs) {
    au_int8_t result;
    result = ((au_int8_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t6 = did_overflow;
    if (_t6) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Int8)", 35));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int8_t meth_8(au_int8_t lhs, au_int8_t rhs) {
    au_bool_t _t8 = (rhs == 0);
    if (_t8) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Int8)", 41));
    } else {
    }
    au_bool_t _t7 = ((lhs == Austral__Pervasive____minimum_int8) && (rhs == -1));
    if (_t7) {
        decl_9(au_make_span_from_string("Overflow in trappingDivide (Int8)", 33));
    } else {
    }
    return ((au_int8_t)(lhs / rhs));
}

/*
  Method
*/
au_nat16_t meth_9(au_nat16_t lhs, au_nat16_t rhs) {
    au_nat16_t result;
    result = ((au_nat16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t9 = did_overflow;
    if (_t9) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Nat16)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat16_t meth_10(au_nat16_t lhs, au_nat16_t rhs) {
    au_nat16_t result;
    result = ((au_nat16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t10 = did_overflow;
    if (_t10) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Nat16)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat16_t meth_11(au_nat16_t lhs, au_nat16_t rhs) {
    au_nat16_t result;
    result = ((au_nat16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t11 = did_overflow;
    if (_t11) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Nat16)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat16_t meth_12(au_nat16_t lhs, au_nat16_t rhs) {
    au_bool_t _t12 = (rhs == 0);
    if (_t12) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Nat16)", 42));
    } else {
    }
    return ((au_nat16_t)(lhs / rhs));
}

/*
  Method
*/
au_int16_t meth_13(au_int16_t lhs, au_int16_t rhs) {
    au_int16_t result;
    result = ((au_int16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t13 = did_overflow;
    if (_t13) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Int16)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int16_t meth_14(au_int16_t lhs, au_int16_t rhs) {
    au_int16_t result;
    result = ((au_int16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t14 = did_overflow;
    if (_t14) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Int16)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int16_t meth_15(au_int16_t lhs, au_int16_t rhs) {
    au_int16_t result;
    result = ((au_int16_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t15 = did_overflow;
    if (_t15) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Int16)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int16_t meth_16(au_int16_t lhs, au_int16_t rhs) {
    au_bool_t _t17 = (rhs == 0);
    if (_t17) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Int16)", 42));
    } else {
    }
    au_bool_t _t16 = ((lhs == Austral__Pervasive____minimum_int16) && (rhs == -1));
    if (_t16) {
        decl_9(au_make_span_from_string("Overflow in trappingDivide (Int16)", 34));
    } else {
    }
    return ((au_int16_t)(lhs / rhs));
}

/*
  Method
*/
au_nat32_t meth_17(au_nat32_t lhs, au_nat32_t rhs) {
    au_nat32_t result;
    result = ((au_nat32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t18 = did_overflow;
    if (_t18) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Nat32)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat32_t meth_18(au_nat32_t lhs, au_nat32_t rhs) {
    au_nat32_t result;
    result = ((au_nat32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t19 = did_overflow;
    if (_t19) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Nat32)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat32_t meth_19(au_nat32_t lhs, au_nat32_t rhs) {
    au_nat32_t result;
    result = ((au_nat32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t20 = did_overflow;
    if (_t20) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Nat32)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat32_t meth_20(au_nat32_t lhs, au_nat32_t rhs) {
    au_bool_t _t21 = (rhs == 0);
    if (_t21) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Nat32)", 42));
    } else {
    }
    return ((au_nat32_t)(lhs / rhs));
}

/*
  Method
*/
au_int32_t meth_21(au_int32_t lhs, au_int32_t rhs) {
    au_int32_t result;
    result = ((au_int32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t22 = did_overflow;
    if (_t22) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Int32)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int32_t meth_22(au_int32_t lhs, au_int32_t rhs) {
    au_int32_t result;
    result = ((au_int32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t23 = did_overflow;
    if (_t23) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Int32)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int32_t meth_23(au_int32_t lhs, au_int32_t rhs) {
    au_int32_t result;
    result = ((au_int32_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t24 = did_overflow;
    if (_t24) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Int32)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int32_t meth_24(au_int32_t lhs, au_int32_t rhs) {
    au_bool_t _t26 = (rhs == 0);
    if (_t26) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Int32)", 42));
    } else {
    }
    au_bool_t _t25 = ((lhs == Austral__Pervasive____minimum_int32) && (rhs == -1));
    if (_t25) {
        decl_9(au_make_span_from_string("Overflow in trappingDivide (Int32)", 34));
    } else {
    }
    return ((au_int32_t)(lhs / rhs));
}

/*
  Method
*/
au_nat64_t meth_25(au_nat64_t lhs, au_nat64_t rhs) {
    au_nat64_t result;
    result = ((au_nat64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t27 = did_overflow;
    if (_t27) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Nat64)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat64_t meth_26(au_nat64_t lhs, au_nat64_t rhs) {
    au_nat64_t result;
    result = ((au_nat64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t28 = did_overflow;
    if (_t28) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Nat64)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat64_t meth_27(au_nat64_t lhs, au_nat64_t rhs) {
    au_nat64_t result;
    result = ((au_nat64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t29 = did_overflow;
    if (_t29) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Nat64)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_nat64_t meth_28(au_nat64_t lhs, au_nat64_t rhs) {
    au_bool_t _t30 = (rhs == 0);
    if (_t30) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Nat64)", 42));
    } else {
    }
    return ((au_nat64_t)(lhs / rhs));
}

/*
  Method
*/
au_int64_t meth_29(au_int64_t lhs, au_int64_t rhs) {
    au_int64_t result;
    result = ((au_int64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t31 = did_overflow;
    if (_t31) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Int64)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int64_t meth_30(au_int64_t lhs, au_int64_t rhs) {
    au_int64_t result;
    result = ((au_int64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t32 = did_overflow;
    if (_t32) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Int64)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int64_t meth_31(au_int64_t lhs, au_int64_t rhs) {
    au_int64_t result;
    result = ((au_int64_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t33 = did_overflow;
    if (_t33) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Int64)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_int64_t meth_32(au_int64_t lhs, au_int64_t rhs) {
    au_bool_t _t35 = (rhs == 0);
    if (_t35) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Int64)", 42));
    } else {
    }
    au_bool_t _t34 = ((lhs == Austral__Pervasive____minimum_int64) && (rhs == -1));
    if (_t34) {
        decl_9(au_make_span_from_string("Overflow in trappingDivide (Int64)", 34));
    } else {
    }
    return ((au_int64_t)(lhs / rhs));
}

/*
  Method
*/
au_index_t meth_33(au_index_t lhs, au_index_t rhs) {
    au_index_t result;
    result = ((au_index_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t36 = did_overflow;
    if (_t36) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (Index)", 31));
    } else {
    }
    return result;
}

/*
  Method
*/
au_index_t meth_34(au_index_t lhs, au_index_t rhs) {
    au_index_t result;
    result = ((au_index_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t37 = did_overflow;
    if (_t37) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (Index)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_index_t meth_35(au_index_t lhs, au_index_t rhs) {
    au_index_t result;
    result = ((au_index_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t38 = did_overflow;
    if (_t38) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (Index)", 36));
    } else {
    }
    return result;
}

/*
  Method
*/
au_index_t meth_36(au_index_t lhs, au_index_t rhs) {
    au_bool_t _t39 = (rhs == 0);
    if (_t39) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Index)", 42));
    } else {
    }
    return ((au_index_t)(lhs / rhs));
}

/*
  Method
*/
size_t meth_37(size_t lhs, size_t rhs) {
    size_t result;
    result = ((size_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_add_overflow(lhs, rhs, &result)));
    au_bool_t _t40 = did_overflow;
    if (_t40) {
        decl_9(au_make_span_from_string("Overflow in trappingAdd (ByteSize)", 34));
    } else {
    }
    return result;
}

/*
  Method
*/
size_t meth_38(size_t lhs, size_t rhs) {
    size_t result;
    result = ((size_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_sub_overflow(lhs, rhs, &result)));
    au_bool_t _t41 = did_overflow;
    if (_t41) {
        decl_9(au_make_span_from_string("Overflow in trappingSubtract (ByteSize)", 39));
    } else {
    }
    return result;
}

/*
  Method
*/
size_t meth_39(size_t lhs, size_t rhs) {
    size_t result;
    result = ((size_t) 0);
    au_bool_t did_overflow;
    did_overflow = ((au_bool_t)(__builtin_mul_overflow(lhs, rhs, &result)));
    au_bool_t _t42 = did_overflow;
    if (_t42) {
        decl_9(au_make_span_from_string("Overflow in trappingMultiply (ByteSize)", 39));
    } else {
    }
    return result;
}

/*
  Method
*/
size_t meth_40(size_t lhs, size_t rhs) {
    au_bool_t _t43 = (rhs == 0);
    if (_t43) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (ByteSize)", 45));
    } else {
    }
    return ((size_t)(lhs / rhs));
}

/*
  Method
*/
double meth_41(double lhs, double rhs) {
    return ((double)(lhs + rhs));
}

/*
  Method
*/
double meth_42(double lhs, double rhs) {
    return ((double)(lhs - rhs));
}

/*
  Method
*/
double meth_43(double lhs, double rhs) {
    return ((double)(lhs * rhs));
}

/*
  Method
*/
double meth_44(double lhs, double rhs) {
    au_bool_t _t44 = (rhs == 0.0);
    if (_t44) {
        decl_9(au_make_span_from_string("Division by zero in trappingDivide (Float64)", 44));
    } else {
    }
    return ((double)(lhs / rhs));
}

/*
  Method
*/
au_nat8_t meth_45(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs + rhs));
}

/*
  Method
*/
au_nat8_t meth_46(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs - rhs));
}

/*
  Method
*/
au_nat8_t meth_47(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs * rhs));
}

/*
  Method
*/
au_nat8_t meth_48(au_nat8_t lhs, au_nat8_t rhs) {
    au_bool_t _t45 = (rhs == 0);
    if (_t45) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Nat8)", 40));
    } else {
    }
    return ((au_nat8_t)(lhs / rhs));
}

/*
  Method
*/
au_int8_t meth_49(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs + rhs));
}

/*
  Method
*/
au_int8_t meth_50(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs - rhs));
}

/*
  Method
*/
au_int8_t meth_51(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs * rhs));
}

/*
  Method
*/
au_int8_t meth_52(au_int8_t lhs, au_int8_t rhs) {
    au_bool_t _t46 = (rhs == 0);
    if (_t46) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Int8)", 40));
    } else {
    }
    return ((au_int8_t)(lhs / rhs));
}

/*
  Method
*/
au_nat16_t meth_53(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs + rhs));
}

/*
  Method
*/
au_nat16_t meth_54(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs - rhs));
}

/*
  Method
*/
au_nat16_t meth_55(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs * rhs));
}

/*
  Method
*/
au_nat16_t meth_56(au_nat16_t lhs, au_nat16_t rhs) {
    au_bool_t _t47 = (rhs == 0);
    if (_t47) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Nat16)", 41));
    } else {
    }
    return ((au_nat16_t)(lhs / rhs));
}

/*
  Method
*/
au_int16_t meth_57(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs + rhs));
}

/*
  Method
*/
au_int16_t meth_58(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs - rhs));
}

/*
  Method
*/
au_int16_t meth_59(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs * rhs));
}

/*
  Method
*/
au_int16_t meth_60(au_int16_t lhs, au_int16_t rhs) {
    au_bool_t _t48 = (rhs == 0);
    if (_t48) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Int16)", 41));
    } else {
    }
    return ((au_int16_t)(lhs / rhs));
}

/*
  Method
*/
au_nat32_t meth_61(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs + rhs));
}

/*
  Method
*/
au_nat32_t meth_62(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs - rhs));
}

/*
  Method
*/
au_nat32_t meth_63(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs * rhs));
}

/*
  Method
*/
au_nat32_t meth_64(au_nat32_t lhs, au_nat32_t rhs) {
    au_bool_t _t49 = (rhs == 0);
    if (_t49) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Nat32)", 41));
    } else {
    }
    return ((au_nat32_t)(lhs / rhs));
}

/*
  Method
*/
au_int32_t meth_65(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs + rhs));
}

/*
  Method
*/
au_int32_t meth_66(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs - rhs));
}

/*
  Method
*/
au_int32_t meth_67(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs * rhs));
}

/*
  Method
*/
au_int32_t meth_68(au_int32_t lhs, au_int32_t rhs) {
    au_bool_t _t50 = (rhs == 0);
    if (_t50) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Int32)", 41));
    } else {
    }
    return ((au_int32_t)(lhs / rhs));
}

/*
  Method
*/
au_nat64_t meth_69(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs + rhs));
}

/*
  Method
*/
au_nat64_t meth_70(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs - rhs));
}

/*
  Method
*/
au_nat64_t meth_71(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs * rhs));
}

/*
  Method
*/
au_nat64_t meth_72(au_nat64_t lhs, au_nat64_t rhs) {
    au_bool_t _t51 = (rhs == 0);
    if (_t51) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Nat64)", 41));
    } else {
    }
    return ((au_nat64_t)(lhs / rhs));
}

/*
  Method
*/
au_int64_t meth_73(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs + rhs));
}

/*
  Method
*/
au_int64_t meth_74(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs - rhs));
}

/*
  Method
*/
au_int64_t meth_75(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs * rhs));
}

/*
  Method
*/
au_int64_t meth_76(au_int64_t lhs, au_int64_t rhs) {
    au_bool_t _t52 = (rhs == 0);
    if (_t52) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Int64)", 41));
    } else {
    }
    return ((au_int64_t)(lhs / rhs));
}

/*
  Method
*/
au_index_t meth_77(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs + rhs));
}

/*
  Method
*/
au_index_t meth_78(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs - rhs));
}

/*
  Method
*/
au_index_t meth_79(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs * rhs));
}

/*
  Method
*/
au_index_t meth_80(au_index_t lhs, au_index_t rhs) {
    au_bool_t _t53 = (rhs == 0);
    if (_t53) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (Index)", 41));
    } else {
    }
    return ((au_index_t)(lhs / rhs));
}

/*
  Method
*/
size_t meth_81(size_t lhs, size_t rhs) {
    return ((size_t)(lhs + rhs));
}

/*
  Method
*/
size_t meth_82(size_t lhs, size_t rhs) {
    return ((size_t)(lhs - rhs));
}

/*
  Method
*/
size_t meth_83(size_t lhs, size_t rhs) {
    return ((size_t)(lhs * rhs));
}

/*
  Method
*/
size_t meth_84(size_t lhs, size_t rhs) {
    au_bool_t _t54 = (rhs == 0);
    if (_t54) {
        decl_9(au_make_span_from_string("Division by zero in modularDivide (ByteSize)", 44));
    } else {
    }
    return ((size_t)(lhs / rhs));
}

/*
  Method
*/
au_nat8_t meth_85(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs & rhs));
}

/*
  Method
*/
au_nat8_t meth_86(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs | rhs));
}

/*
  Method
*/
au_nat8_t meth_87(au_nat8_t lhs, au_nat8_t rhs) {
    return ((au_nat8_t)(lhs ^ rhs));
}

/*
  Method
*/
au_nat8_t meth_88(au_nat8_t value) {
    return ((au_nat8_t)(~ value));
}

/*
  Method
*/
au_int8_t meth_89(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs & rhs));
}

/*
  Method
*/
au_int8_t meth_90(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs | rhs));
}

/*
  Method
*/
au_int8_t meth_91(au_int8_t lhs, au_int8_t rhs) {
    return ((au_int8_t)(lhs ^ rhs));
}

/*
  Method
*/
au_int8_t meth_92(au_int8_t value) {
    return ((au_int8_t)(~ value));
}

/*
  Method
*/
au_nat16_t meth_93(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs & rhs));
}

/*
  Method
*/
au_nat16_t meth_94(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs | rhs));
}

/*
  Method
*/
au_nat16_t meth_95(au_nat16_t lhs, au_nat16_t rhs) {
    return ((au_nat16_t)(lhs ^ rhs));
}

/*
  Method
*/
au_nat16_t meth_96(au_nat16_t value) {
    return ((au_nat16_t)(~ value));
}

/*
  Method
*/
au_int16_t meth_97(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs & rhs));
}

/*
  Method
*/
au_int16_t meth_98(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs | rhs));
}

/*
  Method
*/
au_int16_t meth_99(au_int16_t lhs, au_int16_t rhs) {
    return ((au_int16_t)(lhs ^ rhs));
}

/*
  Method
*/
au_int16_t meth_100(au_int16_t value) {
    return ((au_int16_t)(~ value));
}

/*
  Method
*/
au_nat32_t meth_101(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs & rhs));
}

/*
  Method
*/
au_nat32_t meth_102(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs | rhs));
}

/*
  Method
*/
au_nat32_t meth_103(au_nat32_t lhs, au_nat32_t rhs) {
    return ((au_nat32_t)(lhs ^ rhs));
}

/*
  Method
*/
au_nat32_t meth_104(au_nat32_t value) {
    return ((au_nat32_t)(~ value));
}

/*
  Method
*/
au_int32_t meth_105(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs & rhs));
}

/*
  Method
*/
au_int32_t meth_106(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs | rhs));
}

/*
  Method
*/
au_int32_t meth_107(au_int32_t lhs, au_int32_t rhs) {
    return ((au_int32_t)(lhs ^ rhs));
}

/*
  Method
*/
au_int32_t meth_108(au_int32_t value) {
    return ((au_int32_t)(~ value));
}

/*
  Method
*/
au_nat64_t meth_109(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs & rhs));
}

/*
  Method
*/
au_nat64_t meth_110(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs | rhs));
}

/*
  Method
*/
au_nat64_t meth_111(au_nat64_t lhs, au_nat64_t rhs) {
    return ((au_nat64_t)(lhs ^ rhs));
}

/*
  Method
*/
au_nat64_t meth_112(au_nat64_t value) {
    return ((au_nat64_t)(~ value));
}

/*
  Method
*/
au_int64_t meth_113(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs & rhs));
}

/*
  Method
*/
au_int64_t meth_114(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs | rhs));
}

/*
  Method
*/
au_int64_t meth_115(au_int64_t lhs, au_int64_t rhs) {
    return ((au_int64_t)(lhs ^ rhs));
}

/*
  Method
*/
au_int64_t meth_116(au_int64_t value) {
    return ((au_int64_t)(~ value));
}

/*
  Method
*/
au_index_t meth_117(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs & rhs));
}

/*
  Method
*/
au_index_t meth_118(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs | rhs));
}

/*
  Method
*/
au_index_t meth_119(au_index_t lhs, au_index_t rhs) {
    return ((au_index_t)(lhs ^ rhs));
}

/*
  Method
*/
au_index_t meth_120(au_index_t value) {
    return ((au_index_t)(~ value));
}

/*
  Method
*/
size_t meth_121(size_t lhs, size_t rhs) {
    return ((size_t)(lhs & rhs));
}

/*
  Method
*/
size_t meth_122(size_t lhs, size_t rhs) {
    return ((size_t)(lhs | rhs));
}

/*
  Method
*/
size_t meth_123(size_t lhs, size_t rhs) {
    return ((size_t)(lhs ^ rhs));
}

/*
  Method
*/
size_t meth_124(size_t value) {
    return ((size_t)(~ value));
}

/*
  Method
*/
au_unit_t meth_125(au_unit_t value) {
    return ((au_unit_t)(au_printf("nil")));
    return value;
}

/*
  Method
*/
au_unit_t meth_126(au_bool_t value) {
    au_bool_t _t55 = value;
    if (_t55) {
        ((au_unit_t)(au_printf("true")));
    } else {
        ((au_unit_t)(au_printf("false")));
    }
    return false;
}

/*
  Method
*/
au_unit_t meth_127(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_128(au_nat8_t value) {
    return ((au_unit_t)(au_printf("%i", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_129(au_span_t array) {
    au_bool_t _t56 = (((au_index_t)(array.size)) > 0);
    if (_t56) {
        au_index_t _t57 = ((au_index_t) 0);
        au_index_t _t58 = meth_34(((au_index_t) ((au_index_t)(array.size))), ((au_index_t) 1));
        {
            size_t i = _t57;
            for (; i <= _t58; i++) {
                au_span_t* array_tmp_ref0 = &(array);
                ((au_unit_t)(au_printf("%c", *((au_nat8_t*) au_array_index(array_tmp_ref0, i, sizeof(au_nat8_t))))));
            }
        }
    } else {
    }
    return false;
}

/*
  Method
*/
au_unit_t meth_130(au_int8_t value) {
    return ((au_unit_t)(au_printf("%i", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_131(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_132(au_nat16_t value) {
    return ((au_unit_t)(au_printf("%i", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_133(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_134(au_int16_t value) {
    return ((au_unit_t)(au_printf("%i", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_135(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_136(au_nat32_t value) {
    return ((au_unit_t)(au_printf("%u", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_137(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_138(au_int32_t value) {
    return ((au_unit_t)(au_printf("%i", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_139(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_140(au_nat64_t value) {
    return ((au_unit_t)(au_printf("%zu", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_141(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_142(au_int64_t value) {
    return ((au_unit_t)(au_printf("%li", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_143(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_144(au_index_t value) {
    return ((au_unit_t)(au_printf("%zu", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_145(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_146(size_t value) {
    return ((au_unit_t)(au_printf("%zu", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_147(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_148(float value) {
    return ((au_unit_t)(au_printf("%f", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_149(au_span_t array) {
    return false;
}

/*
  Method
*/
au_unit_t meth_150(double value) {
    return ((au_unit_t)(au_printf("%f", value)));
    return false;
}

/*
  Method
*/
au_unit_t meth_151(au_span_t array) {
    return false;
}

/*
  Method
*/
mono_2 meth_154(au_nat8_t value) {
    return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_2 meth_155(au_nat16_t value) {
    au_bool_t _t59 = (value <= ((au_nat16_t)(UINT8_MAX)));
    if (_t59) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_156(au_nat32_t value) {
    au_bool_t _t60 = (value <= ((au_nat32_t)(UINT8_MAX)));
    if (_t60) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_157(au_nat64_t value) {
    au_bool_t _t61 = (value <= ((au_nat64_t)(UINT8_MAX)));
    if (_t61) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_158(au_int8_t value) {
    au_bool_t _t62 = (value >= 0);
    if (_t62) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_159(au_int16_t value) {
    au_bool_t _t63 = ((value >= 0) && (value <= ((au_int16_t)(UINT8_MAX))));
    if (_t63) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_160(au_int32_t value) {
    au_bool_t _t64 = ((value >= 0) && (value <= ((au_int32_t)(UINT8_MAX))));
    if (_t64) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_161(au_int64_t value) {
    au_bool_t _t65 = ((value >= 0) && (value <= ((au_int64_t)(UINT8_MAX))));
    if (_t65) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_162(au_index_t value) {
    au_bool_t _t66 = ((value >= 0) && (value <= ((au_index_t)(UINT8_MAX))));
    if (_t66) {
        return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
    } else {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_2 meth_163(float value) {
    return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
}

/*
  Method
*/
mono_2 meth_164(double value) {
    return ((mono_2) { .tag = mono_2_tag_Some, .data = { .Some = { .value = ((au_nat8_t)(((au_nat8_t)(value)))) } } });
}

/*
  Method
*/
mono_3 meth_165(au_nat8_t value) {
    return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
}

/*
  Method
*/
mono_3 meth_166(au_nat16_t value) {
    return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_3 meth_167(au_nat32_t value) {
    au_bool_t _t67 = (value <= ((au_nat32_t)(UINT16_MAX)));
    if (_t67) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_168(au_nat64_t value) {
    au_bool_t _t68 = (value <= ((au_nat64_t)(UINT16_MAX)));
    if (_t68) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_169(au_int8_t value) {
    au_bool_t _t69 = (value >= 0);
    if (_t69) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_170(au_int16_t value) {
    au_bool_t _t70 = (value >= 0);
    if (_t70) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_171(au_int32_t value) {
    au_bool_t _t71 = ((value >= 0) && (value <= ((au_int32_t)(UINT16_MAX))));
    if (_t71) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_172(au_int64_t value) {
    au_bool_t _t72 = ((value >= 0) && (value <= ((au_int64_t)(UINT16_MAX))));
    if (_t72) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_173(au_index_t value) {
    au_bool_t _t73 = ((value >= 0) && (value <= ((au_index_t)(UINT16_MAX))));
    if (_t73) {
        return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
    } else {
        return ((mono_3) { .tag = mono_3_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_3 meth_174(float value) {
    return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
}

/*
  Method
*/
mono_3 meth_175(double value) {
    return ((mono_3) { .tag = mono_3_tag_Some, .data = { .Some = { .value = ((au_nat16_t)(((au_nat16_t)(value)))) } } });
}

/*
  Method
*/
mono_4 meth_176(au_nat8_t value) {
    return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
}

/*
  Method
*/
mono_4 meth_177(au_nat16_t value) {
    return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
}

/*
  Method
*/
mono_4 meth_178(au_nat32_t value) {
    return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_4 meth_179(au_nat64_t value) {
    au_bool_t _t74 = (value <= ((au_nat64_t)(UINT32_MAX)));
    if (_t74) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_180(au_int8_t value) {
    au_bool_t _t75 = (value >= 0);
    if (_t75) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_181(au_int16_t value) {
    au_bool_t _t76 = (value >= 0);
    if (_t76) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_182(au_int32_t value) {
    au_bool_t _t77 = (value >= 0);
    if (_t77) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_183(au_int64_t value) {
    au_bool_t _t78 = ((value >= 0) && (value <= ((au_int64_t)(UINT32_MAX))));
    if (_t78) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_184(au_index_t value) {
    au_bool_t _t79 = ((value >= 0) && (value <= ((au_index_t)(UINT32_MAX))));
    if (_t79) {
        return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
    } else {
        return ((mono_4) { .tag = mono_4_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_4 meth_185(float value) {
    return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
}

/*
  Method
*/
mono_4 meth_186(double value) {
    return ((mono_4) { .tag = mono_4_tag_Some, .data = { .Some = { .value = ((au_nat32_t)(((au_nat32_t)(value)))) } } });
}

/*
  Method
*/
mono_5 meth_187(au_nat8_t value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
}

/*
  Method
*/
mono_5 meth_188(au_nat16_t value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
}

/*
  Method
*/
mono_5 meth_189(au_nat32_t value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
}

/*
  Method
*/
mono_5 meth_190(au_nat64_t value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_5 meth_191(au_int8_t value) {
    au_bool_t _t80 = (value >= 0);
    if (_t80) {
        return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
    } else {
        return ((mono_5) { .tag = mono_5_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_5 meth_192(au_int16_t value) {
    au_bool_t _t81 = (value >= 0);
    if (_t81) {
        return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
    } else {
        return ((mono_5) { .tag = mono_5_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_5 meth_193(au_int32_t value) {
    au_bool_t _t82 = (value >= 0);
    if (_t82) {
        return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
    } else {
        return ((mono_5) { .tag = mono_5_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_5 meth_194(au_int64_t value) {
    au_bool_t _t83 = (value >= 0);
    if (_t83) {
        return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
    } else {
        return ((mono_5) { .tag = mono_5_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_5 meth_195(au_index_t value) {
    au_bool_t _t84 = ((value >= 0) && (value <= ((au_index_t)(UINT64_MAX))));
    if (_t84) {
        return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
    } else {
        return ((mono_5) { .tag = mono_5_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_5 meth_196(float value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
}

/*
  Method
*/
mono_5 meth_197(double value) {
    return ((mono_5) { .tag = mono_5_tag_Some, .data = { .Some = { .value = ((au_nat64_t)(((au_nat64_t)(value)))) } } });
}

/*
  Method
*/
mono_6 meth_198(au_nat8_t value) {
    au_bool_t _t85 = (value <= ((au_nat8_t)(INT8_MAX)));
    if (_t85) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_199(au_nat16_t value) {
    au_bool_t _t86 = (value <= ((au_nat16_t)(INT8_MAX)));
    if (_t86) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_200(au_nat32_t value) {
    au_bool_t _t87 = (value <= ((au_nat32_t)(INT8_MAX)));
    if (_t87) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_201(au_nat64_t value) {
    au_bool_t _t88 = (value <= ((au_nat64_t)(INT8_MAX)));
    if (_t88) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_202(au_int8_t value) {
    return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_6 meth_203(au_int16_t value) {
    au_bool_t _t89 = ((value >= ((au_int16_t)(INT8_MIN))) && (value <= ((au_int16_t)(INT8_MAX))));
    if (_t89) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_204(au_int32_t value) {
    au_bool_t _t90 = ((value >= ((au_int32_t)(INT8_MIN))) && (value <= ((au_int32_t)(INT8_MAX))));
    if (_t90) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_205(au_int64_t value) {
    au_bool_t _t91 = ((value >= ((au_int64_t)(INT8_MIN))) && (value <= ((au_int64_t)(INT8_MAX))));
    if (_t91) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_206(au_index_t value) {
    au_bool_t _t92 = ((value >= ((au_index_t)(INT8_MIN))) && (value <= ((au_index_t)(INT8_MAX))));
    if (_t92) {
        return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
    } else {
        return ((mono_6) { .tag = mono_6_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_6 meth_207(float value) {
    return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
}

/*
  Method
*/
mono_6 meth_208(double value) {
    return ((mono_6) { .tag = mono_6_tag_Some, .data = { .Some = { .value = ((au_int8_t)(((au_int8_t)(value)))) } } });
}

/*
  Method
*/
mono_7 meth_209(au_nat8_t value) {
    return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
}

/*
  Method
*/
mono_7 meth_210(au_nat16_t value) {
    au_bool_t _t93 = (value <= ((au_nat16_t)(INT16_MAX)));
    if (_t93) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_211(au_nat32_t value) {
    au_bool_t _t94 = (value <= ((au_nat32_t)(INT16_MAX)));
    if (_t94) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_212(au_nat64_t value) {
    au_bool_t _t95 = (value <= ((au_nat64_t)(INT16_MAX)));
    if (_t95) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_213(au_int8_t value) {
    return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
}

/*
  Method
*/
mono_7 meth_214(au_int16_t value) {
    return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_7 meth_215(au_int32_t value) {
    au_bool_t _t96 = ((value >= ((au_int32_t)(INT16_MIN))) && (value <= ((au_int32_t)(INT16_MAX))));
    if (_t96) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_216(au_int64_t value) {
    au_bool_t _t97 = ((value >= ((au_int64_t)(INT16_MIN))) && (value <= ((au_int64_t)(INT16_MAX))));
    if (_t97) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_217(au_index_t value) {
    au_bool_t _t98 = ((value >= ((au_index_t)(INT16_MIN))) && (value <= ((au_index_t)(INT16_MAX))));
    if (_t98) {
        return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
    } else {
        return ((mono_7) { .tag = mono_7_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_7 meth_218(float value) {
    return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
}

/*
  Method
*/
mono_7 meth_219(double value) {
    return ((mono_7) { .tag = mono_7_tag_Some, .data = { .Some = { .value = ((au_int16_t)(((au_int16_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_220(au_nat8_t value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_221(au_nat16_t value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_222(au_nat32_t value) {
    au_bool_t _t99 = (value <= ((au_nat32_t)(INT32_MAX)));
    if (_t99) {
        return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
    } else {
        return ((mono_8) { .tag = mono_8_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_8 meth_223(au_nat64_t value) {
    au_bool_t _t100 = (value <= ((au_nat64_t)(INT32_MAX)));
    if (_t100) {
        return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
    } else {
        return ((mono_8) { .tag = mono_8_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_8 meth_224(au_int8_t value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_225(au_int16_t value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_226(au_int32_t value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_8 meth_227(au_int64_t value) {
    au_bool_t _t101 = ((value >= ((au_int64_t)(INT32_MIN))) && (value <= ((au_int64_t)(INT32_MAX))));
    if (_t101) {
        return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
    } else {
        return ((mono_8) { .tag = mono_8_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_8 meth_228(au_index_t value) {
    au_bool_t _t102 = (value <= ((au_index_t)(INT32_MAX)));
    if (_t102) {
        return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
    } else {
        return ((mono_8) { .tag = mono_8_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_8 meth_229(float value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_8 meth_230(double value) {
    return ((mono_8) { .tag = mono_8_tag_Some, .data = { .Some = { .value = ((au_int32_t)(((au_int32_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_231(au_nat8_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_232(au_nat16_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_233(au_nat32_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_234(au_nat64_t value) {
    au_bool_t _t103 = (value <= ((au_nat64_t)(INT64_MAX)));
    if (_t103) {
        return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
    } else {
        return ((mono_9) { .tag = mono_9_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_9 meth_235(au_int8_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_236(au_int16_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_237(au_int32_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_238(au_int64_t value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_9 meth_239(au_index_t value) {
    au_bool_t _t104 = ((value >= ((au_index_t)(INT64_MIN))) && (value <= ((au_index_t)(INT64_MAX))));
    if (_t104) {
        return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
    } else {
        return ((mono_9) { .tag = mono_9_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_9 meth_240(float value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_9 meth_241(double value) {
    return ((mono_9) { .tag = mono_9_tag_Some, .data = { .Some = { .value = ((au_int64_t)(((au_int64_t)(value)))) } } });
}

/*
  Method
*/
mono_10 meth_242(au_nat8_t value) {
    return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
}

/*
  Method
*/
mono_10 meth_243(au_nat16_t value) {
    au_bool_t _t105 = (value <= ((au_nat16_t)(SIZE_MAX)));
    if (_t105) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_244(au_nat32_t value) {
    au_bool_t _t106 = (value <= ((au_nat32_t)(SIZE_MAX)));
    if (_t106) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_245(au_nat64_t value) {
    au_bool_t _t107 = (value <= ((au_nat64_t)(SIZE_MAX)));
    if (_t107) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_246(au_int8_t value) {
    au_bool_t _t108 = (value >= 0);
    if (_t108) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_247(au_int16_t value) {
    au_bool_t _t109 = (value >= 0);
    if (_t109) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_248(au_int32_t value) {
    au_index_t cast;
    cast = ((au_index_t)(((au_index_t)(value))));
    au_bool_t _t110 = ((value >= 0) && (cast <= ((au_index_t)(SIZE_MAX))));
    if (_t110) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_249(au_int64_t value) {
    au_bool_t _t111 = ((value >= 0) && (value <= ((au_int64_t)(SIZE_MAX))));
    if (_t111) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Method
*/
mono_10 meth_250(au_index_t value) {
    return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_10 meth_251(float value) {
    return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
}

/*
  Method
*/
mono_10 meth_252(double value) {
    return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = ((au_index_t)(((au_index_t)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_253(au_nat8_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_254(au_nat16_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_255(au_nat32_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_256(au_nat64_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_257(au_int8_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_258(au_int16_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_259(au_int32_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_260(au_int64_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_261(au_index_t value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_11 meth_262(float value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
mono_11 meth_263(double value) {
    return ((mono_11) { .tag = mono_11_tag_Some, .data = { .Some = { .value = ((float)(((float)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_264(au_nat8_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((float)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_265(au_nat16_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_266(au_nat32_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_267(au_nat64_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_268(au_int8_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_269(au_int16_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_270(au_int32_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_271(au_int64_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_272(au_index_t value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_273(float value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = ((double)(((double)(value)))) } } });
}

/*
  Method
*/
mono_12 meth_274(double value) {
    return ((mono_12) { .tag = mono_12_tag_Some, .data = { .Some = { .value = value } } });
}

/*
  Method
*/
au_nat8_t meth_275(au_nat8_t lhs, au_nat8_t rhs) {
    au_bool_t _t112 = (rhs == 0);
    if (_t112) {
        decl_9(au_make_span_from_string("Division by zero in rem(Nat8)", 29));
    } else {
    }
    return ((au_nat8_t)((lhs % rhs)));
}

/*
  Method
*/
au_nat16_t meth_276(au_nat16_t lhs, au_nat16_t rhs) {
    au_bool_t _t113 = (rhs == 0);
    if (_t113) {
        decl_9(au_make_span_from_string("Division by zero in rem(Nat16)", 30));
    } else {
    }
    return ((au_nat16_t)((lhs % rhs)));
}

/*
  Method
*/
au_nat32_t meth_277(au_nat32_t lhs, au_nat32_t rhs) {
    au_bool_t _t114 = (rhs == 0);
    if (_t114) {
        decl_9(au_make_span_from_string("Division by zero in rem(Nat32)", 30));
    } else {
    }
    return ((au_nat32_t)((lhs % rhs)));
}

/*
  Method
*/
au_nat64_t meth_278(au_nat64_t lhs, au_nat64_t rhs) {
    au_bool_t _t115 = (rhs == 0);
    if (_t115) {
        decl_9(au_make_span_from_string("Division by zero in rem(Nat64)", 30));
    } else {
    }
    return ((au_nat64_t)((lhs % rhs)));
}

/*
  Method
*/
au_int8_t meth_279(au_int8_t lhs, au_int8_t rhs) {
    au_bool_t _t117 = (rhs == 0);
    if (_t117) {
        decl_9(au_make_span_from_string("Division by zero in rem(Int8)", 29));
    } else {
    }
    au_bool_t _t116 = ((lhs == Austral__Pervasive____minimum_int8) && (rhs == -1));
    if (_t116) {
        decl_9(au_make_span_from_string("Overflow in rem(Int8)", 21));
    } else {
    }
    return ((au_int8_t)((lhs % rhs)));
}

/*
  Method
*/
au_int16_t meth_280(au_int16_t lhs, au_int16_t rhs) {
    au_bool_t _t119 = (rhs == 0);
    if (_t119) {
        decl_9(au_make_span_from_string("Division by zero in rem(Int16)", 30));
    } else {
    }
    au_bool_t _t118 = ((lhs == Austral__Pervasive____minimum_int16) && (rhs == -1));
    if (_t118) {
        decl_9(au_make_span_from_string("Overflow in rem(Int16)", 22));
    } else {
    }
    return ((au_int16_t)((lhs % rhs)));
}

/*
  Method
*/
au_int32_t meth_281(au_int32_t lhs, au_int32_t rhs) {
    au_bool_t _t121 = (rhs == 0);
    if (_t121) {
        decl_9(au_make_span_from_string("Division by zero in rem(Int32)", 30));
    } else {
    }
    au_bool_t _t120 = ((lhs == Austral__Pervasive____minimum_int32) && (rhs == -1));
    if (_t120) {
        decl_9(au_make_span_from_string("Overflow in rem(Int32)", 22));
    } else {
    }
    return ((au_int32_t)((lhs % rhs)));
}

/*
  Method
*/
au_int64_t meth_282(au_int64_t lhs, au_int64_t rhs) {
    au_bool_t _t123 = (rhs == 0);
    if (_t123) {
        decl_9(au_make_span_from_string("Division by zero in rem(Int64)", 30));
    } else {
    }
    au_bool_t _t122 = ((lhs == Austral__Pervasive____minimum_int64) && (rhs == -1));
    if (_t122) {
        decl_9(au_make_span_from_string("Overflow in rem(Int64)", 22));
    } else {
    }
    return ((au_int64_t)((lhs % rhs)));
}

/*
  Method
*/
au_index_t meth_283(au_index_t lhs, au_index_t rhs) {
    au_bool_t _t124 = (rhs == 0);
    if (_t124) {
        decl_9(au_make_span_from_string("Division by zero in rem(Index)", 30));
    } else {
    }
    return ((au_index_t)((lhs % rhs)));
}
/* --- END translation unit for module 'Austral.Pervasive' --- */

/* --- BEGIN translation unit for module 'Austral.Memory' --- */
/*
  Function forward declaratioon
*/
size_t decl_272(size_t elem_size, au_index_t count);

/*
  Function forward declaratioon
*/
au_index_t decl_280(size_t size);

/*
  Function
*/
size_t decl_272(size_t elem_size, au_index_t count) {
    size_t count_bytes;
    count_bytes = ((size_t)(count));
    size_t size;
    size = meth_39(((size_t) elem_size), ((size_t) count_bytes));
    return size;
}

/*
  Function
*/
au_index_t decl_280(size_t size) {
    return ((au_index_t)(size));
}
/* --- END translation unit for module 'Austral.Memory' --- */

/* --- BEGIN translation unit for module 'Standard.Tuples' --- */

/* --- END translation unit for module 'Standard.Tuples' --- */

/* --- BEGIN translation unit for module 'Standard.Bounded' --- */
/*
  Method forward declaration
*/
au_bool_t meth_284();

/*
  Method forward declaration
*/
au_bool_t meth_285();

/*
  Method forward declaration
*/
au_nat8_t meth_286();

/*
  Method forward declaration
*/
au_nat8_t meth_287();

/*
  Method forward declaration
*/
au_nat16_t meth_288();

/*
  Method forward declaration
*/
au_nat16_t meth_289();

/*
  Method forward declaration
*/
au_nat32_t meth_290();

/*
  Method forward declaration
*/
au_nat32_t meth_291();

/*
  Method forward declaration
*/
au_nat64_t meth_292();

/*
  Method forward declaration
*/
au_nat64_t meth_293();

/*
  Method forward declaration
*/
au_int8_t meth_294();

/*
  Method forward declaration
*/
au_int8_t meth_295();

/*
  Method forward declaration
*/
au_int16_t meth_296();

/*
  Method forward declaration
*/
au_int16_t meth_297();

/*
  Method forward declaration
*/
au_int32_t meth_298();

/*
  Method forward declaration
*/
au_int32_t meth_299();

/*
  Method forward declaration
*/
au_int64_t meth_300();

/*
  Method forward declaration
*/
au_int64_t meth_301();

/*
  Method forward declaration
*/
au_index_t meth_302();

/*
  Method forward declaration
*/
au_index_t meth_303();

/*
  Method
*/
au_bool_t meth_284() {
    return false;
}

/*
  Method
*/
au_bool_t meth_285() {
    return true;
}

/*
  Method
*/
au_nat8_t meth_286() {
    return 0;
}

/*
  Method
*/
au_nat8_t meth_287() {
    return Austral__Pervasive____maximum_nat8;
}

/*
  Method
*/
au_nat16_t meth_288() {
    return 0;
}

/*
  Method
*/
au_nat16_t meth_289() {
    return Austral__Pervasive____maximum_nat16;
}

/*
  Method
*/
au_nat32_t meth_290() {
    return 0;
}

/*
  Method
*/
au_nat32_t meth_291() {
    return Austral__Pervasive____maximum_nat32;
}

/*
  Method
*/
au_nat64_t meth_292() {
    return 0;
}

/*
  Method
*/
au_nat64_t meth_293() {
    return Austral__Pervasive____maximum_nat64;
}

/*
  Method
*/
au_int8_t meth_294() {
    return Austral__Pervasive____minimum_int8;
}

/*
  Method
*/
au_int8_t meth_295() {
    return Austral__Pervasive____maximum_int8;
}

/*
  Method
*/
au_int16_t meth_296() {
    return Austral__Pervasive____minimum_int16;
}

/*
  Method
*/
au_int16_t meth_297() {
    return Austral__Pervasive____maximum_int16;
}

/*
  Method
*/
au_int32_t meth_298() {
    return Austral__Pervasive____minimum_int32;
}

/*
  Method
*/
au_int32_t meth_299() {
    return Austral__Pervasive____maximum_int32;
}

/*
  Method
*/
au_int64_t meth_300() {
    return Austral__Pervasive____minimum_int64;
}

/*
  Method
*/
au_int64_t meth_301() {
    return Austral__Pervasive____maximum_int64;
}

/*
  Method
*/
au_index_t meth_302() {
    return Austral__Pervasive____minimum_index;
}

/*
  Method
*/
au_index_t meth_303() {
    return Austral__Pervasive____maximum_index;
}
/* --- END translation unit for module 'Standard.Bounded' --- */

/* --- BEGIN translation unit for module 'Standard.Equality' --- */
/*
  Method forward declaration
*/
au_bool_t meth_304(au_unit_t* a, au_unit_t* b);

/*
  Method forward declaration
*/
au_bool_t meth_305(au_unit_t* a, au_unit_t* b);

/*
  Method
*/
au_bool_t meth_304(au_unit_t* a, au_unit_t* b) {
    return true;
}

/*
  Method
*/
au_bool_t meth_305(au_unit_t* a, au_unit_t* b) {
    return true;
}
/* --- END translation unit for module 'Standard.Equality' --- */

/* --- BEGIN translation unit for module 'Standard.Order' --- */
/*
  Union monomorph tag enum: Ordering[]
*/
typedef enum {
    mono_13_tag_GreaterThan, mono_13_tag_Equal, mono_13_tag_LessThan
} mono_13_tag;

/*
  Union monomorph tag enum: PartialOrdering[]
*/
typedef enum {
    mono_14_tag_Incomparable, mono_14_tag_Comparable
} mono_14_tag;

/*
  Union monomorph: Ordering[]
*/
struct mono_13;
typedef struct mono_13 mono_13;

/*
  Union monomorph: PartialOrdering[]
*/
struct mono_14;
typedef struct mono_14 mono_14;

/*
  Union monomorph: Ordering[]
*/
typedef struct mono_13 {mono_13_tag tag;union { struct  {} GreaterThan;struct  {} Equal;struct  {} LessThan;} data;} mono_13;

/*
  Union monomorph: PartialOrdering[]
*/
typedef struct mono_14 {mono_14_tag tag;union { struct  {} Incomparable;struct  {mono_13 order;} Comparable;} data;} mono_14;

/*
  Method forward declaration
*/
mono_14 meth_306(au_nat8_t* a, au_nat8_t* b);

/*
  Method forward declaration
*/
mono_13 meth_307(au_nat8_t* a, au_nat8_t* b);

/*
  Method
*/
mono_14 meth_306(au_nat8_t* a, au_nat8_t* b) {
    return ((mono_14) { .tag = mono_14_tag_Comparable, .data = { .Comparable = { .order = meth_307(a, b) } } });
}

/*
  Method
*/
mono_13 meth_307(au_nat8_t* a, au_nat8_t* b) {
    au_nat8_t av;
    av = *a;
    au_nat8_t bv;
    bv = *b;
    au_bool_t _t128 = (av == bv);
    if (_t128) {
        return ((mono_13) { .tag = mono_13_tag_Equal, .data = { .Equal = {  } } });
    } else {
        au_bool_t _t129 = (av > bv);
        if (_t129) {
            return ((mono_13) { .tag = mono_13_tag_GreaterThan, .data = { .GreaterThan = {  } } });
        } else {
            return ((mono_13) { .tag = mono_13_tag_LessThan, .data = { .LessThan = {  } } });
        }
    }
}
/* --- END translation unit for module 'Standard.Order' --- */

/* --- BEGIN translation unit for module 'Standard.Box' --- */

/* --- END translation unit for module 'Standard.Box' --- */

/* --- BEGIN translation unit for module 'Standard.Buffer' --- */
/*
  Constant
*/
#define Standard__Buffer____minimum_capacity 16

/*
  Constant
*/
#define Standard__Buffer____growth_factor 2

/*
  Function forward declaratioon
*/
au_index_t decl_354(au_index_t a, au_index_t b);

/*
  Function
*/
au_index_t decl_354(au_index_t a, au_index_t b) {
    au_bool_t _t170 = (a > b);
    if (_t170) {
        return a;
    } else {
        return b;
    }
}
/* --- END translation unit for module 'Standard.Buffer' --- */

/* --- BEGIN translation unit for module 'Standard.String' --- */
/*
  Union monomorph tag enum: Option[(MonoType.MonoNamedType (Id.MonoId 15))]
*/
typedef enum {
    mono_22_tag_Some, mono_22_tag_None
} mono_22_tag;

/*
  Union monomorph tag enum: Option[(MonoType.MonoPointer (MonoType.MonoInteger (Type.Unsigned, Type.Width8)))]
*/
typedef enum {
    mono_28_tag_Some, mono_28_tag_None
} mono_28_tag;

/*
  Record monomorph: Buffer[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
struct mono_15;
typedef struct mono_15 mono_15;

/*
  Record monomorph: String[]
*/
struct mono_16;
typedef struct mono_16 mono_16;

/*
  Union monomorph: Option[(MonoType.MonoNamedType (Id.MonoId 15))]
*/
struct mono_22;
typedef struct mono_22 mono_22;

/*
  Union monomorph: Option[(MonoType.MonoPointer (MonoType.MonoInteger (Type.Unsigned, Type.Width8)))]
*/
struct mono_28;
typedef struct mono_28 mono_28;

/*
  Record monomorph: Buffer[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
typedef struct mono_15 {au_index_t capacity;au_index_t size;au_nat8_t* array;} mono_15;

/*
  Record monomorph: String[]
*/
typedef struct mono_16 {mono_15 buf;} mono_16;

/*
  Union monomorph: Option[(MonoType.MonoPointer (MonoType.MonoInteger (Type.Unsigned, Type.Width8)))]
*/
typedef struct mono_28 {mono_28_tag tag;union { struct  {au_nat8_t* value;} Some;struct  {} None;} data;} mono_28;

/*
  Union monomorph: Option[(MonoType.MonoNamedType (Id.MonoId 15))]
*/
typedef struct mono_22 {mono_22_tag tag;union { struct  {mono_15 value;} Some;struct  {} None;} data;} mono_22;

/*
  Function forward declaratioon
*/
mono_16 decl_360();

/*
  Function forward declaratioon
*/
mono_16 decl_361(au_index_t size, au_nat8_t initial);

/*
  Function forward declaratioon
*/
au_unit_t decl_362(mono_16 str);

/*
  Function forward declaratioon
*/
mono_16 decl_363(au_span_t arr);

/*
  Function forward declaratioon
*/
mono_16 decl_364(mono_15 buffer);

/*
  Function monomorph forward declaration
*/
mono_15 mono_17();

/*
  Function monomorph forward declaration
*/
mono_15 mono_18(au_index_t size, au_nat8_t initialElement);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_19(mono_15 buffer);

/*
  Function monomorph forward declaration
*/
au_index_t mono_20(au_span_t sp);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_21(mono_16* string, au_index_t pos, au_nat8_t byte);

/*
  Function monomorph forward declaration
*/
mono_22 mono_23();

/*
  Function monomorph forward declaration
*/
mono_22 mono_24(au_index_t size, au_nat8_t initialElement);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_25(au_nat8_t* pointer);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_26(mono_15* buf, au_index_t pos, au_nat8_t element);

/*
  Function monomorph forward declaration
*/
au_nat8_t* mono_27(au_index_t count);

/*
  Function monomorph forward declaration
*/
mono_28 mono_29(au_nat8_t* address);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_30(mono_15* buf, au_nat8_t element);

/*
  Function monomorph forward declaration
*/
au_nat8_t* mono_31(au_nat8_t* pointer, au_index_t offset);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_32(au_nat8_t* pointer, au_nat8_t value);

/*
  Function monomorph forward declaration
*/
au_nat8_t* mono_33();

/*
  Function
*/
mono_16 decl_360() {
    mono_15 buf;
    buf = mono_17();
    return ((mono_16) { .buf = buf });
}

/*
  Function
*/
mono_16 decl_361(au_index_t size, au_nat8_t initial) {
    mono_15 buf;
    buf = mono_18(((au_index_t) size), ((au_nat8_t) initial));
    return ((mono_16) { .buf = buf });
}

/*
  Function
*/
au_unit_t decl_362(mono_16 str) {
    mono_16 tmp1 = str;
    mono_15 buf = (tmp1).buf;
    mono_19(buf);
    return false;
}

/*
  Function
*/
mono_16 decl_363(au_span_t arr) {
    au_index_t size;
    size = mono_20(arr);
    mono_16 str;
    str = decl_361(((au_index_t) size), ((au_nat8_t) 32));
    au_bool_t _t174 = (size > 0);
    if (_t174) {
        au_index_t _t175 = ((au_index_t) 0);
        au_index_t _t176 = meth_34(((au_index_t) size), ((au_index_t) 1));
        {
            size_t i = _t175;
            for (; i <= _t176; i++) {
                mono_16* str_tmp_ref64 = &(str);
                au_span_t* arr_tmp_ref65 = &(arr);
                mono_21(str_tmp_ref64, ((au_index_t) i), ((au_nat8_t) *((au_nat8_t*) au_array_index(arr_tmp_ref65, i, sizeof(au_nat8_t)))));
            }
        }
    } else {
    }
    return str;
}

/*
  Function
*/
mono_16 decl_364(mono_15 buffer) {
    return ((mono_16) { .buf = buffer });
}

/*
  Function monomorph: allocateEmpty[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
mono_15 mono_17() {
    mono_22 opt;
    opt = mono_23();
    mono_22 _t131 = opt;
    mono_22 tmp2 = _t131;
    switch ((tmp2).tag) {
        case mono_22_tag_Some:
            {
                mono_15 value = (((tmp2).data).Some).value;
                return value;
            }
            break;
        case mono_22_tag_None:
            {
                decl_9(au_make_span_from_string("allocateEmpty: allocation failed", 32));
            }
            break;
    }
}

/*
  Function monomorph: initialize[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
mono_15 mono_18(au_index_t size, au_nat8_t initialElement) {
    mono_22 opt;
    opt = mono_24(((au_index_t) size), initialElement);
    mono_22 _t133 = opt;
    mono_22 tmp3 = _t133;
    switch ((tmp3).tag) {
        case mono_22_tag_Some:
            {
                mono_15 value = (((tmp3).data).Some).value;
                return value;
            }
            break;
        case mono_22_tag_None:
            {
                decl_9(au_make_span_from_string("initialize: allocation failed", 29));
            }
            break;
    }
}

/*
  Function monomorph: destroyFree[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_unit_t mono_19(mono_15 buffer) {
    mono_15 tmp4 = buffer;
    au_index_t capacity = (tmp4).capacity;
    au_index_t size = (tmp4).size;
    au_nat8_t* array = (tmp4).array;
    mono_25(array);
    return false;
}

/*
  Function monomorph: spanLength[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_index_t mono_20(au_span_t sp) {
    return ((au_index_t)(sp.size));
}

/*
  Function monomorph: storeNth[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_21(mono_16* string, au_index_t pos, au_nat8_t byte) {
    return mono_26(&((*string).buf), ((au_index_t) pos), ((au_nat8_t) byte));
}

/*
  Function monomorph: safeAllocateEmpty[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
mono_22 mono_23() {
    au_nat8_t* addr;
    addr = mono_27(((au_index_t) Standard__Buffer____minimum_capacity));
    mono_28 _t132 = mono_29(addr);
    mono_28 tmp5 = _t132;
    switch ((tmp5).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* value = (((tmp5).data).Some).value;
                mono_15 buf;
                buf = ((mono_15) { .capacity = Standard__Buffer____minimum_capacity, .size = 0, .array = value });
                return ((mono_22) { .tag = mono_22_tag_Some, .data = { .Some = { .value = buf } } });
            }
            break;
        case mono_28_tag_None:
            {
                return ((mono_22) { .tag = mono_22_tag_None, .data = { .None = {  } } });
            }
            break;
    }
}

/*
  Function monomorph: safeInitialize[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
mono_22 mono_24(au_index_t size, au_nat8_t initialElement) {
    au_bool_t _t135 = (size == 0);
    if (_t135) {
        return mono_23();
    } else {
    }
    au_index_t capacity;
    capacity = decl_354(((au_index_t) size), ((au_index_t) Standard__Buffer____minimum_capacity));
    au_nat8_t* addr;
    addr = mono_27(((au_index_t) capacity));
    mono_28 _t134 = mono_29(addr);
    mono_28 tmp6 = _t134;
    switch ((tmp6).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* value = (((tmp6).data).Some).value;
                mono_15 buf;
                buf = ((mono_15) { .capacity = capacity, .size = size, .array = value });
                mono_15* buf_tmp_ref4 = &(buf);
                mono_30(buf_tmp_ref4, initialElement);
                return ((mono_22) { .tag = mono_22_tag_Some, .data = { .Some = { .value = buf } } });
            }
            break;
        case mono_28_tag_None:
            {
                return ((mono_22) { .tag = mono_22_tag_None, .data = { .None = {  } } });
            }
            break;
    }
}

/*
  Function monomorph: deallocate[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_unit_t mono_25(au_nat8_t* pointer) {
    ((au_unit_t)(au_free(pointer)));
    return false;
}

/*
  Function monomorph: storeNth[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_unit_t mono_26(mono_15* buf, au_index_t pos, au_nat8_t element) {
    mono_15** buf_tmp_ref8 = &(buf);
    au_bool_t _t138 = (pos >= *&((*buf_tmp_ref8)->size));
    if (_t138) {
        decl_9(au_make_span_from_string("storeNth: index out of range", 28));
    } else {
    }
    au_nat8_t* ptr;
    mono_15** buf_tmp_ref9 = &(buf);
    ptr = mono_31(*&((*buf_tmp_ref9)->array), ((au_index_t) pos));
    mono_32(ptr, element);
    return false;
}

/*
  Function monomorph: allocateBuffer[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_nat8_t* mono_27(au_index_t count) {
    au_bool_t _t126 = (count == 0);
    if (_t126) {
        decl_9(au_make_span_from_string("allocate called with count of zero", 34));
    } else {
    }
    return ((au_nat8_t*)(au_calloc(sizeof(au_nat8_t), count)));
}

/*
  Function monomorph: nullCheck[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
mono_28 mono_29(au_nat8_t* address) {
    au_nat8_t* n;
    n = mono_33();
    au_bool_t _t125 = (address != n);
    if (_t125) {
        au_nat8_t* ptr;
        ptr = ((au_nat8_t*)(address));
        mono_28 opt;
        opt = ((mono_28) { .tag = mono_28_tag_Some, .data = { .Some = { .value = ptr } } });
        return opt;
    } else {
        mono_28 res;
        res = ((mono_28) { .tag = mono_28_tag_None, .data = { .None = {  } } });
        return res;
    }
}

/*
  Function monomorph: fill[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_unit_t mono_30(mono_15* buf, au_nat8_t element) {
    au_index_t _t143 = ((au_index_t) 0);
    mono_15** buf_tmp_ref18 = &(buf);
    au_index_t _t144 = meth_34(((au_index_t) *&((*buf_tmp_ref18)->size)), ((au_index_t) 1));
    {
        size_t i = _t143;
        for (; i <= _t144; i++) {
            mono_15* buf_tmp_ref19 = buf;
            mono_26(buf_tmp_ref19, ((au_index_t) i), element);
        }
    }
    return false;
}

/*
  Function monomorph: positiveOffset[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_nat8_t* mono_31(au_nat8_t* pointer, au_index_t offset) {
    return ((au_nat8_t*)(pointer + offset));
}

/*
  Function monomorph: store[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_unit_t mono_32(au_nat8_t* pointer, au_nat8_t value) {
    ((au_unit_t)(AU_STORE(pointer, value)));
    return false;
}

/*
  Function monomorph: nullPointer[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_nat8_t* mono_33() {
    return ((au_nat8_t*)(NULL));
}
/* --- END translation unit for module 'Standard.String' --- */

/* --- BEGIN translation unit for module 'Standard.StringBuilder' --- */
/*
  Record monomorph: StringBuilder[]
*/
struct mono_34;
typedef struct mono_34 mono_34;

/*
  Record monomorph: StringBuilder[]
*/
typedef struct mono_34 {mono_15 buffer;} mono_34;

/*
  Function forward declaratioon
*/
mono_34 decl_371();

/*
  Function forward declaratioon
*/
mono_16 decl_374(mono_34 builder);

/*
  Function
*/
mono_34 decl_371() {
    mono_15 buffer;
    buffer = mono_17();
    return ((mono_34) { .buffer = buffer });
}

/*
  Function
*/
mono_16 decl_374(mono_34 builder) {
    mono_34 tmp7 = builder;
    mono_15 buffer = (tmp7).buffer;
    return decl_364(buffer);
}
/* --- END translation unit for module 'Standard.StringBuilder' --- */

/* --- BEGIN translation unit for module 'Standard.IO' --- */
/*
  Record monomorph: TerminalCapability[]
*/
struct mono_35;
typedef struct mono_35 mono_35;

/*
  Record monomorph: TerminalCapability[]
*/
typedef struct mono_35 {} mono_35;

/*
  Function forward declaratioon
*/
au_unit_t decl_377(mono_35 term);

/*
  Function
*/
au_unit_t decl_377(mono_35 term) {
    mono_35 tmp8 = term;
    return false;
}
/* --- END translation unit for module 'Standard.IO' --- */

/* --- BEGIN translation unit for module 'Standard.IO.Terminal' --- */
/*
  Record monomorph: StandardOutput[]
*/
struct mono_36;
typedef struct mono_36 mono_36;

/*
  Record monomorph: StandardError[]
*/
struct mono_37;
typedef struct mono_37 mono_37;

/*
  Record monomorph: StandardInput[]
*/
struct mono_38;
typedef struct mono_38 mono_38;

/*
  Record monomorph: StandardOutput[]
*/
typedef struct mono_36 {} mono_36;

/*
  Record monomorph: StandardError[]
*/
typedef struct mono_37 {} mono_37;

/*
  Record monomorph: StandardInput[]
*/
typedef struct mono_38 {} mono_38;

/*
  Constant
*/
#define Standard__IO__Terminal____EOF -1

/*
  Function forward declaratioon
*/
au_unit_t decl_388(mono_36 stream);

/*
  Function forward declaratioon
*/
au_unit_t decl_389(mono_37 stream);

/*
  Function forward declaratioon
*/
au_unit_t decl_390(mono_38 stream);

/*
  Method forward declaration
*/
au_unit_t meth_308(mono_36* stream, au_nat8_t byte);

/*
  Method forward declaration
*/
au_unit_t meth_309(mono_37* stream, au_nat8_t byte);

/*
  Method forward declaration
*/
mono_2 meth_310(mono_38* stream);

/*
  Function forward declaratioon
*/
au_nat8_t* decl_396();

/*
  Function forward declaratioon
*/
au_nat8_t* decl_397();

/*
  Function forward declaratioon
*/
au_nat8_t* decl_398();

/*
  Foreign function forward declaration
*/
au_int32_t decl_399(au_int32_t c, au_nat8_t* stream);

/*
  Foreign function forward declaration
*/
au_int32_t decl_400(au_nat8_t* stream);

/*
  Function
*/
au_unit_t decl_388(mono_36 stream) {
    mono_36 tmp9 = stream;
    return false;
}

/*
  Function
*/
au_unit_t decl_389(mono_37 stream) {
    mono_37 tmp10 = stream;
    return false;
}

/*
  Function
*/
au_unit_t decl_390(mono_38 stream) {
    mono_38 tmp11 = stream;
    return false;
}

/*
  Method
*/
au_unit_t meth_308(mono_36* stream, au_nat8_t byte) {
    au_nat8_t* stdout;
    stdout = decl_396();
    mono_8 _t179 = meth_220(byte);
    mono_8 tmp12 = _t179;
    switch ((tmp12).tag) {
        case mono_8_tag_Some:
            {
                au_int32_t c = (((tmp12).data).Some).value;
                decl_399(((au_int32_t) c), stdout);
            }
            break;
        case mono_8_tag_None:
            {
            }
            break;
    }
    return false;
}

/*
  Method
*/
au_unit_t meth_309(mono_37* stream, au_nat8_t byte) {
    au_nat8_t* stderr;
    stderr = decl_397();
    mono_8 _t180 = meth_220(byte);
    mono_8 tmp13 = _t180;
    switch ((tmp13).tag) {
        case mono_8_tag_Some:
            {
                au_int32_t c = (((tmp13).data).Some).value;
                decl_399(((au_int32_t) c), stderr);
            }
            break;
        case mono_8_tag_None:
            {
            }
            break;
    }
    return false;
}

/*
  Method
*/
mono_2 meth_310(mono_38* stream) {
    au_nat8_t* stdin;
    stdin = decl_398();
    au_int32_t res;
    res = decl_400(stdin);
    au_bool_t _t181 = (res == Standard__IO__Terminal____EOF);
    if (_t181) {
        return ((mono_2) { .tag = mono_2_tag_None, .data = { .None = {  } } });
    } else {
        return meth_160(res);
    }
}

/*
  Function
*/
au_nat8_t* decl_396() {
    return ((au_nat8_t*)(au_stdout()));
}

/*
  Function
*/
au_nat8_t* decl_397() {
    return ((au_nat8_t*)(au_stderr()));
}

/*
  Function
*/
au_nat8_t* decl_398() {
    return ((au_nat8_t*)(au_stdin()));
}

/*
  Foreign function
*/
au_int32_t decl_399(au_int32_t c, au_nat8_t* stream) {
    extern au_int32_t fputc(au_int32_t c, au_nat8_t* stream);
    return fputc(c, stream);
}

/*
  Foreign function
*/
au_int32_t decl_400(au_nat8_t* stream) {
    extern au_int32_t fgetc(au_nat8_t* stream);
    return fgetc(stream);
}
/* --- END translation unit for module 'Standard.IO.Terminal' --- */

/* --- BEGIN translation unit for module 'BfetchAust.Posix' --- */
/*
  Constant
*/
#define BfetchAust__Posix____o_rdonly 0

/*
  Constant
*/
#define BfetchAust__Posix____seek_set 0

/*
  Constant
*/
#define BfetchAust__Posix____seek_end 2

/*
  Constant
*/
#define BfetchAust__Posix____f_ok 0

/*
  Constant
*/
#define BfetchAust__Posix____prot_read 1

/*
  Constant
*/
#define BfetchAust__Posix____map_private 2

/*
  Constant
*/
#define BfetchAust__Posix____stdout_fd 1

/*
  Constant
*/
#define BfetchAust__Posix____dirent_reclen_offset 16

/*
  Constant
*/
#define BfetchAust__Posix____dirent_type_offset 18

/*
  Constant
*/
#define BfetchAust__Posix____dirent_name_offset 19

/*
  Constant
*/
#define BfetchAust__Posix____utsname_size 390

/*
  Constant
*/
#define BfetchAust__Posix____utsname_release_offset 130

/*
  Constant
*/
#define BfetchAust__Posix____utsname_machine_offset 260

/*
  Foreign function forward declaration
*/
au_int32_t decl_414(au_span_t path, au_int32_t flags, au_int32_t mode);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_415(au_span_t path, au_span_t mode);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_416(au_nat8_t* path, au_span_t mode);

/*
  Foreign function forward declaration
*/
au_index_t decl_417(au_nat8_t* ptr, au_index_t size, au_index_t nmemb, au_nat8_t* stream);

/*
  Foreign function forward declaration
*/
au_int32_t decl_418(au_nat8_t* stream);

/*
  Foreign function forward declaration
*/
au_int32_t decl_419(au_nat8_t* stream);

/*
  Foreign function forward declaration
*/
au_int64_t decl_420(au_int32_t fd, au_nat8_t* buf, au_index_t count);

/*
  Foreign function forward declaration
*/
au_int64_t decl_421(au_int32_t fd, au_nat8_t* buf, au_index_t count);

/*
  Foreign function forward declaration
*/
au_int32_t decl_422(au_int32_t fd);

/*
  Foreign function forward declaration
*/
au_int32_t decl_423(au_span_t path, au_int32_t mode);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_424(au_span_t name);

/*
  Foreign function forward declaration
*/
au_int32_t decl_425();

/*
  Foreign function forward declaration
*/
au_int64_t decl_426(au_span_t path, au_nat8_t* buf, au_index_t count);

/*
  Foreign function forward declaration
*/
au_int64_t decl_427(au_nat8_t* path, au_nat8_t* buf, au_index_t count);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_428(au_span_t path);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_429(au_nat8_t* path);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_430(au_nat8_t* dir);

/*
  Foreign function forward declaration
*/
au_int32_t decl_431(au_nat8_t* dir);

/*
  Foreign function forward declaration
*/
au_int64_t decl_432(au_int32_t fd, au_int64_t offset, au_int32_t whence);

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_433(au_nat8_t* addr, au_index_t length, au_int32_t prot, au_int32_t flags, au_int32_t fd, au_int64_t offset);

/*
  Foreign function forward declaration
*/
au_int32_t decl_434(au_nat8_t* addr, au_index_t length);

/*
  Foreign function forward declaration
*/
au_int32_t decl_435(au_nat8_t* buf);

/*
  Foreign function forward declaration
*/
au_int32_t decl_436();

/*
  Foreign function forward declaration
*/
au_nat8_t* decl_437(au_nat8_t* haystack, au_index_t haystack_len, au_nat8_t* needle, au_index_t needle_len);

/*
  Foreign function forward declaration
*/
au_int32_t decl_438(au_nat8_t* lhs, au_nat8_t* rhs, au_index_t count);

/*
  Function forward declaratioon
*/
au_int32_t decl_439(au_span_t path);

/*
  Function forward declaratioon
*/
au_nat8_t decl_440(au_nat8_t* ent);

/*
  Function forward declaratioon
*/
au_nat8_t* decl_441(au_nat8_t* ent);

/*
  Function forward declaratioon
*/
au_nat16_t decl_442(au_nat8_t* ent);

/*
  Function forward declaratioon
*/
au_nat8_t* decl_443(au_int32_t fd, au_index_t length);

/*
  Function forward declaratioon
*/
au_bool_t decl_444(au_nat8_t* addr);

/*
  Function monomorph forward declaration
*/
au_nat8_t mono_39(au_nat8_t* pointer);

/*
  Function monomorph forward declaration
*/
au_nat16_t* mono_40(au_nat8_t* ptr);

/*
  Function monomorph forward declaration
*/
au_nat16_t mono_41(au_nat16_t* pointer);

/*
  Foreign function
*/
au_int32_t decl_414(au_span_t path, au_int32_t flags, au_int32_t mode) {
    extern au_int32_t open(au_nat8_t* path, au_int32_t flags, au_int32_t mode);
    return open((path).data, flags, mode);
}

/*
  Foreign function
*/
au_nat8_t* decl_415(au_span_t path, au_span_t mode) {
    extern au_nat8_t* fopen(au_nat8_t* path, au_nat8_t* mode);
    return fopen((path).data, (mode).data);
}

/*
  Foreign function
*/
au_nat8_t* decl_416(au_nat8_t* path, au_span_t mode) {
    extern au_nat8_t* fopen(au_nat8_t* path, au_nat8_t* mode);
    return fopen(path, (mode).data);
}

/*
  Foreign function
*/
au_index_t decl_417(au_nat8_t* ptr, au_index_t size, au_index_t nmemb, au_nat8_t* stream) {
    extern au_index_t fread(au_nat8_t* ptr, au_index_t size, au_index_t nmemb, au_nat8_t* stream);
    return fread(ptr, size, nmemb, stream);
}

/*
  Foreign function
*/
au_int32_t decl_418(au_nat8_t* stream) {
    extern au_int32_t fclose(au_nat8_t* stream);
    return fclose(stream);
}

/*
  Foreign function
*/
au_int32_t decl_419(au_nat8_t* stream) {
    extern au_int32_t fileno(au_nat8_t* stream);
    return fileno(stream);
}

/*
  Foreign function
*/
au_int64_t decl_420(au_int32_t fd, au_nat8_t* buf, au_index_t count) {
    extern au_int64_t read(au_int32_t fd, au_nat8_t* buf, au_index_t count);
    return read(fd, buf, count);
}

/*
  Foreign function
*/
au_int64_t decl_421(au_int32_t fd, au_nat8_t* buf, au_index_t count) {
    extern au_int64_t write(au_int32_t fd, au_nat8_t* buf, au_index_t count);
    return write(fd, buf, count);
}

/*
  Foreign function
*/
au_int32_t decl_422(au_int32_t fd) {
    extern au_int32_t close(au_int32_t fd);
    return close(fd);
}

/*
  Foreign function
*/
au_int32_t decl_423(au_span_t path, au_int32_t mode) {
    extern au_int32_t access(au_nat8_t* path, au_int32_t mode);
    return access((path).data, mode);
}

/*
  Foreign function
*/
au_nat8_t* decl_424(au_span_t name) {
    extern au_nat8_t* getenv(au_nat8_t* name);
    return getenv((name).data);
}

/*
  Foreign function
*/
au_int32_t decl_425() {
    extern au_int32_t getppid();
    return getppid();
}

/*
  Foreign function
*/
au_int64_t decl_426(au_span_t path, au_nat8_t* buf, au_index_t count) {
    extern au_int64_t readlink(au_nat8_t* path, au_nat8_t* buf, au_index_t count);
    return readlink((path).data, buf, count);
}

/*
  Foreign function
*/
au_int64_t decl_427(au_nat8_t* path, au_nat8_t* buf, au_index_t count) {
    extern au_int64_t readlink(au_nat8_t* path, au_nat8_t* buf, au_index_t count);
    return readlink(path, buf, count);
}

/*
  Foreign function
*/
au_nat8_t* decl_428(au_span_t path) {
    extern au_nat8_t* opendir(au_nat8_t* path);
    return opendir((path).data);
}

/*
  Foreign function
*/
au_nat8_t* decl_429(au_nat8_t* path) {
    extern au_nat8_t* opendir(au_nat8_t* path);
    return opendir(path);
}

/*
  Foreign function
*/
au_nat8_t* decl_430(au_nat8_t* dir) {
    extern au_nat8_t* readdir(au_nat8_t* dir);
    return readdir(dir);
}

/*
  Foreign function
*/
au_int32_t decl_431(au_nat8_t* dir) {
    extern au_int32_t closedir(au_nat8_t* dir);
    return closedir(dir);
}

/*
  Foreign function
*/
au_int64_t decl_432(au_int32_t fd, au_int64_t offset, au_int32_t whence) {
    extern au_int64_t lseek(au_int32_t fd, au_int64_t offset, au_int32_t whence);
    return lseek(fd, offset, whence);
}

/*
  Foreign function
*/
au_nat8_t* decl_433(au_nat8_t* addr, au_index_t length, au_int32_t prot, au_int32_t flags, au_int32_t fd, au_int64_t offset) {
    extern au_nat8_t* mmap(au_nat8_t* addr, au_index_t length, au_int32_t prot, au_int32_t flags, au_int32_t fd, au_int64_t offset);
    return mmap(addr, length, prot, flags, fd, offset);
}

/*
  Foreign function
*/
au_int32_t decl_434(au_nat8_t* addr, au_index_t length) {
    extern au_int32_t munmap(au_nat8_t* addr, au_index_t length);
    return munmap(addr, length);
}

/*
  Foreign function
*/
au_int32_t decl_435(au_nat8_t* buf) {
    extern au_int32_t uname(au_nat8_t* buf);
    return uname(buf);
}

/*
  Foreign function
*/
au_int32_t decl_436() {
    extern au_int32_t get_nprocs();
    return get_nprocs();
}

/*
  Foreign function
*/
au_nat8_t* decl_437(au_nat8_t* haystack, au_index_t haystack_len, au_nat8_t* needle, au_index_t needle_len) {
    extern au_nat8_t* memmem(au_nat8_t* haystack, au_index_t haystack_len, au_nat8_t* needle, au_index_t needle_len);
    return memmem(haystack, haystack_len, needle, needle_len);
}

/*
  Foreign function
*/
au_int32_t decl_438(au_nat8_t* lhs, au_nat8_t* rhs, au_index_t count) {
    extern au_int32_t strncasecmp(au_nat8_t* lhs, au_nat8_t* rhs, au_index_t count);
    return strncasecmp(lhs, rhs, count);
}

/*
  Function
*/
au_int32_t decl_439(au_span_t path) {
    return decl_414(path, ((au_int32_t) BfetchAust__Posix____o_rdonly), ((au_int32_t) 0));
}

/*
  Function
*/
au_nat8_t decl_440(au_nat8_t* ent) {
    return mono_39(mono_31(ent, ((au_index_t) BfetchAust__Posix____dirent_type_offset)));
}

/*
  Function
*/
au_nat8_t* decl_441(au_nat8_t* ent) {
    return mono_31(ent, ((au_index_t) BfetchAust__Posix____dirent_name_offset));
}

/*
  Function
*/
au_nat16_t decl_442(au_nat8_t* ent) {
    au_nat16_t* ptr;
    ptr = mono_40(mono_31(ent, ((au_index_t) BfetchAust__Posix____dirent_reclen_offset)));
    return mono_41(ptr);
}

/*
  Function
*/
au_nat8_t* decl_443(au_int32_t fd, au_index_t length) {
    au_nat8_t* addr;
    addr = mono_33();
    return decl_433(addr, ((au_index_t) length), ((au_int32_t) BfetchAust__Posix____prot_read), ((au_int32_t) BfetchAust__Posix____map_private), ((au_int32_t) fd), ((au_int64_t) 0));
}

/*
  Function
*/
au_bool_t decl_444(au_nat8_t* addr) {
    return ((au_bool_t)(addr == ((void*)-1)));
}

/*
  Function monomorph: load[(MonoType.MonoInteger (Type.Unsigned, Type.Width8))]
*/
au_nat8_t mono_39(au_nat8_t* pointer) {
    return ((au_nat8_t)(*(pointer)));
}

/*
  Function monomorph: pointerCast[(MonoType.MonoInteger (Type.Unsigned, Type.Width8)), (MonoType.MonoInteger (Type.Unsigned, Type.Width16))]
*/
au_nat16_t* mono_40(au_nat8_t* ptr) {
    return ((au_nat16_t*)(ptr));
}

/*
  Function monomorph: load[(MonoType.MonoInteger (Type.Unsigned, Type.Width16))]
*/
au_nat16_t mono_41(au_nat16_t* pointer) {
    return ((au_nat16_t)(*(pointer)));
}
/* --- END translation unit for module 'BfetchAust.Posix' --- */

/* --- BEGIN translation unit for module 'BfetchAust.FastIO' --- */
/*
  Record monomorph: ByteBuf[]
*/
struct mono_42;
typedef struct mono_42 mono_42;

/*
  Record monomorph: ByteBuf[]
*/
typedef struct mono_42 {au_index_t cap;au_index_t len;au_nat8_t* ptr;} mono_42;

/*
  Function forward declaratioon
*/
mono_42 decl_446(au_index_t cap0);

/*
  Function forward declaratioon
*/
au_unit_t decl_447(mono_42 buf);

/*
  Function forward declaratioon
*/
au_nat8_t decl_468(au_nat8_t value);

/*
  Function
*/
mono_42 decl_446(au_index_t cap0) {
    au_bool_t _t188 = (cap0 == ((au_index_t) 0));
    if (_t188) {
        decl_9(au_make_span_from_string("makeByteBuf: zero capacity", 26));
    } else {
    }
    au_nat8_t* addr;
    addr = mono_27(((au_index_t) cap0));
    mono_28 _t187 = mono_29(addr);
    mono_28 tmp14 = _t187;
    switch ((tmp14).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* ptr = (((tmp14).data).Some).value;
                return ((mono_42) { .cap = cap0, .len = ((au_index_t) 0), .ptr = ptr });
            }
            break;
        case mono_28_tag_None:
            {
                decl_9(au_make_span_from_string("makeByteBuf: allocation failed", 30));
            }
            break;
    }
}

/*
  Function
*/
au_unit_t decl_447(mono_42 buf) {
    mono_42 tmp15 = buf;
    au_index_t cap = (tmp15).cap;
    au_index_t len = (tmp15).len;
    au_nat8_t* ptr = (tmp15).ptr;
    mono_25(ptr);
    return false;
}

/*
  Function
*/
au_nat8_t decl_468(au_nat8_t value) {
    au_bool_t _t223 = (value < ((au_nat8_t) 10));
    if (_t223) {
        return meth_1(((au_nat8_t) value), ((au_nat8_t) ((au_nat8_t) 48)));
    } else {
        return meth_1(((au_nat8_t) value), ((au_nat8_t) ((au_nat8_t) 87)));
    }
}
/* --- END translation unit for module 'BfetchAust.FastIO' --- */

/* --- BEGIN translation unit for module 'BfetchAust.Sys' --- */
/*
  Union monomorph tag enum: SystemType[]
*/
typedef enum {
    mono_43_tag_OtherMode, mono_43_tag_CachyMode, mono_43_tag_GentooMode, mono_43_tag_BedrockMode
} mono_43_tag;

/*
  Union monomorph: SystemType[]
*/
struct mono_43;
typedef struct mono_43 mono_43;

/*
  Union monomorph: SystemType[]
*/
typedef struct mono_43 {mono_43_tag tag;union { struct  {} OtherMode;struct  {} CachyMode;struct  {} GentooMode;struct  {} BedrockMode;} data;} mono_43;

/*
  Function forward declaratioon
*/
au_nat8_t decl_485(au_nat8_t byte);

/*
  Function forward declaratioon
*/
au_index_t decl_486(au_nat8_t byte);

/*
  Function forward declaratioon
*/
au_bool_t decl_493(mono_43 ty);

/*
  Function forward declaratioon
*/
mono_10 decl_502(au_nat8_t byte);

/*
  Function forward declaratioon
*/
au_nat8_t decl_505(au_index_t value);

/*
  Function forward declaratioon
*/
au_nat32_t decl_506(au_index_t value);

/*
  Function forward declaratioon
*/
au_index_t decl_514(au_nat8_t* ptr);

/*
  Function forward declaratioon
*/
au_bool_t decl_515(au_nat8_t* ptr);

/*
  Function forward declaratioon
*/
au_bool_t decl_516(au_nat8_t* ptr, au_span_t lit);

/*
  Function forward declaratioon
*/
au_bool_t decl_517(au_nat8_t* ptr, au_span_t lit);

/*
  Function forward declaratioon
*/
au_bool_t decl_518(au_nat8_t* ptr);

/*
  Function forward declaratioon
*/
au_index_t decl_521(au_nat8_t* dir);

/*
  Function forward declaratioon
*/
au_index_t decl_522(au_span_t path);

/*
  Function forward declaratioon
*/
au_index_t decl_523(au_nat8_t* path);

/*
  Function forward declaratioon
*/
au_index_t decl_524();

/*
  Function forward declaratioon
*/
au_index_t decl_525(au_span_t path, au_span_t needle);

/*
  Function forward declaratioon
*/
au_index_t decl_526(au_nat8_t* path, au_span_t needle);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_44(au_span_t path, mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_index_t mono_45(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_46(mono_42* buf, au_index_t pos, au_span_t lit);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_47(au_nat8_t* path, mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_48(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_nat8_t mono_49(mono_42* buf, au_index_t pos);

/*
  Function
*/
au_nat8_t decl_485(au_nat8_t byte) {
    au_bool_t _t299 = ((byte >= 65) && (byte <= 90));
    if (_t299) {
        return meth_1(((au_nat8_t) byte), ((au_nat8_t) ((au_nat8_t) 32)));
    } else {
        return byte;
    }
}

/*
  Function
*/
au_index_t decl_486(au_nat8_t byte) {
    mono_10 _t300 = meth_242(meth_2(((au_nat8_t) byte), ((au_nat8_t) 48)));
    mono_10 tmp16 = _t300;
    switch ((tmp16).tag) {
        case mono_10_tag_Some:
            {
                au_index_t digit = (((tmp16).data).Some).value;
                return digit;
            }
            break;
        case mono_10_tag_None:
            {
                decl_9(au_make_span_from_string("digitIndex: conversion failed", 29));
            }
            break;
    }
}

/*
  Function
*/
au_bool_t decl_493(mono_43 ty) {
    mono_43 _t324 = ty;
    mono_43 tmp17 = _t324;
    switch ((tmp17).tag) {
        case mono_43_tag_OtherMode:
            {
                return true;
            }
            break;
        case mono_43_tag_CachyMode:
            {
                return false;
            }
            break;
        case mono_43_tag_GentooMode:
            {
                return false;
            }
            break;
        case mono_43_tag_BedrockMode:
            {
                return false;
            }
            break;
    }
}

/*
  Function
*/
mono_10 decl_502(au_nat8_t byte) {
    au_bool_t _t416 = ((byte >= 48) && (byte <= 57));
    if (_t416) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = decl_486(((au_nat8_t) byte)) } } });
    } else {
        au_bool_t _t417 = ((decl_485(((au_nat8_t) byte)) >= 97) && (decl_485(((au_nat8_t) byte)) <= 102));
        if (_t417) {
            mono_10 _t418 = meth_242(meth_2(((au_nat8_t) decl_485(((au_nat8_t) byte))), ((au_nat8_t) 97)));
            mono_10 tmp18 = _t418;
            switch ((tmp18).tag) {
                case mono_10_tag_Some:
                    {
                        au_index_t d = (((tmp18).data).Some).value;
                        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = meth_33(((au_index_t) ((au_index_t) 10)), ((au_index_t) d)) } } });
                    }
                    break;
                case mono_10_tag_None:
                    {
                        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
                    }
                    break;
            }
        } else {
            return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
        }
    }
}

/*
  Function
*/
au_nat8_t decl_505(au_index_t value) {
    au_bool_t _t430 = (value < ((au_index_t) 10));
    if (_t430) {
        mono_2 _t432 = meth_162(value);
        mono_2 tmp20 = _t432;
        switch ((tmp20).tag) {
            case mono_2_tag_Some:
                {
                    au_nat8_t digit = (((tmp20).data).Some).value;
                    return meth_1(((au_nat8_t) ((au_nat8_t) 48)), ((au_nat8_t) digit));
                }
                break;
            case mono_2_tag_None:
                {
                    decl_9(au_make_span_from_string("hex4Char: decimal conversion failed", 35));
                }
                break;
        }
    } else {
        mono_2 _t431 = meth_162(meth_34(((au_index_t) value), ((au_index_t) ((au_index_t) 10))));
        mono_2 tmp19 = _t431;
        switch ((tmp19).tag) {
            case mono_2_tag_Some:
                {
                    au_nat8_t digit = (((tmp19).data).Some).value;
                    return meth_1(((au_nat8_t) ((au_nat8_t) 97)), ((au_nat8_t) digit));
                }
                break;
            case mono_2_tag_None:
                {
                    decl_9(au_make_span_from_string("hex4Char: hex conversion failed", 31));
                }
                break;
        }
    }
}

/*
  Function
*/
au_nat32_t decl_506(au_index_t value) {
    mono_4 _t433 = meth_184(value);
    mono_4 tmp21 = _t433;
    switch ((tmp21).tag) {
        case mono_4_tag_Some:
            {
                au_nat32_t out = (((tmp21).data).Some).value;
                return out;
            }
            break;
        case mono_4_tag_None:
            {
                decl_9(au_make_span_from_string("indexToNat32: conversion failed", 31));
            }
            break;
    }
}

/*
  Function
*/
au_index_t decl_514(au_nat8_t* ptr) {
    au_index_t len;
    len = ((au_index_t) 0);
    au_bool_t _t515 = (mono_39(mono_31(ptr, ((au_index_t) len))) != ((au_nat8_t) 0));
    while (_t515) {
        au_index_t _t516 = meth_33(((au_index_t) len), ((au_index_t) ((au_index_t) 1)));
        len = _t516;
        _t515 = (mono_39(mono_31(ptr, ((au_index_t) len))) != ((au_nat8_t) 0));
    }
    return len;
}

/*
  Function
*/
au_bool_t decl_515(au_nat8_t* ptr) {
    return (mono_39(ptr) == 46);
}

/*
  Function
*/
au_bool_t decl_516(au_nat8_t* ptr, au_span_t lit) {
    au_index_t n;
    n = decl_514(ptr);
    au_bool_t _t521 = (n != mono_20(lit));
    if (_t521) {
        return false;
    } else {
    }
    au_bool_t _t517 = (n > ((au_index_t) 0));
    if (_t517) {
        au_index_t _t518 = ((au_index_t) 0);
        au_index_t _t519 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t518;
            for (; i <= _t519; i++) {
                au_span_t* lit_tmp_ref323 = &(lit);
                au_bool_t _t520 = (mono_39(mono_31(ptr, ((au_index_t) i))) != *((au_nat8_t*) au_array_index(lit_tmp_ref323, i, sizeof(au_nat8_t))));
                if (_t520) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function
*/
au_bool_t decl_517(au_nat8_t* ptr, au_span_t lit) {
    au_index_t n;
    n = decl_514(ptr);
    au_index_t m;
    m = mono_20(lit);
    au_bool_t _t526 = (m > n);
    if (_t526) {
        return false;
    } else {
    }
    au_bool_t _t522 = (m > ((au_index_t) 0));
    if (_t522) {
        au_index_t _t523 = ((au_index_t) 0);
        au_index_t _t524 = meth_34(((au_index_t) m), ((au_index_t) 1));
        {
            size_t i = _t523;
            for (; i <= _t524; i++) {
                au_span_t* lit_tmp_ref324 = &(lit);
                au_bool_t _t525 = (mono_39(mono_31(ptr, ((au_index_t) meth_33(((au_index_t) meth_34(((au_index_t) n), ((au_index_t) m))), ((au_index_t) i))))) != *((au_nat8_t*) au_array_index(lit_tmp_ref324, i, sizeof(au_nat8_t))));
                if (_t525) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function
*/
au_bool_t decl_518(au_nat8_t* ptr) {
    au_bool_t _t527 = decl_516(ptr, au_make_span_from_string("bash", 4));
    if (_t527) {
        return true;
    } else {
        au_bool_t _t528 = decl_516(ptr, au_make_span_from_string("zsh", 3));
        if (_t528) {
            return true;
        } else {
            au_bool_t _t529 = decl_516(ptr, au_make_span_from_string("fish", 4));
            if (_t529) {
                return true;
            } else {
                au_bool_t _t530 = decl_516(ptr, au_make_span_from_string("sh", 2));
                if (_t530) {
                    return true;
                } else {
                    return false;
                }
            }
        }
    }
}

/*
  Function
*/
au_index_t decl_521(au_nat8_t* dir) {
    au_index_t count;
    count = ((au_index_t) 0);
    au_bool_t reading;
    reading = true;
    au_bool_t _t549 = reading;
    while (_t549) {
        mono_28 _t550 = mono_29(decl_430(dir));
        mono_28 tmp22 = _t550;
        switch ((tmp22).tag) {
            case mono_28_tag_Some:
                {
                    au_nat8_t* ent = (((tmp22).data).Some).value;
                    au_bool_t _t551 = !(decl_515(decl_441(ent)));
                    if (_t551) {
                        au_index_t _t552 = meth_33(((au_index_t) count), ((au_index_t) ((au_index_t) 1)));
                        count = _t552;
                    } else {
                    }
                }
                break;
            case mono_28_tag_None:
                {
                    au_bool_t _t553 = false;
                    reading = _t553;
                }
                break;
        }
        _t549 = reading;
    }
    return count;
}

/*
  Function
*/
au_index_t decl_522(au_span_t path) {
    mono_28 _t554 = mono_29(decl_428(path));
    mono_28 tmp23 = _t554;
    switch ((tmp23).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* dir = (((tmp23).data).Some).value;
                au_index_t count;
                count = decl_521(dir);
                decl_431(dir);
                return count;
            }
            break;
        case mono_28_tag_None:
            {
                return ((au_index_t) 0);
            }
            break;
    }
}

/*
  Function
*/
au_index_t decl_523(au_nat8_t* path) {
    mono_28 _t555 = mono_29(decl_429(path));
    mono_28 tmp24 = _t555;
    switch ((tmp24).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* dir = (((tmp24).data).Some).value;
                au_index_t count;
                count = decl_521(dir);
                decl_431(dir);
                return count;
            }
            break;
        case mono_28_tag_None:
            {
                return ((au_index_t) 0);
            }
            break;
    }
}

/*
  Function
*/
au_index_t decl_524() {
    mono_28 _t556 = mono_29(decl_428(au_make_span_from_string("/var/lib/dpkg/info", 18)));
    mono_28 tmp25 = _t556;
    switch ((tmp25).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* dir = (((tmp25).data).Some).value;
                au_index_t count;
                count = ((au_index_t) 0);
                au_bool_t reading;
                reading = true;
                au_bool_t _t557 = reading;
                while (_t557) {
                    mono_28 _t558 = mono_29(decl_430(dir));
                    mono_28 tmp26 = _t558;
                    switch ((tmp26).tag) {
                        case mono_28_tag_Some:
                            {
                                au_nat8_t* ent = (((tmp26).data).Some).value;
                                au_nat8_t* name;
                                name = decl_441(ent);
                                au_bool_t _t559 = (!(decl_515(name)) && decl_517(name, au_make_span_from_string(".list", 5)));
                                if (_t559) {
                                    au_index_t _t560 = meth_33(((au_index_t) count), ((au_index_t) ((au_index_t) 1)));
                                    count = _t560;
                                } else {
                                }
                            }
                            break;
                        case mono_28_tag_None:
                            {
                                au_bool_t _t561 = false;
                                reading = _t561;
                            }
                            break;
                    }
                    _t557 = reading;
                }
                decl_431(dir);
                return count;
            }
            break;
        case mono_28_tag_None:
            {
                return ((au_index_t) 0);
            }
            break;
    }
}

/*
  Function
*/
au_index_t decl_525(au_span_t path, au_span_t needle) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 65536)));
    au_index_t count;
    count = ((au_index_t) 0);
    mono_42* raw_tmp_ref328 = &(raw);
    au_bool_t _t562 = mono_44(path, raw_tmp_ref328);
    if (_t562) {
        au_index_t n;
        n = mono_20(needle);
        mono_42* raw_tmp_ref329 = &(raw);
        au_bool_t _t563 = ((n > ((au_index_t) 0)) && (n <= mono_45(raw_tmp_ref329)));
        if (_t563) {
            au_index_t _t564 = ((au_index_t) 0);
            mono_42* raw_tmp_ref330 = &(raw);
            au_index_t _t565 = meth_34(((au_index_t) mono_45(raw_tmp_ref330)), ((au_index_t) n));
            {
                size_t pos = _t564;
                for (; pos <= _t565; pos++) {
                    mono_42* raw_tmp_ref331 = &(raw);
                    au_bool_t _t566 = mono_46(raw_tmp_ref331, ((au_index_t) pos), needle);
                    if (_t566) {
                        au_index_t _t567 = meth_33(((au_index_t) count), ((au_index_t) ((au_index_t) 1)));
                        count = _t567;
                    } else {
                    }
                }
            }
        } else {
        }
    } else {
    }
    decl_447(raw);
    return count;
}

/*
  Function
*/
au_index_t decl_526(au_nat8_t* path, au_span_t needle) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 65536)));
    au_index_t count;
    count = ((au_index_t) 0);
    mono_42* raw_tmp_ref332 = &(raw);
    au_bool_t _t568 = mono_47(path, raw_tmp_ref332);
    if (_t568) {
        au_index_t n;
        n = mono_20(needle);
        mono_42* raw_tmp_ref333 = &(raw);
        au_bool_t _t569 = ((n > ((au_index_t) 0)) && (n <= mono_45(raw_tmp_ref333)));
        if (_t569) {
            au_index_t _t570 = ((au_index_t) 0);
            mono_42* raw_tmp_ref334 = &(raw);
            au_index_t _t571 = meth_34(((au_index_t) mono_45(raw_tmp_ref334)), ((au_index_t) n));
            {
                size_t pos = _t570;
                for (; pos <= _t571; pos++) {
                    mono_42* raw_tmp_ref335 = &(raw);
                    au_bool_t _t572 = mono_46(raw_tmp_ref335, ((au_index_t) pos), needle);
                    if (_t572) {
                        au_index_t _t573 = meth_33(((au_index_t) count), ((au_index_t) ((au_index_t) 1)));
                        count = _t573;
                    } else {
                    }
                }
            }
        } else {
        }
    } else {
    }
    decl_447(raw);
    return count;
}

/*
  Function monomorph: readFileFast[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_44(au_span_t path, mono_42* buf) {
    mono_42* buf_tmp_ref119 = buf;
    mono_48(buf_tmp_ref119);
    mono_28 _t212 = mono_29(decl_415(path, au_make_span_from_string("rb", 2)));
    mono_28 tmp27 = _t212;
    switch ((tmp27).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* file = (((tmp27).data).Some).value;
                au_index_t bytes_read;
                bytes_read = ((au_index_t) 0);
                mono_42* ref = buf;
                mono_42** ref_tmp_ref120 = &(ref);
                mono_42** ref_tmp_ref121 = &(ref);
                au_index_t _t215 = decl_417(*&((*ref_tmp_ref120)->ptr), ((au_index_t) ((au_index_t) 1)), ((au_index_t) meth_34(((au_index_t) *&((*ref_tmp_ref121)->cap)), ((au_index_t) ((au_index_t) 1)))), file);
                bytes_read = _t215;
                decl_418(file);
                au_bool_t _t213 = (bytes_read == ((au_index_t) 0));
                if (_t213) {
                    return false;
                } else {
                    au_index_t _t214 = bytes_read;
                    mono_42** buf_tmp_ref122 = &(buf);
                    *&((*buf_tmp_ref122)->len) = _t214;
                    mono_42** buf_tmp_ref123 = &(buf);
                    mono_32(mono_31(*&((*buf_tmp_ref123)->ptr), ((au_index_t) bytes_read)), ((au_nat8_t) ((au_nat8_t) 0)));
                    return true;
                }
            }
            break;
        case mono_28_tag_None:
            {
                return false;
            }
            break;
    }
}

/*
  Function monomorph: length[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_index_t mono_45(mono_42* buf) {
    mono_42** buf_tmp_ref68 = &(buf);
    return *&((*buf_tmp_ref68)->len);
}

/*
  Function monomorph: matchesAt[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_46(mono_42* buf, au_index_t pos, au_span_t lit) {
    au_index_t n;
    n = mono_20(lit);
    au_bool_t _t305 = (meth_33(((au_index_t) pos), ((au_index_t) n)) > mono_45(buf));
    if (_t305) {
        return false;
    } else {
    }
    au_bool_t _t301 = (n > 0);
    if (_t301) {
        au_index_t _t302 = ((au_index_t) 0);
        au_index_t _t303 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t302;
            for (; i <= _t303; i++) {
                au_span_t* lit_tmp_ref268 = &(lit);
                au_bool_t _t304 = (mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) i)))) != *((au_nat8_t*) au_array_index(lit_tmp_ref268, i, sizeof(au_nat8_t))));
                if (_t304) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function monomorph: readFilePtrFast[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_47(au_nat8_t* path, mono_42* buf) {
    mono_42* buf_tmp_ref124 = buf;
    mono_48(buf_tmp_ref124);
    mono_28 _t216 = mono_29(decl_416(path, au_make_span_from_string("rb", 2)));
    mono_28 tmp28 = _t216;
    switch ((tmp28).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* file = (((tmp28).data).Some).value;
                au_index_t bytes_read;
                bytes_read = ((au_index_t) 0);
                mono_42* ref = buf;
                mono_42** ref_tmp_ref125 = &(ref);
                mono_42** ref_tmp_ref126 = &(ref);
                au_index_t _t219 = decl_417(*&((*ref_tmp_ref125)->ptr), ((au_index_t) ((au_index_t) 1)), ((au_index_t) meth_34(((au_index_t) *&((*ref_tmp_ref126)->cap)), ((au_index_t) ((au_index_t) 1)))), file);
                bytes_read = _t219;
                decl_418(file);
                au_bool_t _t217 = (bytes_read == ((au_index_t) 0));
                if (_t217) {
                    return false;
                } else {
                    au_index_t _t218 = bytes_read;
                    mono_42** buf_tmp_ref127 = &(buf);
                    *&((*buf_tmp_ref127)->len) = _t218;
                    mono_42** buf_tmp_ref128 = &(buf);
                    mono_32(mono_31(*&((*buf_tmp_ref128)->ptr), ((au_index_t) bytes_read)), ((au_nat8_t) ((au_nat8_t) 0)));
                    return true;
                }
            }
            break;
        case mono_28_tag_None:
            {
                return false;
            }
            break;
    }
}

/*
  Function monomorph: clear[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_48(mono_42* buf) {
    au_index_t _t189 = ((au_index_t) 0);
    mono_42** buf_tmp_ref71 = &(buf);
    *&((*buf_tmp_ref71)->len) = _t189;
    mono_42** buf_tmp_ref72 = &(buf);
    mono_32(*&((*buf_tmp_ref72)->ptr), ((au_nat8_t) ((au_nat8_t) 0)));
    return false;
}

/*
  Function monomorph: nth[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_nat8_t mono_49(mono_42* buf, au_index_t pos) {
    mono_42** buf_tmp_ref73 = &(buf);
    au_bool_t _t190 = (pos >= *&((*buf_tmp_ref73)->len));
    if (_t190) {
        decl_9(au_make_span_from_string("ByteBuf.nth: index out of range", 31));
    } else {
    }
    mono_42** buf_tmp_ref74 = &(buf);
    return mono_39(mono_31(*&((*buf_tmp_ref74)->ptr), ((au_index_t) pos)));
}
/* --- END translation unit for module 'BfetchAust.Sys' --- */

/* --- BEGIN translation unit for module 'BfetchAust.Fetch' --- */
/*
  Union monomorph tag enum: ExitCode[]
*/
typedef enum {
    mono_50_tag_ExitFailure, mono_50_tag_ExitSuccess
} mono_50_tag;

/*
  Union monomorph: ExitCode[]
*/
struct mono_50;
typedef struct mono_50 mono_50;

/*
  Union monomorph: ExitCode[]
*/
typedef struct mono_50 {mono_50_tag tag;union { struct  {} ExitFailure;struct  {} ExitSuccess;} data;} mono_50;

/*
  Constant
*/
#define BfetchAust__Fetch____c_reset 0

/*
  Constant
*/
#define BfetchAust__Fetch____c_bold 1

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord0 30

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord1 90

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord3 97

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord4 97

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord7 36

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord8 96

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord9 34

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord10 94

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord11 91

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord12 93

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord13 33

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord14 32

/*
  Constant
*/
#define BfetchAust__Fetch____c_nord15 95

/*
  Function forward declaratioon
*/
au_bool_t decl_543(au_span_t left, au_span_t right);

/*
  Function forward declaratioon
*/
mono_50 decl_562(mono_1 root);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_51(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_int64_t mono_52(au_int32_t fd, mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_53(mono_42* out);

/*
  Function monomorph forward declaration
*/
mono_43 mono_54(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_55(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_56(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_57(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_58(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_59(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_60(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_61(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_62(mono_42* out, mono_43 ty);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_63(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_64(mono_42* out, mono_43 ty, mono_42* distro, mono_42* kernel, mono_42* uptime, mono_42* wm, mono_42* packages, mono_42* terminal, mono_42* memory, mono_42* shell, mono_42* cpu, mono_42* gpu);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_65(mono_42* buf, au_span_t bytes);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_66(mono_42* out);

/*
  Function monomorph forward declaration
*/
mono_43 mono_67(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_68(mono_42* out, mono_42* src);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_69(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_nat8_t* mono_70(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_71(mono_42* buf, au_nat8_t* str);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_72(mono_42* out);

/*
  Function monomorph forward declaration
*/
mono_10 mono_73(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_74(mono_42* buf, au_span_t bytes);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_75(mono_42* buf, au_index_t value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_76(mono_42* buf, au_nat8_t byte);

/*
  Function monomorph forward declaration
*/
mono_10 mono_77(mono_42* buf, au_span_t key);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_78(mono_42* out, au_index_t kib);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_79(mono_42* out, au_index_t pid, au_span_t tail);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_80(mono_42* buf);

/*
  Function monomorph forward declaration
*/
mono_10 mono_81(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_82(mono_42* out, mono_42* src);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_83(mono_42* out, mono_42* src, au_span_t key);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_84(mono_42* out, mono_42* src);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_85(mono_42* out, au_index_t value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_86(mono_42* out, au_index_t card, au_span_t tail);

/*
  Function monomorph forward declaration
*/
mono_10 mono_87(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_88(mono_42* out, au_index_t vendor, au_index_t device);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_89(mono_42* buf, au_nat32_t value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_90(mono_42* buf, au_nat8_t* str);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_91(mono_42* out, au_bool_t need_sep, au_index_t count, au_span_t manager);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_92(mono_42* out, au_nat8_t* ptr);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_93(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_94(mono_42* out, au_index_t bar_code, au_span_t bar_text, au_index_t label_code, au_span_t label, mono_42* value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_95(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_96(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_97(mono_42* out);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_98(mono_42* buf, au_span_t lit);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_99(mono_42* buf, au_index_t value);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_100(mono_42* buf, au_span_t lit);

/*
  Function monomorph forward declaration
*/
au_index_t mono_101(mono_42* buf);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_102(mono_42* buf, au_index_t pos, au_span_t lit);

/*
  Function monomorph forward declaration
*/
au_bool_t mono_103(mono_42* buf, au_index_t pos, au_index_t value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_104(mono_42* out, mono_42* src, au_index_t start, au_index_t stop, au_index_t vendor);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_105(mono_42* out, au_index_t code0, au_index_t code1, au_span_t text);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_106(mono_42* out, au_index_t code, au_span_t text);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_107(mono_42* out, au_index_t code0, au_index_t code1, au_index_t count);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_108(mono_42* out, au_index_t code0, au_index_t code1, au_span_t text, au_index_t count);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_109(mono_42* out, au_index_t code);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_110(mono_42* out, mono_42* value);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_111(mono_42* out, au_index_t code, au_span_t text, au_index_t count);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_112(mono_42* out, au_index_t code, au_index_t count);

/*
  Function monomorph forward declaration
*/
au_unit_t mono_113(mono_42* out, au_index_t count);

/*
  Function
*/
au_bool_t decl_543(au_span_t left, au_span_t right) {
    au_index_t left_len;
    left_len = mono_20(left);
    au_index_t right_len;
    right_len = mono_20(right);
    au_bool_t _t580 = (left_len != right_len);
    if (_t580) {
        return false;
    } else {
    }
    au_bool_t _t576 = (left_len > ((au_index_t) 0));
    if (_t576) {
        au_index_t _t577 = ((au_index_t) 0);
        au_index_t _t578 = meth_34(((au_index_t) left_len), ((au_index_t) 1));
        {
            size_t i = _t577;
            for (; i <= _t578; i++) {
                au_span_t* left_tmp_ref341 = &(left);
                au_span_t* right_tmp_ref342 = &(right);
                au_bool_t _t579 = (*((au_nat8_t*) au_array_index(left_tmp_ref341, i, sizeof(au_nat8_t))) != *((au_nat8_t*) au_array_index(right_tmp_ref342, i, sizeof(au_nat8_t))));
                if (_t579) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function
*/
mono_50 decl_562(mono_1 root) {
    mono_43 au_ckw_forced_mode;
    au_ckw_forced_mode = ((mono_43) { .tag = mono_43_tag_OtherMode, .data = { .OtherMode = {  } } });
    au_bool_t has_forced_mode;
    has_forced_mode = false;
    au_bool_t show_help;
    show_help = false;
    au_bool_t show_version;
    show_version = false;
    au_index_t argc;
    argc = decl_12();
    au_bool_t _t589 = (argc > ((au_index_t) 1));
    if (_t589) {
        au_index_t _t590 = ((au_index_t) 1);
        au_index_t _t591 = meth_34(((au_index_t) argc), ((au_index_t) 1));
        {
            size_t i = _t590;
            for (; i <= _t591; i++) {
                au_span_t arg;
                arg = decl_13(((au_index_t) i));
                au_bool_t _t592 = decl_543(arg, au_make_span_from_string("--gentoo", 8));
                if (_t592) {
                    mono_43 _t610 = ((mono_43) { .tag = mono_43_tag_GentooMode, .data = { .GentooMode = {  } } });
                    au_ckw_forced_mode = _t610;
                    au_bool_t _t609 = true;
                    has_forced_mode = _t609;
                } else {
                    au_bool_t _t593 = decl_543(arg, au_make_span_from_string("--cachyos", 9));
                    if (_t593) {
                        mono_43 _t608 = ((mono_43) { .tag = mono_43_tag_CachyMode, .data = { .CachyMode = {  } } });
                        au_ckw_forced_mode = _t608;
                        au_bool_t _t607 = true;
                        has_forced_mode = _t607;
                    } else {
                        au_bool_t _t594 = decl_543(arg, au_make_span_from_string("--bedrock", 9));
                        if (_t594) {
                            mono_43 _t606 = ((mono_43) { .tag = mono_43_tag_BedrockMode, .data = { .BedrockMode = {  } } });
                            au_ckw_forced_mode = _t606;
                            au_bool_t _t605 = true;
                            has_forced_mode = _t605;
                        } else {
                            au_bool_t _t595 = decl_543(arg, au_make_span_from_string("-v", 2));
                            if (_t595) {
                                au_bool_t _t604 = true;
                                show_version = _t604;
                            } else {
                                au_bool_t _t596 = decl_543(arg, au_make_span_from_string("--version", 9));
                                if (_t596) {
                                    au_bool_t _t603 = true;
                                    show_version = _t603;
                                } else {
                                    au_bool_t _t597 = decl_543(arg, au_make_span_from_string("-h", 2));
                                    if (_t597) {
                                        au_bool_t _t602 = true;
                                        show_help = _t602;
                                    } else {
                                        au_bool_t _t598 = decl_543(arg, au_make_span_from_string("--help", 6));
                                        if (_t598) {
                                            au_bool_t _t601 = true;
                                            show_help = _t601;
                                        } else {
                                            au_bool_t _t599 = decl_543(arg, au_make_span_from_string("--Help", 6));
                                            if (_t599) {
                                                au_bool_t _t600 = true;
                                                show_help = _t600;
                                            } else {
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
    }
    mono_42 out;
    out = decl_446(((au_index_t) ((au_index_t) 16384)));
    au_bool_t _t585 = show_help;
    if (_t585) {
        mono_42* out_tmp_ref780 = &(out);
        mono_51(out_tmp_ref780);
        mono_42* out_tmp_ref781 = &(out);
        mono_52(((au_int32_t) BfetchAust__Posix____stdout_fd), out_tmp_ref781);
        decl_447(out);
    } else {
        au_bool_t _t586 = show_version;
        if (_t586) {
            mono_42* out_tmp_ref782 = &(out);
            mono_53(out_tmp_ref782);
            mono_42* out_tmp_ref783 = &(out);
            mono_52(((au_int32_t) BfetchAust__Posix____stdout_fd), out_tmp_ref783);
            decl_447(out);
        } else {
            mono_42 distro;
            distro = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 kernel;
            kernel = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 uptime;
            uptime = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 memory;
            memory = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 wm;
            wm = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 terminal;
            terminal = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 shell;
            shell = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 cpu;
            cpu = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 gpu;
            gpu = decl_446(((au_index_t) ((au_index_t) 256)));
            mono_42 packages;
            packages = decl_446(((au_index_t) ((au_index_t) 1024)));
            mono_43 system_type;
            mono_42* distro_tmp_ref784 = &(distro);
            system_type = mono_54(distro_tmp_ref784);
            au_bool_t _t587 = has_forced_mode;
            if (_t587) {
                mono_43 _t588 = au_ckw_forced_mode;
                system_type = _t588;
            } else {
            }
            mono_42* kernel_tmp_ref785 = &(kernel);
            mono_55(kernel_tmp_ref785);
            mono_42* uptime_tmp_ref786 = &(uptime);
            mono_56(uptime_tmp_ref786);
            mono_42* memory_tmp_ref787 = &(memory);
            mono_57(memory_tmp_ref787);
            mono_42* wm_tmp_ref788 = &(wm);
            mono_58(wm_tmp_ref788);
            mono_42* terminal_tmp_ref789 = &(terminal);
            mono_59(terminal_tmp_ref789);
            mono_42* cpu_tmp_ref790 = &(cpu);
            mono_60(cpu_tmp_ref790);
            mono_42* gpu_tmp_ref791 = &(gpu);
            mono_61(gpu_tmp_ref791);
            mono_42* packages_tmp_ref792 = &(packages);
            mono_62(packages_tmp_ref792, system_type);
            mono_42* shell_tmp_ref793 = &(shell);
            mono_63(shell_tmp_ref793);
            mono_42* out_tmp_ref794 = &(out);
            mono_42* distro_tmp_ref795 = &(distro);
            mono_42* kernel_tmp_ref796 = &(kernel);
            mono_42* uptime_tmp_ref797 = &(uptime);
            mono_42* wm_tmp_ref798 = &(wm);
            mono_42* packages_tmp_ref799 = &(packages);
            mono_42* terminal_tmp_ref800 = &(terminal);
            mono_42* memory_tmp_ref801 = &(memory);
            mono_42* shell_tmp_ref802 = &(shell);
            mono_42* cpu_tmp_ref803 = &(cpu);
            mono_42* gpu_tmp_ref804 = &(gpu);
            mono_64(out_tmp_ref794, system_type, distro_tmp_ref795, kernel_tmp_ref796, uptime_tmp_ref797, wm_tmp_ref798, packages_tmp_ref799, terminal_tmp_ref800, memory_tmp_ref801, shell_tmp_ref802, cpu_tmp_ref803, gpu_tmp_ref804);
            mono_42* out_tmp_ref805 = &(out);
            mono_52(((au_int32_t) BfetchAust__Posix____stdout_fd), out_tmp_ref805);
            decl_447(packages);
            decl_447(gpu);
            decl_447(cpu);
            decl_447(shell);
            decl_447(terminal);
            decl_447(wm);
            decl_447(memory);
            decl_447(uptime);
            decl_447(kernel);
            decl_447(distro);
            decl_447(out);
        }
    }
    decl_11(root);
    return ((mono_50) { .tag = mono_50_tag_ExitSuccess, .data = { .ExitSuccess = {  } } });
}

/*
  Function monomorph: appendHelp[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_51(mono_42* out) {
    mono_42* out_tmp_ref374 = out;
    mono_65(out_tmp_ref374, au_make_span_from_string("Usage: ./bfetch [OPTIONS]", 25));
    mono_42* out_tmp_ref375 = out;
    mono_66(out_tmp_ref375);
    mono_42* out_tmp_ref376 = out;
    mono_65(out_tmp_ref376, au_make_span_from_string("Options:", 8));
    mono_42* out_tmp_ref377 = out;
    mono_66(out_tmp_ref377);
    mono_42* out_tmp_ref378 = out;
    mono_65(out_tmp_ref378, au_make_span_from_string("  -v, --version    Show version", 31));
    mono_42* out_tmp_ref379 = out;
    mono_66(out_tmp_ref379);
    mono_42* out_tmp_ref380 = out;
    mono_65(out_tmp_ref380, au_make_span_from_string("  -h, --Help       Show this help", 33));
    mono_42* out_tmp_ref381 = out;
    mono_66(out_tmp_ref381);
    mono_42* out_tmp_ref382 = out;
    mono_65(out_tmp_ref382, au_make_span_from_string("  --gentoo         Force Gentoo mode", 36));
    mono_42* out_tmp_ref383 = out;
    mono_66(out_tmp_ref383);
    mono_42* out_tmp_ref384 = out;
    mono_65(out_tmp_ref384, au_make_span_from_string("  --cachyos        Force CachyOS mode", 37));
    mono_42* out_tmp_ref385 = out;
    mono_66(out_tmp_ref385);
    mono_42* out_tmp_ref386 = out;
    mono_65(out_tmp_ref386, au_make_span_from_string("  --bedrock        Force Bedrock mode", 37));
    mono_42* out_tmp_ref387 = out;
    mono_66(out_tmp_ref387);
    return false;
}

/*
  Function monomorph: writeAll[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_int64_t mono_52(au_int32_t fd, mono_42* buf) {
    mono_42** buf_tmp_ref134 = &(buf);
    mono_42** buf_tmp_ref135 = &(buf);
    return decl_421(((au_int32_t) fd), *&((*buf_tmp_ref134)->ptr), ((au_index_t) *&((*buf_tmp_ref135)->len)));
}

/*
  Function monomorph: appendVersion[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_53(mono_42* out) {
    mono_42* out_tmp_ref388 = out;
    mono_65(out_tmp_ref388, au_make_span_from_string("bfetch version 2.4.0-fastasf", 28));
    mono_42* out_tmp_ref389 = out;
    mono_66(out_tmp_ref389);
    return false;
}

/*
  Function monomorph: getDistroAndType[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_43 mono_54(mono_42* out) {
    mono_42 file;
    file = decl_446(((au_index_t) ((au_index_t) 1024)));
    mono_43 ty;
    ty = ((mono_43) { .tag = mono_43_tag_OtherMode, .data = { .OtherMode = {  } } });
    mono_42* file_tmp_ref138 = &(file);
    au_bool_t _t227 = mono_44(au_make_span_from_string("/etc/os-release", 15), file_tmp_ref138);
    if (_t227) {
        mono_42* file_tmp_ref139 = &(file);
        mono_43 _t229 = mono_67(file_tmp_ref139);
        ty = _t229;
        mono_42* out_tmp_ref140 = out;
        mono_42* file_tmp_ref141 = &(file);
        au_bool_t _t228 = !(mono_68(out_tmp_ref140, file_tmp_ref141));
        if (_t228) {
            mono_42* out_tmp_ref142 = out;
            mono_69(out_tmp_ref142);
        } else {
        }
    } else {
        mono_42* out_tmp_ref143 = out;
        mono_69(out_tmp_ref143);
    }
    decl_447(file);
    au_bool_t _t226 = ((decl_423(au_make_span_from_string("/bedrock", 8), ((au_int32_t) BfetchAust__Posix____f_ok)) == ((au_int32_t) 0)) && decl_493(ty));
    if (_t226) {
        return ((mono_43) { .tag = mono_43_tag_BedrockMode, .data = { .BedrockMode = {  } } });
    } else {
        return ty;
    }
}

/*
  Function monomorph: getKernel[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_55(mono_42* out) {
    mono_42 raw;
    raw = decl_446(((au_index_t) BfetchAust__Posix____utsname_size));
    mono_42* raw_tmp_ref144 = &(raw);
    au_bool_t _t230 = (decl_435(mono_70(raw_tmp_ref144)) == ((au_int32_t) 0));
    if (_t230) {
        mono_42* out_tmp_ref145 = out;
        mono_42* raw_tmp_ref146 = &(raw);
        mono_71(out_tmp_ref145, mono_31(mono_70(raw_tmp_ref146), ((au_index_t) BfetchAust__Posix____utsname_release_offset)));
    } else {
        mono_42* out_tmp_ref147 = out;
        mono_72(out_tmp_ref147);
    }
    decl_447(raw);
    return false;
}

/*
  Function monomorph: getUptime[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_56(mono_42* out) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 64)));
    mono_42* raw_tmp_ref148 = &(raw);
    au_bool_t _t231 = mono_44(au_make_span_from_string("/proc/uptime", 12), raw_tmp_ref148);
    if (_t231) {
        mono_42* raw_tmp_ref149 = &(raw);
        mono_10 _t232 = mono_73(raw_tmp_ref149);
        mono_10 tmp29 = _t232;
        switch ((tmp29).tag) {
            case mono_10_tag_Some:
                {
                    au_index_t secs = (((tmp29).data).Some).value;
                    au_index_t mins;
                    mins = meth_36(((au_index_t) secs), ((au_index_t) ((au_index_t) 60)));
                    au_index_t hours;
                    hours = meth_36(((au_index_t) mins), ((au_index_t) ((au_index_t) 60)));
                    au_index_t days;
                    days = meth_36(((au_index_t) hours), ((au_index_t) ((au_index_t) 24)));
                    mono_42* out_tmp_ref150 = out;
                    mono_74(out_tmp_ref150, au_make_span_from_string("", 0));
                    au_bool_t _t233 = (days > ((au_index_t) 0));
                    if (_t233) {
                        mono_42* out_tmp_ref151 = out;
                        mono_75(out_tmp_ref151, ((au_index_t) days));
                        mono_42* out_tmp_ref152 = out;
                        mono_76(out_tmp_ref152, ((au_nat8_t) 100));
                        mono_42* out_tmp_ref153 = out;
                        mono_76(out_tmp_ref153, ((au_nat8_t) 32));
                        mono_42* out_tmp_ref154 = out;
                        mono_75(out_tmp_ref154, ((au_index_t) meth_283(hours, ((au_index_t) 24))));
                        mono_42* out_tmp_ref155 = out;
                        mono_76(out_tmp_ref155, ((au_nat8_t) 104));
                        mono_42* out_tmp_ref156 = out;
                        mono_76(out_tmp_ref156, ((au_nat8_t) 32));
                        mono_42* out_tmp_ref157 = out;
                        mono_75(out_tmp_ref157, ((au_index_t) meth_283(mins, ((au_index_t) 60))));
                        mono_42* out_tmp_ref158 = out;
                        mono_76(out_tmp_ref158, ((au_nat8_t) 109));
                    } else {
                        mono_42* out_tmp_ref159 = out;
                        mono_75(out_tmp_ref159, ((au_index_t) hours));
                        mono_42* out_tmp_ref160 = out;
                        mono_76(out_tmp_ref160, ((au_nat8_t) 104));
                        mono_42* out_tmp_ref161 = out;
                        mono_76(out_tmp_ref161, ((au_nat8_t) 32));
                        mono_42* out_tmp_ref162 = out;
                        mono_75(out_tmp_ref162, ((au_index_t) meth_283(mins, ((au_index_t) 60))));
                        mono_42* out_tmp_ref163 = out;
                        mono_76(out_tmp_ref163, ((au_nat8_t) 109));
                    }
                }
                break;
            case mono_10_tag_None:
                {
                    mono_42* out_tmp_ref164 = out;
                    mono_72(out_tmp_ref164);
                }
                break;
        }
    } else {
        mono_42* out_tmp_ref165 = out;
        mono_72(out_tmp_ref165);
    }
    decl_447(raw);
    return false;
}

/*
  Function monomorph: getMemory[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_57(mono_42* out) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 2048)));
    mono_42* raw_tmp_ref166 = &(raw);
    au_bool_t _t234 = mono_44(au_make_span_from_string("/proc/meminfo", 13), raw_tmp_ref166);
    if (_t234) {
        mono_42* raw_tmp_ref167 = &(raw);
        mono_10 _t235 = mono_77(raw_tmp_ref167, au_make_span_from_string("MemTotal:", 9));
        mono_10 tmp30 = _t235;
        switch ((tmp30).tag) {
            case mono_10_tag_Some:
                {
                    au_index_t total = (((tmp30).data).Some).value;
                    mono_42* raw_tmp_ref168 = &(raw);
                    mono_10 _t236 = mono_77(raw_tmp_ref168, au_make_span_from_string("MemAvailable:", 13));
                    mono_10 tmp31 = _t236;
                    switch ((tmp31).tag) {
                        case mono_10_tag_Some:
                            {
                                au_index_t available = (((tmp31).data).Some).value;
                                au_index_t used;
                                used = meth_34(((au_index_t) total), ((au_index_t) available));
                                mono_42* out_tmp_ref169 = out;
                                mono_74(out_tmp_ref169, au_make_span_from_string("", 0));
                                mono_42* out_tmp_ref170 = out;
                                mono_78(out_tmp_ref170, ((au_index_t) used));
                                mono_42* out_tmp_ref171 = out;
                                mono_65(out_tmp_ref171, au_make_span_from_string(" / ", 3));
                                mono_42* out_tmp_ref172 = out;
                                mono_78(out_tmp_ref172, ((au_index_t) total));
                            }
                            break;
                        case mono_10_tag_None:
                            {
                                mono_42* out_tmp_ref173 = out;
                                mono_72(out_tmp_ref173);
                            }
                            break;
                    }
                }
                break;
            case mono_10_tag_None:
                {
                    mono_42* out_tmp_ref174 = out;
                    mono_72(out_tmp_ref174);
                }
                break;
        }
    } else {
        mono_42* out_tmp_ref175 = out;
        mono_72(out_tmp_ref175);
    }
    decl_447(raw);
    return false;
}

/*
  Function monomorph: getWm[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_58(mono_42* out) {
    mono_28 _t237 = mono_29(decl_424(au_make_span_from_string("XDG_CURRENT_DESKTOP", 19)));
    mono_28 tmp32 = _t237;
    switch ((tmp32).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* ptr = (((tmp32).data).Some).value;
                mono_42* out_tmp_ref176 = out;
                mono_71(out_tmp_ref176, ptr);
            }
            break;
        case mono_28_tag_None:
            {
                mono_28 _t238 = mono_29(decl_424(au_make_span_from_string("DESKTOP_SESSION", 15)));
                mono_28 tmp33 = _t238;
                switch ((tmp33).tag) {
                    case mono_28_tag_Some:
                        {
                            au_nat8_t* ptr = (((tmp33).data).Some).value;
                            mono_42* out_tmp_ref177 = out;
                            mono_71(out_tmp_ref177, ptr);
                        }
                        break;
                    case mono_28_tag_None:
                        {
                            mono_42* out_tmp_ref178 = out;
                            mono_72(out_tmp_ref178);
                        }
                        break;
                }
            }
            break;
    }
    return false;
}

/*
  Function monomorph: getTerminal[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_59(mono_42* out) {
    mono_28 _t239 = mono_29(decl_424(au_make_span_from_string("TERM_PROGRAM", 12)));
    mono_28 tmp34 = _t239;
    switch ((tmp34).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* ptr = (((tmp34).data).Some).value;
                mono_42* out_tmp_ref179 = out;
                mono_71(out_tmp_ref179, ptr);
            }
            break;
        case mono_28_tag_None:
            {
                mono_10 _t240 = meth_248(decl_425());
                mono_10 tmp35 = _t240;
                switch ((tmp35).tag) {
                    case mono_10_tag_Some:
                        {
                            au_index_t ppid = (((tmp35).data).Some).value;
                            mono_42 path;
                            path = decl_446(((au_index_t) ((au_index_t) 64)));
                            mono_42 proc;
                            proc = decl_446(((au_index_t) ((au_index_t) 256)));
                            mono_42* path_tmp_ref180 = &(path);
                            mono_79(path_tmp_ref180, ((au_index_t) ppid), au_make_span_from_string("/comm", 5));
                            mono_42* path_tmp_ref181 = &(path);
                            mono_42* proc_tmp_ref182 = &(proc);
                            au_bool_t _t241 = mono_47(mono_70(path_tmp_ref181), proc_tmp_ref182);
                            if (_t241) {
                                mono_42* proc_tmp_ref183 = &(proc);
                                au_bool_t _t243 = mono_80(proc_tmp_ref183);
                                if (_t243) {
                                    mono_42* path_tmp_ref184 = &(path);
                                    mono_79(path_tmp_ref184, ((au_index_t) ppid), au_make_span_from_string("/stat", 5));
                                    mono_42* path_tmp_ref185 = &(path);
                                    mono_42* proc_tmp_ref186 = &(proc);
                                    au_bool_t _t244 = mono_47(mono_70(path_tmp_ref185), proc_tmp_ref186);
                                    if (_t244) {
                                        mono_42* proc_tmp_ref187 = &(proc);
                                        mono_10 _t245 = mono_81(proc_tmp_ref187);
                                        mono_10 tmp37 = _t245;
                                        switch ((tmp37).tag) {
                                            case mono_10_tag_Some:
                                                {
                                                    au_index_t pppid = (((tmp37).data).Some).value;
                                                    mono_42* path_tmp_ref188 = &(path);
                                                    mono_79(path_tmp_ref188, ((au_index_t) pppid), au_make_span_from_string("/comm", 5));
                                                    mono_42* path_tmp_ref189 = &(path);
                                                    mono_42* proc_tmp_ref190 = &(proc);
                                                    au_bool_t _t246 = mono_47(mono_70(path_tmp_ref189), proc_tmp_ref190);
                                                    if (_t246) {
                                                        mono_42* out_tmp_ref191 = out;
                                                        mono_42* proc_tmp_ref192 = &(proc);
                                                        mono_82(out_tmp_ref191, proc_tmp_ref192);
                                                    } else {
                                                        mono_42* out_tmp_ref193 = out;
                                                        mono_72(out_tmp_ref193);
                                                    }
                                                }
                                                break;
                                            case mono_10_tag_None:
                                                {
                                                    mono_42* out_tmp_ref194 = out;
                                                    mono_72(out_tmp_ref194);
                                                }
                                                break;
                                        }
                                    } else {
                                        mono_42* out_tmp_ref195 = out;
                                        mono_72(out_tmp_ref195);
                                    }
                                } else {
                                    mono_42* out_tmp_ref196 = out;
                                    mono_42* proc_tmp_ref197 = &(proc);
                                    mono_82(out_tmp_ref196, proc_tmp_ref197);
                                }
                            } else {
                                mono_28 _t242 = mono_29(decl_424(au_make_span_from_string("TERM", 4)));
                                mono_28 tmp36 = _t242;
                                switch ((tmp36).tag) {
                                    case mono_28_tag_Some:
                                        {
                                            au_nat8_t* ptr = (((tmp36).data).Some).value;
                                            mono_42* out_tmp_ref198 = out;
                                            mono_71(out_tmp_ref198, ptr);
                                        }
                                        break;
                                    case mono_28_tag_None:
                                        {
                                            mono_42* out_tmp_ref199 = out;
                                            mono_72(out_tmp_ref199);
                                        }
                                        break;
                                }
                            }
                            decl_447(proc);
                            decl_447(path);
                        }
                        break;
                    case mono_10_tag_None:
                        {
                            mono_28 _t247 = mono_29(decl_424(au_make_span_from_string("TERM", 4)));
                            mono_28 tmp38 = _t247;
                            switch ((tmp38).tag) {
                                case mono_28_tag_Some:
                                    {
                                        au_nat8_t* ptr = (((tmp38).data).Some).value;
                                        mono_42* out_tmp_ref200 = out;
                                        mono_71(out_tmp_ref200, ptr);
                                    }
                                    break;
                                case mono_28_tag_None:
                                    {
                                        mono_42* out_tmp_ref201 = out;
                                        mono_72(out_tmp_ref201);
                                    }
                                    break;
                            }
                        }
                        break;
                }
            }
            break;
    }
    return false;
}

/*
  Function monomorph: getCpu[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_60(mono_42* out) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 65536)));
    mono_42 name;
    name = decl_446(((au_index_t) ((au_index_t) 256)));
    mono_42 freq;
    freq = decl_446(((au_index_t) ((au_index_t) 64)));
    au_bool_t found_name;
    found_name = false;
    mono_42* raw_tmp_ref228 = &(raw);
    au_bool_t _t274 = mono_44(au_make_span_from_string("/proc/cpuinfo", 13), raw_tmp_ref228);
    if (_t274) {
        mono_42* name_tmp_ref229 = &(name);
        mono_42* raw_tmp_ref230 = &(raw);
        au_bool_t _t279 = mono_83(name_tmp_ref229, raw_tmp_ref230, au_make_span_from_string("model name", 10));
        if (_t279) {
            au_bool_t _t284 = true;
            found_name = _t284;
        } else {
            mono_42* name_tmp_ref231 = &(name);
            mono_42* raw_tmp_ref232 = &(raw);
            au_bool_t _t280 = mono_83(name_tmp_ref231, raw_tmp_ref232, au_make_span_from_string("Hardware", 8));
            if (_t280) {
                au_bool_t _t283 = true;
                found_name = _t283;
            } else {
                mono_42* name_tmp_ref233 = &(name);
                mono_42* raw_tmp_ref234 = &(raw);
                au_bool_t _t281 = mono_83(name_tmp_ref233, raw_tmp_ref234, au_make_span_from_string("Processor", 9));
                if (_t281) {
                    au_bool_t _t282 = true;
                    found_name = _t282;
                } else {
                }
            }
        }
        au_bool_t _t275 = found_name;
        if (_t275) {
            mono_42* out_tmp_ref235 = out;
            mono_42* name_tmp_ref236 = &(name);
            mono_84(out_tmp_ref235, name_tmp_ref236);
            mono_42* out_tmp_ref237 = out;
            mono_65(out_tmp_ref237, au_make_span_from_string(" (", 2));
            mono_10 _t278 = meth_248(decl_436());
            mono_10 tmp40 = _t278;
            switch ((tmp40).tag) {
                case mono_10_tag_Some:
                    {
                        au_index_t threads = (((tmp40).data).Some).value;
                        mono_42* out_tmp_ref238 = out;
                        mono_75(out_tmp_ref238, ((au_index_t) threads));
                    }
                    break;
                case mono_10_tag_None:
                    {
                        mono_42* out_tmp_ref239 = out;
                        mono_76(out_tmp_ref239, ((au_nat8_t) 63));
                    }
                    break;
            }
            mono_42* out_tmp_ref240 = out;
            mono_76(out_tmp_ref240, ((au_nat8_t) 41));
            mono_42* freq_tmp_ref241 = &(freq);
            au_bool_t _t276 = mono_44(au_make_span_from_string("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", 53), freq_tmp_ref241);
            if (_t276) {
                mono_42* freq_tmp_ref242 = &(freq);
                mono_10 _t277 = mono_73(freq_tmp_ref242);
                mono_10 tmp39 = _t277;
                switch ((tmp39).tag) {
                    case mono_10_tag_Some:
                        {
                            au_index_t khz = (((tmp39).data).Some).value;
                            au_index_t hundredths;
                            hundredths = meth_36(((au_index_t) meth_33(((au_index_t) khz), ((au_index_t) ((au_index_t) 5000)))), ((au_index_t) ((au_index_t) 10000)));
                            mono_42* out_tmp_ref243 = out;
                            mono_65(out_tmp_ref243, au_make_span_from_string(" @ ", 3));
                            mono_42* out_tmp_ref244 = out;
                            mono_75(out_tmp_ref244, ((au_index_t) meth_36(((au_index_t) hundredths), ((au_index_t) ((au_index_t) 100)))));
                            mono_42* out_tmp_ref245 = out;
                            mono_76(out_tmp_ref245, ((au_nat8_t) 46));
                            mono_42* out_tmp_ref246 = out;
                            mono_85(out_tmp_ref246, ((au_index_t) meth_283(hundredths, ((au_index_t) 100))));
                            mono_42* out_tmp_ref247 = out;
                            mono_65(out_tmp_ref247, au_make_span_from_string(" GHz", 4));
                        }
                        break;
                    case mono_10_tag_None:
                        {
                        }
                        break;
                }
            } else {
            }
        } else {
            mono_42* out_tmp_ref248 = out;
            mono_72(out_tmp_ref248);
        }
    } else {
        mono_42* out_tmp_ref249 = out;
        mono_72(out_tmp_ref249);
    }
    decl_447(freq);
    decl_447(name);
    decl_447(raw);
    return false;
}

/*
  Function monomorph: getGpu[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_61(mono_42* out) {
    mono_42 path;
    path = decl_446(((au_index_t) ((au_index_t) 128)));
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 64)));
    au_index_t vendor;
    vendor = ((au_index_t) 0);
    au_index_t device;
    device = ((au_index_t) 0);
    au_index_t card;
    card = ((au_index_t) 0);
    au_bool_t _t289 = ((card < ((au_index_t) 10)) && (vendor == ((au_index_t) 0)));
    while (_t289) {
        mono_42* path_tmp_ref250 = &(path);
        mono_86(path_tmp_ref250, ((au_index_t) card), au_make_span_from_string("/device/vendor", 14));
        mono_42* path_tmp_ref251 = &(path);
        mono_42* raw_tmp_ref252 = &(raw);
        au_bool_t _t291 = mono_47(mono_70(path_tmp_ref251), raw_tmp_ref252);
        if (_t291) {
            mono_42* raw_tmp_ref253 = &(raw);
            mono_10 _t297 = mono_87(raw_tmp_ref253);
            mono_10 tmp42 = _t297;
            switch ((tmp42).tag) {
                case mono_10_tag_Some:
                    {
                        au_index_t parsed = (((tmp42).data).Some).value;
                        au_index_t _t298 = parsed;
                        vendor = _t298;
                    }
                    break;
                case mono_10_tag_None:
                    {
                    }
                    break;
            }
            au_bool_t _t292 = (vendor != ((au_index_t) 0));
            if (_t292) {
                mono_42* path_tmp_ref254 = &(path);
                mono_86(path_tmp_ref254, ((au_index_t) card), au_make_span_from_string("/device/device", 14));
                mono_42* path_tmp_ref255 = &(path);
                mono_42* raw_tmp_ref256 = &(raw);
                au_bool_t _t293 = mono_47(mono_70(path_tmp_ref255), raw_tmp_ref256);
                if (_t293) {
                    mono_42* raw_tmp_ref257 = &(raw);
                    mono_10 _t294 = mono_87(raw_tmp_ref257);
                    mono_10 tmp41 = _t294;
                    switch ((tmp41).tag) {
                        case mono_10_tag_Some:
                            {
                                au_index_t parsed = (((tmp41).data).Some).value;
                                au_index_t _t295 = parsed;
                                device = _t295;
                            }
                            break;
                        case mono_10_tag_None:
                            {
                                au_index_t _t296 = ((au_index_t) 0);
                                device = _t296;
                            }
                            break;
                    }
                } else {
                }
            } else {
            }
        } else {
        }
        au_index_t _t290 = meth_33(((au_index_t) card), ((au_index_t) ((au_index_t) 1)));
        card = _t290;
        _t289 = ((card < ((au_index_t) 10)) && (vendor == ((au_index_t) 0)));
    }
    au_bool_t _t285 = ((vendor == ((au_index_t) 0)) || (device == ((au_index_t) 0)));
    if (_t285) {
        mono_42* out_tmp_ref258 = out;
        mono_72(out_tmp_ref258);
    } else {
        mono_42* out_tmp_ref259 = out;
        au_bool_t _t286 = mono_88(out_tmp_ref259, ((au_index_t) vendor), ((au_index_t) device));
        if (_t286) {
        } else {
            au_bool_t _t287 = (vendor == ((au_index_t) 4318));
            if (_t287) {
                mono_42* out_tmp_ref260 = out;
                mono_74(out_tmp_ref260, au_make_span_from_string("NVIDIA GPU 0x", 13));
                mono_42* out_tmp_ref261 = out;
                mono_89(out_tmp_ref261, ((au_nat32_t) decl_506(((au_index_t) device))));
            } else {
                au_bool_t _t288 = (vendor == ((au_index_t) 4098));
                if (_t288) {
                    mono_42* out_tmp_ref262 = out;
                    mono_74(out_tmp_ref262, au_make_span_from_string("AMD GPU 0x", 10));
                    mono_42* out_tmp_ref263 = out;
                    mono_89(out_tmp_ref263, ((au_nat32_t) decl_506(((au_index_t) device))));
                } else {
                    mono_42* out_tmp_ref264 = out;
                    mono_74(out_tmp_ref264, au_make_span_from_string("GPU 0x", 6));
                    mono_42* out_tmp_ref265 = out;
                    mono_89(out_tmp_ref265, ((au_nat32_t) decl_506(((au_index_t) vendor))));
                    mono_42* out_tmp_ref266 = out;
                    mono_65(out_tmp_ref266, au_make_span_from_string(":0x", 3));
                    mono_42* out_tmp_ref267 = out;
                    mono_89(out_tmp_ref267, ((au_nat32_t) decl_506(((au_index_t) device))));
                }
            }
        }
    }
    decl_447(raw);
    decl_447(path);
    return false;
}

/*
  Function monomorph: getPackages[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_62(mono_42* out, mono_43 ty) {
    au_index_t pacman;
    pacman = decl_522(au_make_span_from_string("/var/lib/pacman/local", 21));
    au_index_t dpkg;
    dpkg = decl_524();
    au_index_t nix;
    nix = decl_525(au_make_span_from_string("/nix/var/nix/profiles/default/manifest.json", 43), au_make_span_from_string("\"active\":true", 13));
    au_index_t _t273 = meth_33(((au_index_t) nix), ((au_index_t) decl_525(au_make_span_from_string("/run/current-system/sw/manifest.json", 36), au_make_span_from_string("\"active\":true", 13))));
    nix = _t273;
    au_index_t flatpak;
    flatpak = decl_522(au_make_span_from_string("/var/lib/flatpak/app", 20));
    au_index_t snap;
    snap = decl_522(au_make_span_from_string("/var/lib/snapd/snaps", 20));
    mono_43 _t266 = ty;
    mono_43 tmp44 = _t266;
    switch ((tmp44).tag) {
        case mono_43_tag_OtherMode:
            {
            }
            break;
        case mono_43_tag_CachyMode:
            {
            }
            break;
        case mono_43_tag_GentooMode:
            {
                mono_28 _t267 = mono_29(decl_428(au_make_span_from_string("/var/db/pkg", 11)));
                mono_28 tmp45 = _t267;
                switch ((tmp45).tag) {
                    case mono_28_tag_Some:
                        {
                            au_nat8_t* dir = (((tmp45).data).Some).value;
                            au_index_t count;
                            count = ((au_index_t) 0);
                            mono_42 path;
                            path = decl_446(((au_index_t) ((au_index_t) 1024)));
                            au_bool_t reading;
                            reading = true;
                            au_bool_t _t268 = reading;
                            while (_t268) {
                                mono_28 _t269 = mono_29(decl_430(dir));
                                mono_28 tmp46 = _t269;
                                switch ((tmp46).tag) {
                                    case mono_28_tag_Some:
                                        {
                                            au_nat8_t* ent = (((tmp46).data).Some).value;
                                            au_nat8_t* name;
                                            name = decl_441(ent);
                                            au_bool_t _t270 = (!(decl_515(name)) && (decl_440(ent) == ((au_nat8_t) 4)));
                                            if (_t270) {
                                                mono_42* path_tmp_ref204 = &(path);
                                                mono_74(path_tmp_ref204, au_make_span_from_string("/var/db/pkg/", 12));
                                                mono_42* path_tmp_ref205 = &(path);
                                                mono_90(path_tmp_ref205, name);
                                                mono_42* path_tmp_ref206 = &(path);
                                                au_index_t _t271 = meth_33(((au_index_t) count), ((au_index_t) decl_523(mono_70(path_tmp_ref206))));
                                                count = _t271;
                                            } else {
                                            }
                                        }
                                        break;
                                    case mono_28_tag_None:
                                        {
                                            au_bool_t _t272 = false;
                                            reading = _t272;
                                        }
                                        break;
                                }
                                _t268 = reading;
                            }
                            decl_431(dir);
                            decl_447(path);
                            mono_42* out_tmp_ref207 = out;
                            mono_75(out_tmp_ref207, ((au_index_t) count));
                            mono_42* out_tmp_ref208 = out;
                            mono_65(out_tmp_ref208, au_make_span_from_string(" (emerge)", 9));
                            return false;
                        }
                        break;
                    case mono_28_tag_None:
                        {
                        }
                        break;
                }
            }
            break;
        case mono_43_tag_BedrockMode:
            {
            }
            break;
    }
    mono_28 _t260 = mono_29(decl_424(au_make_span_from_string("HOME", 4)));
    mono_28 tmp43 = _t260;
    switch ((tmp43).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* home = (((tmp43).data).Some).value;
                mono_42 path;
                path = decl_446(((au_index_t) ((au_index_t) 1024)));
                mono_42* path_tmp_ref209 = &(path);
                mono_71(path_tmp_ref209, home);
                mono_42* path_tmp_ref210 = &(path);
                mono_65(path_tmp_ref210, au_make_span_from_string("/.local/share/flatpak/app", 25));
                mono_42* path_tmp_ref211 = &(path);
                au_index_t _t265 = meth_33(((au_index_t) flatpak), ((au_index_t) decl_523(mono_70(path_tmp_ref211))));
                flatpak = _t265;
                mono_42* path_tmp_ref212 = &(path);
                mono_71(path_tmp_ref212, home);
                mono_42* path_tmp_ref213 = &(path);
                mono_65(path_tmp_ref213, au_make_span_from_string("/.nix-profile/manifest.json", 27));
                mono_42* path_tmp_ref214 = &(path);
                au_index_t _t264 = meth_33(((au_index_t) nix), ((au_index_t) decl_526(mono_70(path_tmp_ref214), au_make_span_from_string("\"active\":true", 13))));
                nix = _t264;
                au_bool_t _t262 = (nix == ((au_index_t) 0));
                if (_t262) {
                    mono_42* path_tmp_ref215 = &(path);
                    mono_71(path_tmp_ref215, home);
                    mono_42* path_tmp_ref216 = &(path);
                    mono_65(path_tmp_ref216, au_make_span_from_string("/.nix-profile/manifest.nix", 26));
                    mono_42* path_tmp_ref217 = &(path);
                    au_index_t _t263 = meth_33(((au_index_t) nix), ((au_index_t) decl_526(mono_70(path_tmp_ref217), au_make_span_from_string("name = \"", 8))));
                    nix = _t263;
                } else {
                }
                mono_42* path_tmp_ref218 = &(path);
                mono_71(path_tmp_ref218, home);
                mono_42* path_tmp_ref219 = &(path);
                mono_65(path_tmp_ref219, au_make_span_from_string("/.local/state/nix/profiles/home-manager/manifest.json", 53));
                mono_42* path_tmp_ref220 = &(path);
                au_index_t _t261 = meth_33(((au_index_t) nix), ((au_index_t) decl_526(mono_70(path_tmp_ref220), au_make_span_from_string("\"active\":true", 13))));
                nix = _t261;
                decl_447(path);
            }
            break;
        case mono_28_tag_None:
            {
            }
            break;
    }
    mono_42* out_tmp_ref221 = out;
    mono_74(out_tmp_ref221, au_make_span_from_string("", 0));
    au_bool_t have_any;
    have_any = false;
    mono_42* out_tmp_ref222 = out;
    mono_91(out_tmp_ref222, have_any, ((au_index_t) pacman), au_make_span_from_string("pacman", 6));
    au_bool_t _t258 = (pacman > ((au_index_t) 0));
    if (_t258) {
        au_bool_t _t259 = true;
        have_any = _t259;
    } else {
    }
    mono_42* out_tmp_ref223 = out;
    mono_91(out_tmp_ref223, have_any, ((au_index_t) dpkg), au_make_span_from_string("dpkg", 4));
    au_bool_t _t256 = (dpkg > ((au_index_t) 0));
    if (_t256) {
        au_bool_t _t257 = true;
        have_any = _t257;
    } else {
    }
    mono_42* out_tmp_ref224 = out;
    mono_91(out_tmp_ref224, have_any, ((au_index_t) flatpak), au_make_span_from_string("flatpak", 7));
    au_bool_t _t254 = (flatpak > ((au_index_t) 0));
    if (_t254) {
        au_bool_t _t255 = true;
        have_any = _t255;
    } else {
    }
    mono_42* out_tmp_ref225 = out;
    mono_91(out_tmp_ref225, have_any, ((au_index_t) snap), au_make_span_from_string("snap", 4));
    au_bool_t _t252 = (snap > ((au_index_t) 0));
    if (_t252) {
        au_bool_t _t253 = true;
        have_any = _t253;
    } else {
    }
    mono_42* out_tmp_ref226 = out;
    mono_91(out_tmp_ref226, have_any, ((au_index_t) nix), au_make_span_from_string("nix", 3));
    au_bool_t _t250 = (nix > ((au_index_t) 0));
    if (_t250) {
        au_bool_t _t251 = true;
        have_any = _t251;
    } else {
    }
    au_bool_t _t249 = !(have_any);
    if (_t249) {
        mono_42* out_tmp_ref227 = out;
        mono_72(out_tmp_ref227);
    } else {
    }
    return false;
}

/*
  Function monomorph: getShell[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_63(mono_42* out) {
    mono_28 _t248 = mono_29(decl_424(au_make_span_from_string("SHELL", 5)));
    mono_28 tmp47 = _t248;
    switch ((tmp47).tag) {
        case mono_28_tag_Some:
            {
                au_nat8_t* ptr = (((tmp47).data).Some).value;
                mono_42* out_tmp_ref202 = out;
                mono_92(out_tmp_ref202, ptr);
            }
            break;
        case mono_28_tag_None:
            {
                mono_42* out_tmp_ref203 = out;
                mono_72(out_tmp_ref203);
            }
            break;
    }
    return false;
}

/*
  Function monomorph: renderFetch[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_64(mono_42* out, mono_43 ty, mono_42* distro, mono_42* kernel, mono_42* uptime, mono_42* wm, mono_42* packages, mono_42* terminal, mono_42* memory, mono_42* shell, mono_42* cpu, mono_42* gpu) {
    mono_43 _t584 = ty;
    mono_43 tmp48 = _t584;
    switch ((tmp48).tag) {
        case mono_43_tag_OtherMode:
            {
                mono_42* out_tmp_ref768 = out;
                mono_93(out_tmp_ref768);
                mono_42* out_tmp_ref769 = out;
                mono_94(out_tmp_ref769, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Distro: ", 8), distro);
                mono_42* out_tmp_ref770 = out;
                mono_94(out_tmp_ref770, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Kernel: ", 8), kernel);
                mono_42* out_tmp_ref771 = out;
                mono_94(out_tmp_ref771, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Uptime: ", 8), uptime);
                mono_42* out_tmp_ref772 = out;
                mono_94(out_tmp_ref772, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("WM: ", 4), wm);
                mono_42* out_tmp_ref773 = out;
                mono_94(out_tmp_ref773, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Packages: ", 10), packages);
                mono_42* out_tmp_ref774 = out;
                mono_94(out_tmp_ref774, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Terminal: ", 10), terminal);
                mono_42* out_tmp_ref775 = out;
                mono_94(out_tmp_ref775, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Memory: ", 8), memory);
                mono_42* out_tmp_ref776 = out;
                mono_94(out_tmp_ref776, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Shell: ", 7), shell);
                mono_42* out_tmp_ref777 = out;
                mono_94(out_tmp_ref777, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("CPU: ", 5), cpu);
                mono_42* out_tmp_ref778 = out;
                mono_94(out_tmp_ref778, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("GPU: ", 5), gpu);
            }
            break;
        case mono_43_tag_CachyMode:
            {
                mono_42* out_tmp_ref746 = out;
                mono_95(out_tmp_ref746);
                mono_42* out_tmp_ref747 = out;
                mono_94(out_tmp_ref747, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Distro: ", 8), distro);
                mono_42* out_tmp_ref748 = out;
                mono_94(out_tmp_ref748, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Kernel: ", 8), kernel);
                mono_42* out_tmp_ref749 = out;
                mono_94(out_tmp_ref749, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Uptime: ", 8), uptime);
                mono_42* out_tmp_ref750 = out;
                mono_94(out_tmp_ref750, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("WM: ", 4), wm);
                mono_42* out_tmp_ref751 = out;
                mono_94(out_tmp_ref751, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Packages: ", 10), packages);
                mono_42* out_tmp_ref752 = out;
                mono_94(out_tmp_ref752, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Terminal: ", 10), terminal);
                mono_42* out_tmp_ref753 = out;
                mono_94(out_tmp_ref753, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Memory: ", 8), memory);
                mono_42* out_tmp_ref754 = out;
                mono_94(out_tmp_ref754, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Shell: ", 7), shell);
                mono_42* out_tmp_ref755 = out;
                mono_94(out_tmp_ref755, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("CPU: ", 5), cpu);
                mono_42* out_tmp_ref756 = out;
                mono_94(out_tmp_ref756, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("GPU: ", 5), gpu);
            }
            break;
        case mono_43_tag_GentooMode:
            {
                mono_42* out_tmp_ref735 = out;
                mono_96(out_tmp_ref735);
                mono_42* out_tmp_ref736 = out;
                mono_94(out_tmp_ref736, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Distro: ", 8), distro);
                mono_42* out_tmp_ref737 = out;
                mono_94(out_tmp_ref737, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Kernel: ", 8), kernel);
                mono_42* out_tmp_ref738 = out;
                mono_94(out_tmp_ref738, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Uptime: ", 8), uptime);
                mono_42* out_tmp_ref739 = out;
                mono_94(out_tmp_ref739, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("WM: ", 4), wm);
                mono_42* out_tmp_ref740 = out;
                mono_94(out_tmp_ref740, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Packages: ", 10), packages);
                mono_42* out_tmp_ref741 = out;
                mono_94(out_tmp_ref741, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Terminal: ", 10), terminal);
                mono_42* out_tmp_ref742 = out;
                mono_94(out_tmp_ref742, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Memory: ", 8), memory);
                mono_42* out_tmp_ref743 = out;
                mono_94(out_tmp_ref743, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Shell: ", 7), shell);
                mono_42* out_tmp_ref744 = out;
                mono_94(out_tmp_ref744, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("CPU: ", 5), cpu);
                mono_42* out_tmp_ref745 = out;
                mono_94(out_tmp_ref745, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("GPU: ", 5), gpu);
            }
            break;
        case mono_43_tag_BedrockMode:
            {
                mono_42* out_tmp_ref757 = out;
                mono_93(out_tmp_ref757);
                mono_42* out_tmp_ref758 = out;
                mono_94(out_tmp_ref758, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Distro: ", 8), distro);
                mono_42* out_tmp_ref759 = out;
                mono_94(out_tmp_ref759, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("Kernel: ", 8), kernel);
                mono_42* out_tmp_ref760 = out;
                mono_94(out_tmp_ref760, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Uptime: ", 8), uptime);
                mono_42* out_tmp_ref761 = out;
                mono_94(out_tmp_ref761, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("WM: ", 4), wm);
                mono_42* out_tmp_ref762 = out;
                mono_94(out_tmp_ref762, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("Packages: ", 10), packages);
                mono_42* out_tmp_ref763 = out;
                mono_94(out_tmp_ref763, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Terminal: ", 10), terminal);
                mono_42* out_tmp_ref764 = out;
                mono_94(out_tmp_ref764, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Memory: ", 8), memory);
                mono_42* out_tmp_ref765 = out;
                mono_94(out_tmp_ref765, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("Shell: ", 7), shell);
                mono_42* out_tmp_ref766 = out;
                mono_94(out_tmp_ref766, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("CPU: ", 5), cpu);
                mono_42* out_tmp_ref767 = out;
                mono_94(out_tmp_ref767, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6), ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("GPU: ", 5), gpu);
            }
            break;
    }
    mono_42* out_tmp_ref779 = out;
    mono_97(out_tmp_ref779);
    return false;
}

/*
  Function monomorph: appendSpan[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_65(mono_42* buf, au_span_t bytes) {
    au_index_t n;
    n = mono_20(bytes);
    au_bool_t _t196 = (n > 0);
    if (_t196) {
        au_index_t _t197 = ((au_index_t) 0);
        au_index_t _t198 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t197;
            for (; i <= _t198; i++) {
                mono_42** buf_tmp_ref88 = &(buf);
                mono_42** buf_tmp_ref89 = &(buf);
                au_bool_t _t200 = (meth_33(((au_index_t) *&((*buf_tmp_ref88)->len)), ((au_index_t) ((au_index_t) 1))) >= *&((*buf_tmp_ref89)->cap));
                if (_t200) {
                    decl_9(au_make_span_from_string("ByteBuf.appendSpan: buffer full", 31));
                } else {
                }
                mono_42** buf_tmp_ref90 = &(buf);
                mono_42** buf_tmp_ref91 = &(buf);
                au_span_t* bytes_tmp_ref92 = &(bytes);
                mono_32(mono_31(*&((*buf_tmp_ref90)->ptr), ((au_index_t) *&((*buf_tmp_ref91)->len))), ((au_nat8_t) *((au_nat8_t*) au_array_index(bytes_tmp_ref92, i, sizeof(au_nat8_t)))));
                mono_42** buf_tmp_ref93 = &(buf);
                au_index_t _t199 = meth_33(((au_index_t) *&((*buf_tmp_ref93)->len)), ((au_index_t) ((au_index_t) 1)));
                mono_42** buf_tmp_ref94 = &(buf);
                *&((*buf_tmp_ref94)->len) = _t199;
                mono_42** buf_tmp_ref95 = &(buf);
                mono_42** buf_tmp_ref96 = &(buf);
                mono_32(mono_31(*&((*buf_tmp_ref95)->ptr), ((au_index_t) *&((*buf_tmp_ref96)->len))), ((au_nat8_t) ((au_nat8_t) 0)));
            }
        }
    } else {
    }
    return false;
}

/*
  Function monomorph: nl[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_66(mono_42* out) {
    mono_42* out_tmp_ref352 = out;
    mono_76(out_tmp_ref352, ((au_nat8_t) ((au_nat8_t) 10)));
    return false;
}

/*
  Function monomorph: detectType[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_43 mono_67(mono_42* buf) {
    au_bool_t _t321 = mono_98(buf, au_make_span_from_string("cachyos", 7));
    if (_t321) {
        return ((mono_43) { .tag = mono_43_tag_CachyMode, .data = { .CachyMode = {  } } });
    } else {
        au_bool_t _t322 = mono_98(buf, au_make_span_from_string("gentoo", 6));
        if (_t322) {
            return ((mono_43) { .tag = mono_43_tag_GentooMode, .data = { .GentooMode = {  } } });
        } else {
            au_bool_t _t323 = mono_98(buf, au_make_span_from_string("bedrock", 7));
            if (_t323) {
                return ((mono_43) { .tag = mono_43_tag_BedrockMode, .data = { .BedrockMode = {  } } });
            } else {
                return ((mono_43) { .tag = mono_43_tag_OtherMode, .data = { .OtherMode = {  } } });
            }
        }
    }
}

/*
  Function monomorph: copyPrettyName[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_68(mono_42* out, mono_42* src) {
    au_span_t quoted;
    quoted = au_make_span_from_string("PRETTY_NAME=\"", 13);
    au_span_t plain;
    plain = au_make_span_from_string("PRETTY_NAME=", 12);
    au_index_t n;
    n = mono_45(src);
    au_bool_t _t335 = (n == ((au_index_t) 0));
    if (_t335) {
        return false;
    } else {
    }
    au_index_t _t325 = ((au_index_t) 0);
    au_index_t _t326 = meth_34(((au_index_t) n), ((au_index_t) ((au_index_t) 1)));
    {
        size_t pos = _t325;
        for (; pos <= _t326; pos++) {
            au_bool_t _t331 = mono_46(src, ((au_index_t) pos), quoted);
            if (_t331) {
                au_index_t i;
                i = meth_33(((au_index_t) pos), ((au_index_t) mono_20(quoted)));
                mono_42* out_tmp_ref273 = out;
                mono_74(out_tmp_ref273, au_make_span_from_string("", 0));
                au_bool_t _t332 = (i < n);
                while (_t332) {
                    au_nat8_t byte;
                    byte = mono_49(src, ((au_index_t) i));
                    au_bool_t _t334 = ((byte == 34) || (byte == 10));
                    if (_t334) {
                        return true;
                    } else {
                    }
                    mono_42* out_tmp_ref274 = out;
                    mono_76(out_tmp_ref274, ((au_nat8_t) byte));
                    au_index_t _t333 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                    i = _t333;
                    _t332 = (i < n);
                }
                return true;
            } else {
            }
            au_bool_t _t327 = mono_46(src, ((au_index_t) pos), plain);
            if (_t327) {
                au_index_t i;
                i = meth_33(((au_index_t) pos), ((au_index_t) mono_20(plain)));
                mono_42* out_tmp_ref275 = out;
                mono_74(out_tmp_ref275, au_make_span_from_string("", 0));
                au_bool_t _t328 = (i < n);
                while (_t328) {
                    au_nat8_t byte;
                    byte = mono_49(src, ((au_index_t) i));
                    au_bool_t _t330 = (byte == 10);
                    if (_t330) {
                        return true;
                    } else {
                    }
                    mono_42* out_tmp_ref276 = out;
                    mono_76(out_tmp_ref276, ((au_nat8_t) byte));
                    au_index_t _t329 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                    i = _t329;
                    _t328 = (i < n);
                }
                return true;
            } else {
            }
        }
    }
    return false;
}

/*
  Function monomorph: assignLinux[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_69(mono_42* out) {
    mono_42* out_tmp_ref271 = out;
    mono_74(out_tmp_ref271, au_make_span_from_string("Linux", 5));
    return false;
}

/*
  Function monomorph: data[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_nat8_t* mono_70(mono_42* buf) {
    mono_42** buf_tmp_ref70 = &(buf);
    return *&((*buf_tmp_ref70)->ptr);
}

/*
  Function monomorph: assignCString[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_71(mono_42* buf, au_nat8_t* str) {
    mono_42* buf_tmp_ref107 = buf;
    mono_48(buf_tmp_ref107);
    mono_42* buf_tmp_ref108 = buf;
    mono_90(buf_tmp_ref108, str);
    return false;
}

/*
  Function monomorph: assignUnknown[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_72(mono_42* out) {
    mono_42* out_tmp_ref270 = out;
    mono_74(out_tmp_ref270, au_make_span_from_string("Unknown", 7));
    return false;
}

/*
  Function monomorph: parseUnsignedPrefix[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_10 mono_73(mono_42* buf) {
    au_index_t n;
    n = mono_45(buf);
    au_index_t pos;
    pos = ((au_index_t) 0);
    au_index_t value;
    value = ((au_index_t) 0);
    au_bool_t saw_digit;
    saw_digit = false;
    au_bool_t _t495 = (pos < n);
    while (_t495) {
        au_nat8_t byte;
        byte = mono_49(buf, ((au_index_t) pos));
        au_bool_t _t496 = ((byte >= 48) && (byte <= 57));
        if (_t496) {
            au_bool_t _t500 = true;
            saw_digit = _t500;
            au_index_t _t499 = meth_33(((au_index_t) meth_35(((au_index_t) value), ((au_index_t) ((au_index_t) 10)))), ((au_index_t) decl_486(((au_nat8_t) byte))));
            value = _t499;
            au_index_t _t498 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
            pos = _t498;
        } else {
            au_bool_t _t497 = saw_digit;
            if (_t497) {
                return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
            } else {
                return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
            }
        }
        _t495 = (pos < n);
    }
    au_bool_t _t494 = saw_digit;
    if (_t494) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Function monomorph: assignSpan[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_74(mono_42* buf, au_span_t bytes) {
    mono_42* buf_tmp_ref105 = buf;
    mono_48(buf_tmp_ref105);
    mono_42* buf_tmp_ref106 = buf;
    mono_65(buf_tmp_ref106, bytes);
    return false;
}

/*
  Function monomorph: appendIndexDecimal[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_75(mono_42* buf, au_index_t value) {
    au_bool_t _t207 = (value == ((au_index_t) 0));
    if (_t207) {
        mono_42* buf_tmp_ref111 = buf;
        mono_76(buf_tmp_ref111, ((au_nat8_t) 48));
    } else {
        mono_42* buf_tmp_ref112 = buf;
        mono_99(buf_tmp_ref112, ((au_index_t) value));
    }
    return false;
}

/*
  Function monomorph: appendByte[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_76(mono_42* buf, au_nat8_t byte) {
    mono_42** buf_tmp_ref80 = &(buf);
    mono_42** buf_tmp_ref81 = &(buf);
    au_bool_t _t195 = (meth_33(((au_index_t) *&((*buf_tmp_ref80)->len)), ((au_index_t) ((au_index_t) 1))) >= *&((*buf_tmp_ref81)->cap));
    if (_t195) {
        decl_9(au_make_span_from_string("ByteBuf.appendByte: buffer full", 31));
    } else {
    }
    mono_42** buf_tmp_ref82 = &(buf);
    mono_42** buf_tmp_ref83 = &(buf);
    mono_32(mono_31(*&((*buf_tmp_ref82)->ptr), ((au_index_t) *&((*buf_tmp_ref83)->len))), ((au_nat8_t) byte));
    mono_42** buf_tmp_ref84 = &(buf);
    au_index_t _t194 = meth_33(((au_index_t) *&((*buf_tmp_ref84)->len)), ((au_index_t) ((au_index_t) 1)));
    mono_42** buf_tmp_ref85 = &(buf);
    *&((*buf_tmp_ref85)->len) = _t194;
    mono_42** buf_tmp_ref86 = &(buf);
    mono_42** buf_tmp_ref87 = &(buf);
    mono_32(mono_31(*&((*buf_tmp_ref86)->ptr), ((au_index_t) *&((*buf_tmp_ref87)->len))), ((au_nat8_t) ((au_nat8_t) 0)));
    return false;
}

/*
  Function monomorph: findNumberAfterKey[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_10 mono_77(mono_42* buf, au_span_t key) {
    au_index_t n;
    n = mono_45(buf);
    au_index_t key_len;
    key_len = mono_20(key);
    au_bool_t _t513 = (key_len > n);
    if (_t513) {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    } else {
    }
    au_index_t _t501 = ((au_index_t) 0);
    au_index_t _t502 = meth_34(((au_index_t) n), ((au_index_t) key_len));
    {
        size_t pos = _t501;
        for (; pos <= _t502; pos++) {
            au_bool_t _t503 = mono_46(buf, ((au_index_t) pos), key);
            if (_t503) {
                au_index_t i;
                i = meth_33(((au_index_t) pos), ((au_index_t) key_len));
                au_index_t value;
                value = ((au_index_t) 0);
                au_bool_t saw_digit;
                saw_digit = false;
                au_bool_t _t505 = (i < n);
                while (_t505) {
                    au_nat8_t byte;
                    byte = mono_49(buf, ((au_index_t) i));
                    au_bool_t _t506 = ((byte == 32) || (byte == 9));
                    if (_t506) {
                        au_index_t _t512 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                        i = _t512;
                    } else {
                        au_bool_t _t507 = ((byte >= 48) && (byte <= 57));
                        if (_t507) {
                            au_bool_t _t511 = true;
                            saw_digit = _t511;
                            au_index_t _t510 = meth_33(((au_index_t) meth_35(((au_index_t) value), ((au_index_t) ((au_index_t) 10)))), ((au_index_t) decl_486(((au_nat8_t) byte))));
                            value = _t510;
                            au_index_t _t509 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                            i = _t509;
                        } else {
                            au_bool_t _t508 = saw_digit;
                            if (_t508) {
                                return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
                            } else {
                                return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
                            }
                        }
                    }
                    _t505 = (i < n);
                }
                au_bool_t _t504 = saw_digit;
                if (_t504) {
                    return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
                } else {
                    return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
                }
            } else {
            }
        }
    }
    return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
}

/*
  Function monomorph: appendGiB2[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_78(mono_42* out, au_index_t kib) {
    au_index_t hundredths;
    hundredths = meth_36(((au_index_t) meth_33(((au_index_t) meth_35(((au_index_t) kib), ((au_index_t) ((au_index_t) 100)))), ((au_index_t) ((au_index_t) 524288)))), ((au_index_t) ((au_index_t) 1048576)));
    mono_42* out_tmp_ref319 = out;
    mono_75(out_tmp_ref319, ((au_index_t) meth_36(((au_index_t) hundredths), ((au_index_t) ((au_index_t) 100)))));
    mono_42* out_tmp_ref320 = out;
    mono_76(out_tmp_ref320, ((au_nat8_t) 46));
    mono_42* out_tmp_ref321 = out;
    mono_85(out_tmp_ref321, ((au_index_t) meth_283(hundredths, ((au_index_t) 100))));
    mono_42* out_tmp_ref322 = out;
    mono_65(out_tmp_ref322, au_make_span_from_string(" GiB", 4));
    return false;
}

/*
  Function monomorph: buildProcPath[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_79(mono_42* out, au_index_t pid, au_span_t tail) {
    mono_42* out_tmp_ref325 = out;
    mono_74(out_tmp_ref325, au_make_span_from_string("/proc/", 6));
    mono_42* out_tmp_ref326 = out;
    mono_75(out_tmp_ref326, ((au_index_t) pid));
    mono_42* out_tmp_ref327 = out;
    mono_65(out_tmp_ref327, tail);
    return false;
}

/*
  Function monomorph: commIsShell[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_80(mono_42* buf) {
    au_bool_t _t347 = mono_100(buf, au_make_span_from_string("bash", 4));
    if (_t347) {
        return true;
    } else {
        au_bool_t _t348 = mono_100(buf, au_make_span_from_string("zsh", 3));
        if (_t348) {
            return true;
        } else {
            au_bool_t _t349 = mono_100(buf, au_make_span_from_string("fish", 4));
            if (_t349) {
                return true;
            } else {
                au_bool_t _t350 = mono_100(buf, au_make_span_from_string("sh", 2));
                if (_t350) {
                    return true;
                } else {
                    return false;
                }
            }
        }
    }
}

/*
  Function monomorph: parseProcStatParentPid[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_10 mono_81(mono_42* buf) {
    au_index_t n;
    n = mono_45(buf);
    au_bool_t _t548 = (n == ((au_index_t) 0));
    if (_t548) {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    } else {
    }
    au_index_t pos;
    pos = n;
    au_bool_t found;
    found = false;
    au_bool_t _t543 = ((pos > ((au_index_t) 0)) && !(found));
    while (_t543) {
        au_index_t _t547 = meth_34(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
        pos = _t547;
        au_bool_t _t544 = (mono_49(buf, ((au_index_t) pos)) == 41);
        if (_t544) {
            au_bool_t _t546 = true;
            found = _t546;
            au_index_t _t545 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
            pos = _t545;
        } else {
        }
        _t543 = ((pos > ((au_index_t) 0)) && !(found));
    }
    au_bool_t _t542 = !(found);
    if (_t542) {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    } else {
    }
    au_bool_t _t541 = (meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 2))) >= n);
    if (_t541) {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    } else {
    }
    au_index_t _t540 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 2)));
    pos = _t540;
    au_bool_t _t538 = ((pos < n) && (mono_49(buf, ((au_index_t) pos)) == 32));
    while (_t538) {
        au_index_t _t539 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
        pos = _t539;
        _t538 = ((pos < n) && (mono_49(buf, ((au_index_t) pos)) == 32));
    }
    au_index_t value;
    value = ((au_index_t) 0);
    au_bool_t saw_digit;
    saw_digit = false;
    au_bool_t _t532 = (pos < n);
    while (_t532) {
        au_nat8_t byte;
        byte = mono_49(buf, ((au_index_t) pos));
        au_bool_t _t533 = ((byte >= 48) && (byte <= 57));
        if (_t533) {
            au_bool_t _t537 = true;
            saw_digit = _t537;
            au_index_t _t536 = meth_33(((au_index_t) meth_35(((au_index_t) value), ((au_index_t) ((au_index_t) 10)))), ((au_index_t) decl_486(((au_nat8_t) byte))));
            value = _t536;
            au_index_t _t535 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
            pos = _t535;
        } else {
            au_bool_t _t534 = saw_digit;
            if (_t534) {
                return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
            } else {
                return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
            }
        }
        _t532 = (pos < n);
    }
    au_bool_t _t531 = saw_digit;
    if (_t531) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Function monomorph: assignLine[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_82(mono_42* out, mono_42* src) {
    au_index_t n;
    n = mono_101(src);
    mono_42* out_tmp_ref278 = out;
    mono_74(out_tmp_ref278, au_make_span_from_string("", 0));
    au_bool_t _t344 = (n > ((au_index_t) 0));
    if (_t344) {
        au_index_t _t345 = ((au_index_t) 0);
        au_index_t _t346 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t345;
            for (; i <= _t346; i++) {
                mono_42* out_tmp_ref279 = out;
                mono_76(out_tmp_ref279, ((au_nat8_t) mono_49(src, ((au_index_t) i))));
            }
        }
    } else {
    }
    return false;
}

/*
  Function monomorph: copyFieldValueAfterKey[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_83(mono_42* out, mono_42* src, au_span_t key) {
    au_index_t n;
    n = mono_45(src);
    au_index_t key_len;
    key_len = mono_20(key);
    au_bool_t _t368 = (key_len > n);
    if (_t368) {
        return false;
    } else {
    }
    au_index_t _t356 = ((au_index_t) 0);
    au_index_t _t357 = meth_34(((au_index_t) n), ((au_index_t) key_len));
    {
        size_t pos = _t356;
        for (; pos <= _t357; pos++) {
            au_bool_t _t358 = mono_46(src, ((au_index_t) pos), key);
            if (_t358) {
                au_index_t i;
                i = meth_33(((au_index_t) pos), ((au_index_t) key_len));
                au_bool_t _t366 = ((i < n) && (mono_49(src, ((au_index_t) i)) != 58));
                while (_t366) {
                    au_index_t _t367 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                    i = _t367;
                    _t366 = ((i < n) && (mono_49(src, ((au_index_t) i)) != 58));
                }
                au_bool_t _t365 = (i >= n);
                if (_t365) {
                    return false;
                } else {
                }
                au_index_t _t364 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                i = _t364;
                au_bool_t _t362 = ((i < n) && (mono_49(src, ((au_index_t) i)) == 32));
                while (_t362) {
                    au_index_t _t363 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                    i = _t363;
                    _t362 = ((i < n) && (mono_49(src, ((au_index_t) i)) == 32));
                }
                mono_42* out_tmp_ref281 = out;
                mono_74(out_tmp_ref281, au_make_span_from_string("", 0));
                au_bool_t _t359 = (i < n);
                while (_t359) {
                    au_nat8_t byte;
                    byte = mono_49(src, ((au_index_t) i));
                    au_bool_t _t361 = (byte == 10);
                    if (_t361) {
                        return true;
                    } else {
                    }
                    mono_42* out_tmp_ref282 = out;
                    mono_76(out_tmp_ref282, ((au_nat8_t) byte));
                    au_index_t _t360 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
                    i = _t360;
                    _t359 = (i < n);
                }
                return true;
            } else {
            }
        }
    }
    return false;
}

/*
  Function monomorph: cleanCpuModel[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_84(mono_42* out, mono_42* src) {
    au_index_t pos;
    pos = ((au_index_t) 0);
    au_bool_t need_space;
    need_space = false;
    au_index_t out_len;
    out_len = ((au_index_t) 0);
    mono_42* out_tmp_ref283 = out;
    mono_74(out_tmp_ref283, au_make_span_from_string("", 0));
    au_bool_t _t369 = (pos < mono_45(src));
    while (_t369) {
        au_nat8_t byte;
        byte = mono_49(src, ((au_index_t) pos));
        au_bool_t _t370 = (byte == 64);
        if (_t370) {
            au_index_t _t415 = mono_45(src);
            pos = _t415;
        } else {
            au_bool_t _t371 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Six-Core", 8));
            if (_t371) {
                au_index_t _t414 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Six-Core", 8))));
                pos = _t414;
            } else {
                au_bool_t _t372 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Eight-Core", 10));
                if (_t372) {
                    au_index_t _t413 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Eight-Core", 10))));
                    pos = _t413;
                } else {
                    au_bool_t _t373 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Quad-Core", 9));
                    if (_t373) {
                        au_index_t _t412 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Quad-Core", 9))));
                        pos = _t412;
                    } else {
                        au_bool_t _t374 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Twelve-Core", 11));
                        if (_t374) {
                            au_index_t _t411 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Twelve-Core", 11))));
                            pos = _t411;
                        } else {
                            au_bool_t _t375 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Sixteen-Core", 12));
                            if (_t375) {
                                au_index_t _t410 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Sixteen-Core", 12))));
                                pos = _t410;
                            } else {
                                au_bool_t _t376 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("24-Core", 7));
                                if (_t376) {
                                    au_index_t _t409 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("24-Core", 7))));
                                    pos = _t409;
                                } else {
                                    au_bool_t _t377 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("32-Core", 7));
                                    if (_t377) {
                                        au_index_t _t408 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("32-Core", 7))));
                                        pos = _t408;
                                    } else {
                                        au_bool_t _t378 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("64-Core", 7));
                                        if (_t378) {
                                            au_index_t _t407 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("64-Core", 7))));
                                            pos = _t407;
                                        } else {
                                            au_bool_t _t379 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("6-Core", 6));
                                            if (_t379) {
                                                au_index_t _t406 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("6-Core", 6))));
                                                pos = _t406;
                                            } else {
                                                au_bool_t _t380 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("8-Core", 6));
                                                if (_t380) {
                                                    au_index_t _t405 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("8-Core", 6))));
                                                    pos = _t405;
                                                } else {
                                                    au_bool_t _t381 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("12-Core", 7));
                                                    if (_t381) {
                                                        au_index_t _t404 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("12-Core", 7))));
                                                        pos = _t404;
                                                    } else {
                                                        au_bool_t _t382 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("16-Core", 7));
                                                        if (_t382) {
                                                            au_index_t _t403 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("16-Core", 7))));
                                                            pos = _t403;
                                                        } else {
                                                            au_bool_t _t383 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("-Core", 5));
                                                            if (_t383) {
                                                                au_index_t _t402 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("-Core", 5))));
                                                                pos = _t402;
                                                            } else {
                                                                au_bool_t _t384 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Core", 4));
                                                                if (_t384) {
                                                                    au_index_t _t401 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Core", 4))));
                                                                    pos = _t401;
                                                                } else {
                                                                    au_bool_t _t385 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("Processor", 9));
                                                                    if (_t385) {
                                                                        au_index_t _t400 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("Processor", 9))));
                                                                        pos = _t400;
                                                                    } else {
                                                                        au_bool_t _t386 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("with Radeon Graphics", 20));
                                                                        if (_t386) {
                                                                            au_index_t _t399 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("with Radeon Graphics", 20))));
                                                                            pos = _t399;
                                                                        } else {
                                                                            au_bool_t _t387 = mono_102(src, ((au_index_t) pos), au_make_span_from_string("with Graphics", 13));
                                                                            if (_t387) {
                                                                                au_index_t _t398 = meth_33(((au_index_t) pos), ((au_index_t) mono_20(au_make_span_from_string("with Graphics", 13))));
                                                                                pos = _t398;
                                                                            } else {
                                                                                au_bool_t _t388 = (byte == 32);
                                                                                if (_t388) {
                                                                                    au_bool_t _t397 = (out_len > ((au_index_t) 0));
                                                                                    need_space = _t397;
                                                                                    au_index_t _t396 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                                                                                    pos = _t396;
                                                                                    au_bool_t _t394 = ((pos < mono_45(src)) && (mono_49(src, ((au_index_t) pos)) == 32));
                                                                                    while (_t394) {
                                                                                        au_index_t _t395 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                                                                                        pos = _t395;
                                                                                        _t394 = ((pos < mono_45(src)) && (mono_49(src, ((au_index_t) pos)) == 32));
                                                                                    }
                                                                                } else {
                                                                                    au_bool_t _t391 = need_space;
                                                                                    if (_t391) {
                                                                                        mono_42* out_tmp_ref284 = out;
                                                                                        mono_76(out_tmp_ref284, ((au_nat8_t) 32));
                                                                                        au_index_t _t393 = meth_33(((au_index_t) out_len), ((au_index_t) ((au_index_t) 1)));
                                                                                        out_len = _t393;
                                                                                        au_bool_t _t392 = false;
                                                                                        need_space = _t392;
                                                                                    } else {
                                                                                    }
                                                                                    mono_42* out_tmp_ref285 = out;
                                                                                    mono_76(out_tmp_ref285, ((au_nat8_t) byte));
                                                                                    au_index_t _t390 = meth_33(((au_index_t) out_len), ((au_index_t) ((au_index_t) 1)));
                                                                                    out_len = _t390;
                                                                                    au_index_t _t389 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                                                                                    pos = _t389;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        _t369 = (pos < mono_45(src));
    }
    return false;
}

/*
  Function monomorph: appendTwoDigits[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_85(mono_42* out, au_index_t value) {
    au_bool_t _t514 = (value < ((au_index_t) 10));
    if (_t514) {
        mono_42* out_tmp_ref317 = out;
        mono_76(out_tmp_ref317, ((au_nat8_t) 48));
    } else {
    }
    mono_42* out_tmp_ref318 = out;
    mono_75(out_tmp_ref318, ((au_index_t) value));
    return false;
}

/*
  Function monomorph: buildDrmPath[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_86(mono_42* out, au_index_t card, au_span_t tail) {
    mono_42* out_tmp_ref286 = out;
    mono_74(out_tmp_ref286, au_make_span_from_string("/sys/class/drm/card", 19));
    mono_42* out_tmp_ref287 = out;
    mono_75(out_tmp_ref287, ((au_index_t) card));
    mono_42* out_tmp_ref288 = out;
    mono_65(out_tmp_ref288, tail);
    return false;
}

/*
  Function monomorph: parseHexPrefix[(MonoType.MonoRegionTy (Region.Region 0))]
*/
mono_10 mono_87(mono_42* buf) {
    au_index_t n;
    n = mono_45(buf);
    au_index_t pos;
    pos = ((au_index_t) 0);
    au_index_t value;
    value = ((au_index_t) 0);
    au_bool_t saw_digit;
    saw_digit = false;
    au_bool_t _t426 = (n >= ((au_index_t) 2));
    if (_t426) {
        au_bool_t _t427 = (mono_49(buf, ((au_index_t) ((au_index_t) 0))) == 48);
        if (_t427) {
            au_bool_t _t428 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) ((au_index_t) 1))))) == 120);
            if (_t428) {
                au_index_t _t429 = ((au_index_t) 2);
                pos = _t429;
            } else {
            }
        } else {
        }
    } else {
    }
    au_bool_t _t420 = (pos < n);
    while (_t420) {
        mono_10 _t421 = decl_502(((au_nat8_t) mono_49(buf, ((au_index_t) pos))));
        mono_10 tmp49 = _t421;
        switch ((tmp49).tag) {
            case mono_10_tag_Some:
                {
                    au_index_t digit = (((tmp49).data).Some).value;
                    au_bool_t _t424 = true;
                    saw_digit = _t424;
                    au_index_t _t423 = meth_33(((au_index_t) meth_35(((au_index_t) value), ((au_index_t) ((au_index_t) 16)))), ((au_index_t) digit));
                    value = _t423;
                    au_index_t _t422 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                    pos = _t422;
                }
                break;
            case mono_10_tag_None:
                {
                    au_bool_t _t425 = saw_digit;
                    if (_t425) {
                        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
                    } else {
                        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
                    }
                }
                break;
        }
        _t420 = (pos < n);
    }
    au_bool_t _t419 = saw_digit;
    if (_t419) {
        return ((mono_10) { .tag = mono_10_tag_Some, .data = { .Some = { .value = value } } });
    } else {
        return ((mono_10) { .tag = mono_10_tag_None, .data = { .None = {  } } });
    }
}

/*
  Function monomorph: lookupPciName[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_88(mono_42* out, au_index_t vendor, au_index_t device) {
    mono_42 raw;
    raw = decl_446(((au_index_t) ((au_index_t) 4194304)));
    au_bool_t ok;
    ok = false;
    au_bool_t loaded;
    loaded = false;
    mono_42* raw_tmp_ref293 = &(raw);
    au_bool_t _t490 = mono_44(au_make_span_from_string("/usr/share/hwdata/pci.ids", 25), raw_tmp_ref293);
    if (_t490) {
        au_bool_t _t493 = true;
        loaded = _t493;
    } else {
        mono_42* raw_tmp_ref294 = &(raw);
        au_bool_t _t491 = mono_44(au_make_span_from_string("/usr/share/misc/pci.ids", 23), raw_tmp_ref294);
        if (_t491) {
            au_bool_t _t492 = true;
            loaded = _t492;
        } else {
        }
    }
    au_bool_t _t458 = loaded;
    if (_t458) {
        au_index_t n;
        mono_42* raw_tmp_ref295 = &(raw);
        n = mono_45(raw_tmp_ref295);
        au_index_t pos;
        pos = ((au_index_t) 0);
        au_bool_t vendor_found;
        vendor_found = false;
        au_bool_t _t486 = ((pos < n) && !(vendor_found));
        while (_t486) {
            mono_42* raw_tmp_ref296 = &(raw);
            mono_42* raw_tmp_ref297 = &(raw);
            au_bool_t _t487 = (((pos == ((au_index_t) 0)) || (mono_49(raw_tmp_ref296, ((au_index_t) meth_34(((au_index_t) pos), ((au_index_t) ((au_index_t) 1))))) == 10)) && mono_103(raw_tmp_ref297, ((au_index_t) pos), ((au_index_t) vendor)));
            if (_t487) {
                au_bool_t _t489 = true;
                vendor_found = _t489;
            } else {
                au_index_t _t488 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                pos = _t488;
            }
            _t486 = ((pos < n) && !(vendor_found));
        }
        au_bool_t _t459 = vendor_found;
        if (_t459) {
            mono_42* raw_tmp_ref298 = &(raw);
            au_bool_t _t484 = ((pos < n) && (mono_49(raw_tmp_ref298, ((au_index_t) pos)) != 10));
            while (_t484) {
                au_index_t _t485 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                pos = _t485;
                mono_42* raw_tmp_ref299 = &(raw);
                _t484 = ((pos < n) && (mono_49(raw_tmp_ref299, ((au_index_t) pos)) != 10));
            }
            au_bool_t _t482 = (pos < n);
            if (_t482) {
                au_index_t _t483 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                pos = _t483;
            } else {
            }
            au_bool_t _t460 = (pos < n);
            while (_t460) {
                mono_42* raw_tmp_ref300 = &(raw);
                au_bool_t _t461 = (mono_49(raw_tmp_ref300, ((au_index_t) pos)) != 9);
                if (_t461) {
                    mono_42* raw_tmp_ref301 = &(raw);
                    au_bool_t _t480 = ((pos < n) && (mono_49(raw_tmp_ref301, ((au_index_t) pos)) != 10));
                    while (_t480) {
                        au_index_t _t481 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                        pos = _t481;
                        mono_42* raw_tmp_ref302 = &(raw);
                        _t480 = ((pos < n) && (mono_49(raw_tmp_ref302, ((au_index_t) pos)) != 10));
                    }
                    au_bool_t _t478 = (pos < n);
                    if (_t478) {
                        au_index_t _t479 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                        pos = _t479;
                    } else {
                    }
                } else {
                    mono_42* raw_tmp_ref303 = &(raw);
                    au_bool_t _t462 = ((meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1))) < n) && (mono_49(raw_tmp_ref303, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1))))) == 9));
                    if (_t462) {
                        mono_42* raw_tmp_ref304 = &(raw);
                        au_bool_t _t476 = ((pos < n) && (mono_49(raw_tmp_ref304, ((au_index_t) pos)) != 10));
                        while (_t476) {
                            au_index_t _t477 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                            pos = _t477;
                            mono_42* raw_tmp_ref305 = &(raw);
                            _t476 = ((pos < n) && (mono_49(raw_tmp_ref305, ((au_index_t) pos)) != 10));
                        }
                        au_bool_t _t474 = (pos < n);
                        if (_t474) {
                            au_index_t _t475 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                            pos = _t475;
                        } else {
                        }
                    } else {
                        mono_42* raw_tmp_ref306 = &(raw);
                        au_bool_t _t463 = mono_103(raw_tmp_ref306, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)))), ((au_index_t) device));
                        if (_t463) {
                            au_index_t start;
                            start = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 5)));
                            mono_42* raw_tmp_ref307 = &(raw);
                            mono_42* raw_tmp_ref308 = &(raw);
                            au_bool_t _t472 = ((start < n) && ((mono_49(raw_tmp_ref307, ((au_index_t) start)) == 32) || (mono_49(raw_tmp_ref308, ((au_index_t) start)) == 9)));
                            while (_t472) {
                                au_index_t _t473 = meth_33(((au_index_t) start), ((au_index_t) ((au_index_t) 1)));
                                start = _t473;
                                mono_42* raw_tmp_ref309 = &(raw);
                                mono_42* raw_tmp_ref310 = &(raw);
                                _t472 = ((start < n) && ((mono_49(raw_tmp_ref309, ((au_index_t) start)) == 32) || (mono_49(raw_tmp_ref310, ((au_index_t) start)) == 9)));
                            }
                            au_index_t stop;
                            stop = start;
                            mono_42* raw_tmp_ref311 = &(raw);
                            au_bool_t _t470 = ((stop < n) && (mono_49(raw_tmp_ref311, ((au_index_t) stop)) != 10));
                            while (_t470) {
                                au_index_t _t471 = meth_33(((au_index_t) stop), ((au_index_t) ((au_index_t) 1)));
                                stop = _t471;
                                mono_42* raw_tmp_ref312 = &(raw);
                                _t470 = ((stop < n) && (mono_49(raw_tmp_ref312, ((au_index_t) stop)) != 10));
                            }
                            mono_42* out_tmp_ref313 = out;
                            mono_42* raw_tmp_ref314 = &(raw);
                            mono_104(out_tmp_ref313, raw_tmp_ref314, ((au_index_t) start), ((au_index_t) stop), ((au_index_t) vendor));
                            au_bool_t _t469 = true;
                            ok = _t469;
                            au_index_t _t468 = n;
                            pos = _t468;
                        } else {
                            mono_42* raw_tmp_ref315 = &(raw);
                            au_bool_t _t466 = ((pos < n) && (mono_49(raw_tmp_ref315, ((au_index_t) pos)) != 10));
                            while (_t466) {
                                au_index_t _t467 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                                pos = _t467;
                                mono_42* raw_tmp_ref316 = &(raw);
                                _t466 = ((pos < n) && (mono_49(raw_tmp_ref316, ((au_index_t) pos)) != 10));
                            }
                            au_bool_t _t464 = (pos < n);
                            if (_t464) {
                                au_index_t _t465 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                                pos = _t465;
                            } else {
                            }
                        }
                    }
                }
                _t460 = (pos < n);
            }
        } else {
        }
    } else {
    }
    decl_447(raw);
    return ok;
}

/*
  Function monomorph: appendHex4[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_89(mono_42* buf, au_nat32_t value) {
    au_nat32_t s12;
    s12 = meth_101(meth_20(((au_nat32_t) value), ((au_nat32_t) ((au_nat32_t) 4096))), ((au_nat32_t) 15));
    au_nat32_t s8;
    s8 = meth_101(meth_20(((au_nat32_t) value), ((au_nat32_t) ((au_nat32_t) 256))), ((au_nat32_t) 15));
    au_nat32_t s4;
    s4 = meth_101(meth_20(((au_nat32_t) value), ((au_nat32_t) ((au_nat32_t) 16))), ((au_nat32_t) 15));
    au_nat32_t s0;
    s0 = meth_101(value, ((au_nat32_t) 15));
    mono_2 _t211 = meth_156(s12);
    mono_2 tmp53 = _t211;
    switch ((tmp53).tag) {
        case mono_2_tag_Some:
            {
                au_nat8_t d = (((tmp53).data).Some).value;
                mono_42* buf_tmp_ref115 = buf;
                mono_76(buf_tmp_ref115, ((au_nat8_t) decl_468(((au_nat8_t) d))));
            }
            break;
        case mono_2_tag_None:
            {
                decl_9(au_make_span_from_string("appendHex4: conversion failed", 29));
            }
            break;
    }
    mono_2 _t210 = meth_156(s8);
    mono_2 tmp52 = _t210;
    switch ((tmp52).tag) {
        case mono_2_tag_Some:
            {
                au_nat8_t d = (((tmp52).data).Some).value;
                mono_42* buf_tmp_ref116 = buf;
                mono_76(buf_tmp_ref116, ((au_nat8_t) decl_468(((au_nat8_t) d))));
            }
            break;
        case mono_2_tag_None:
            {
                decl_9(au_make_span_from_string("appendHex4: conversion failed", 29));
            }
            break;
    }
    mono_2 _t209 = meth_156(s4);
    mono_2 tmp51 = _t209;
    switch ((tmp51).tag) {
        case mono_2_tag_Some:
            {
                au_nat8_t d = (((tmp51).data).Some).value;
                mono_42* buf_tmp_ref117 = buf;
                mono_76(buf_tmp_ref117, ((au_nat8_t) decl_468(((au_nat8_t) d))));
            }
            break;
        case mono_2_tag_None:
            {
                decl_9(au_make_span_from_string("appendHex4: conversion failed", 29));
            }
            break;
    }
    mono_2 _t208 = meth_156(s0);
    mono_2 tmp50 = _t208;
    switch ((tmp50).tag) {
        case mono_2_tag_Some:
            {
                au_nat8_t d = (((tmp50).data).Some).value;
                mono_42* buf_tmp_ref118 = buf;
                mono_76(buf_tmp_ref118, ((au_nat8_t) decl_468(((au_nat8_t) d))));
            }
            break;
        case mono_2_tag_None:
            {
                decl_9(au_make_span_from_string("appendHex4: conversion failed", 29));
            }
            break;
    }
    return false;
}

/*
  Function monomorph: appendCString[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_90(mono_42* buf, au_nat8_t* str) {
    au_index_t pos;
    pos = ((au_index_t) 0);
    au_bool_t reading;
    reading = true;
    au_bool_t _t201 = reading;
    while (_t201) {
        au_nat8_t byte;
        byte = mono_39(mono_31(str, ((au_index_t) pos)));
        au_bool_t _t202 = (byte == ((au_nat8_t) 0));
        if (_t202) {
            au_bool_t _t206 = false;
            reading = _t206;
        } else {
            mono_42** buf_tmp_ref97 = &(buf);
            mono_42** buf_tmp_ref98 = &(buf);
            au_bool_t _t205 = (meth_33(((au_index_t) *&((*buf_tmp_ref97)->len)), ((au_index_t) ((au_index_t) 1))) >= *&((*buf_tmp_ref98)->cap));
            if (_t205) {
                decl_9(au_make_span_from_string("ByteBuf.appendCString: buffer full", 34));
            } else {
            }
            mono_42** buf_tmp_ref99 = &(buf);
            mono_42** buf_tmp_ref100 = &(buf);
            mono_32(mono_31(*&((*buf_tmp_ref99)->ptr), ((au_index_t) *&((*buf_tmp_ref100)->len))), ((au_nat8_t) byte));
            mono_42** buf_tmp_ref101 = &(buf);
            au_index_t _t204 = meth_33(((au_index_t) *&((*buf_tmp_ref101)->len)), ((au_index_t) ((au_index_t) 1)));
            mono_42** buf_tmp_ref102 = &(buf);
            *&((*buf_tmp_ref102)->len) = _t204;
            mono_42** buf_tmp_ref103 = &(buf);
            mono_42** buf_tmp_ref104 = &(buf);
            mono_32(mono_31(*&((*buf_tmp_ref103)->ptr), ((au_index_t) *&((*buf_tmp_ref104)->len))), ((au_nat8_t) ((au_nat8_t) 0)));
            au_index_t _t203 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
            pos = _t203;
        }
        _t201 = reading;
    }
    return false;
}

/*
  Function monomorph: appendPackageCount[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_91(mono_42* out, au_bool_t need_sep, au_index_t count, au_span_t manager) {
    au_bool_t _t574 = (count > ((au_index_t) 0));
    if (_t574) {
        au_bool_t _t575 = need_sep;
        if (_t575) {
            mono_42* out_tmp_ref336 = out;
            mono_65(out_tmp_ref336, au_make_span_from_string(", ", 2));
        } else {
        }
        mono_42* out_tmp_ref337 = out;
        mono_75(out_tmp_ref337, ((au_index_t) count));
        mono_42* out_tmp_ref338 = out;
        mono_65(out_tmp_ref338, au_make_span_from_string(" (", 2));
        mono_42* out_tmp_ref339 = out;
        mono_65(out_tmp_ref339, manager);
        mono_42* out_tmp_ref340 = out;
        mono_76(out_tmp_ref340, ((au_nat8_t) 41));
    } else {
    }
    return false;
}

/*
  Function monomorph: assignBasename[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_92(mono_42* out, au_nat8_t* ptr) {
    au_index_t pos;
    pos = ((au_index_t) 0);
    au_index_t base;
    base = ((au_index_t) 0);
    au_bool_t reading;
    reading = true;
    au_bool_t _t315 = reading;
    while (_t315) {
        au_nat8_t byte;
        byte = mono_39(mono_31(ptr, ((au_index_t) pos)));
        au_bool_t _t316 = (byte == ((au_nat8_t) 0));
        if (_t316) {
            au_bool_t _t320 = false;
            reading = _t320;
        } else {
            au_bool_t _t318 = (byte == 47);
            if (_t318) {
                au_index_t _t319 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
                base = _t319;
            } else {
            }
            au_index_t _t317 = meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1)));
            pos = _t317;
        }
        _t315 = reading;
    }
    mono_42* out_tmp_ref272 = out;
    mono_71(out_tmp_ref272, mono_31(ptr, ((au_index_t) base)));
    return false;
}

/*
  Function monomorph: bedrockArt[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_93(mono_42* out) {
    mono_42* out_tmp_ref503 = out;
    mono_105(out_tmp_ref503, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" ┌──┐", 13));
    mono_42* out_tmp_ref504 = out;
    mono_105(out_tmp_ref504, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" ┌──────────────────────────────────┐ ", 110));
    mono_42* out_tmp_ref505 = out;
    mono_105(out_tmp_ref505, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("┌────┐", 18));
    mono_42* out_tmp_ref506 = out;
    mono_66(out_tmp_ref506);
    mono_42* out_tmp_ref507 = out;
    mono_105(out_tmp_ref507, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref508 = out;
    mono_106(out_tmp_ref508, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6));
    mono_42* out_tmp_ref509 = out;
    mono_105(out_tmp_ref509, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref510 = out;
    mono_105(out_tmp_ref510, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─", 7));
    mono_42* out_tmp_ref511 = out;
    mono_107(out_tmp_ref511, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), ((au_index_t) ((au_index_t) 13)));
    mono_42* out_tmp_ref512 = out;
    mono_105(out_tmp_ref512, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────────────────────│ ", 64));
    mono_42* out_tmp_ref513 = out;
    mono_105(out_tmp_ref513, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│ 境 │", 11));
    mono_42* out_tmp_ref514 = out;
    mono_66(out_tmp_ref514);
    mono_42* out_tmp_ref515 = out;
    mono_105(out_tmp_ref515, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref516 = out;
    mono_106(out_tmp_ref516, ((au_index_t) BfetchAust__Fetch____c_nord0), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref517 = out;
    mono_105(out_tmp_ref517, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref518 = out;
    mono_105(out_tmp_ref518, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──", 10));
    mono_42* out_tmp_ref519 = out;
    mono_108(out_tmp_ref519, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\      ", 9), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref520 = out;
    mono_105(out_tmp_ref520, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────────────────────│ ", 64));
    mono_42* out_tmp_ref521 = out;
    mono_105(out_tmp_ref521, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│    │", 10));
    mono_42* out_tmp_ref522 = out;
    mono_66(out_tmp_ref522);
    mono_42* out_tmp_ref523 = out;
    mono_105(out_tmp_ref523, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref524 = out;
    mono_106(out_tmp_ref524, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref525 = out;
    mono_105(out_tmp_ref525, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref526 = out;
    mono_105(out_tmp_ref526, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │───", 13));
    mono_42* out_tmp_ref527 = out;
    mono_108(out_tmp_ref527, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\      ", 9), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref528 = out;
    mono_105(out_tmp_ref528, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───────────────────│ ", 61));
    mono_42* out_tmp_ref529 = out;
    mono_105(out_tmp_ref529, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│ 界 │", 11));
    mono_42* out_tmp_ref530 = out;
    mono_66(out_tmp_ref530);
    mono_42* out_tmp_ref531 = out;
    mono_105(out_tmp_ref531, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref532 = out;
    mono_106(out_tmp_ref532, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref533 = out;
    mono_105(out_tmp_ref533, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref534 = out;
    mono_105(out_tmp_ref534, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │────", 16));
    mono_42* out_tmp_ref535 = out;
    mono_108(out_tmp_ref535, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\      ", 9), ((au_index_t) ((au_index_t) 17)));
    mono_42* out_tmp_ref536 = out;
    mono_105(out_tmp_ref536, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────│ ", 16));
    mono_42* out_tmp_ref537 = out;
    mono_105(out_tmp_ref537, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("└────┘", 18));
    mono_42* out_tmp_ref538 = out;
    mono_66(out_tmp_ref538);
    mono_42* out_tmp_ref539 = out;
    mono_105(out_tmp_ref539, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref540 = out;
    mono_106(out_tmp_ref540, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref541 = out;
    mono_105(out_tmp_ref541, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref542 = out;
    mono_105(out_tmp_ref542, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref543 = out;
    mono_108(out_tmp_ref543, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\                    ", 23), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref544 = out;
    mono_105(out_tmp_ref544, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───│", 12));
    mono_42* out_tmp_ref545 = out;
    mono_66(out_tmp_ref545);
    mono_42* out_tmp_ref546 = out;
    mono_105(out_tmp_ref546, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref547 = out;
    mono_106(out_tmp_ref547, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref548 = out;
    mono_105(out_tmp_ref548, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref549 = out;
    mono_105(out_tmp_ref549, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──────", 22));
    mono_42* out_tmp_ref550 = out;
    mono_108(out_tmp_ref550, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\                    ", 23), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref551 = out;
    mono_105(out_tmp_ref551, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("──│", 9));
    mono_42* out_tmp_ref552 = out;
    mono_66(out_tmp_ref552);
    mono_42* out_tmp_ref553 = out;
    mono_105(out_tmp_ref553, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref554 = out;
    mono_106(out_tmp_ref554, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref555 = out;
    mono_105(out_tmp_ref555, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref556 = out;
    mono_105(out_tmp_ref556, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │───────", 25));
    mono_42* out_tmp_ref557 = out;
    mono_108(out_tmp_ref557, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\        ──────      ", 35), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref558 = out;
    mono_105(out_tmp_ref558, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─│", 6));
    mono_42* out_tmp_ref559 = out;
    mono_66(out_tmp_ref559);
    mono_42* out_tmp_ref560 = out;
    mono_105(out_tmp_ref560, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref561 = out;
    mono_106(out_tmp_ref561, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref562 = out;
    mono_105(out_tmp_ref562, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref563 = out;
    mono_105(out_tmp_ref563, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │────────", 28));
    mono_42* out_tmp_ref564 = out;
    mono_105(out_tmp_ref564, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\                   ///", 25));
    mono_42* out_tmp_ref565 = out;
    mono_105(out_tmp_ref565, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─│", 6));
    mono_42* out_tmp_ref566 = out;
    mono_66(out_tmp_ref566);
    mono_42* out_tmp_ref567 = out;
    mono_105(out_tmp_ref567, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref568 = out;
    mono_106(out_tmp_ref568, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref569 = out;
    mono_105(out_tmp_ref569, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref570 = out;
    mono_105(out_tmp_ref570, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─────────", 31));
    mono_42* out_tmp_ref571 = out;
    mono_105(out_tmp_ref571, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\                 ///", 23));
    mono_42* out_tmp_ref572 = out;
    mono_105(out_tmp_ref572, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("──│", 9));
    mono_42* out_tmp_ref573 = out;
    mono_66(out_tmp_ref573);
    mono_42* out_tmp_ref574 = out;
    mono_105(out_tmp_ref574, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref575 = out;
    mono_106(out_tmp_ref575, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref576 = out;
    mono_105(out_tmp_ref576, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref577 = out;
    mono_105(out_tmp_ref577, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──────────", 34));
    mono_42* out_tmp_ref578 = out;
    mono_105(out_tmp_ref578, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\               ///", 21));
    mono_42* out_tmp_ref579 = out;
    mono_105(out_tmp_ref579, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───│", 12));
    mono_42* out_tmp_ref580 = out;
    mono_66(out_tmp_ref580);
    mono_42* out_tmp_ref581 = out;
    mono_105(out_tmp_ref581, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref582 = out;
    mono_106(out_tmp_ref582, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref583 = out;
    mono_105(out_tmp_ref583, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref584 = out;
    mono_105(out_tmp_ref584, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │───────────", 37));
    mono_42* out_tmp_ref585 = out;
    mono_105(out_tmp_ref585, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("\\\\\\////////////////", 19));
    mono_42* out_tmp_ref586 = out;
    mono_105(out_tmp_ref586, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────│", 15));
    mono_42* out_tmp_ref587 = out;
    mono_66(out_tmp_ref587);
    mono_42* out_tmp_ref588 = out;
    mono_105(out_tmp_ref588, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref589 = out;
    mono_106(out_tmp_ref589, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref590 = out;
    mono_105(out_tmp_ref590, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref591 = out;
    mono_105(out_tmp_ref591, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" └──────────────────────────────────┘", 109));
    mono_42* out_tmp_ref592 = out;
    mono_66(out_tmp_ref592);
    return false;
}

/*
  Function monomorph: fieldLine[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_94(mono_42* out, au_index_t bar_code, au_span_t bar_text, au_index_t label_code, au_span_t label, mono_42* value) {
    mono_42* out_tmp_ref364 = out;
    mono_105(out_tmp_ref364, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref365 = out;
    mono_106(out_tmp_ref365, ((au_index_t) bar_code), bar_text);
    mono_42* out_tmp_ref366 = out;
    mono_105(out_tmp_ref366, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│ ", 4));
    mono_42* out_tmp_ref367 = out;
    mono_106(out_tmp_ref367, ((au_index_t) label_code), label);
    mono_42* out_tmp_ref368 = out;
    mono_109(out_tmp_ref368, ((au_index_t) BfetchAust__Fetch____c_nord4));
    mono_42* out_tmp_ref369 = out;
    mono_110(out_tmp_ref369, value);
    mono_42* out_tmp_ref370 = out;
    mono_66(out_tmp_ref370);
    return false;
}

/*
  Function monomorph: cachyArt[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_95(mono_42* out) {
    mono_42* out_tmp_ref593 = out;
    mono_105(out_tmp_ref593, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" ┌──┐", 13));
    mono_42* out_tmp_ref594 = out;
    mono_105(out_tmp_ref594, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" ┌──────────────────────────────────┐ ", 110));
    mono_42* out_tmp_ref595 = out;
    mono_105(out_tmp_ref595, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("┌────┐", 18));
    mono_42* out_tmp_ref596 = out;
    mono_66(out_tmp_ref596);
    mono_42* out_tmp_ref597 = out;
    mono_105(out_tmp_ref597, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref598 = out;
    mono_106(out_tmp_ref598, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6));
    mono_42* out_tmp_ref599 = out;
    mono_105(out_tmp_ref599, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref600 = out;
    mono_105(out_tmp_ref600, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref601 = out;
    mono_106(out_tmp_ref601, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref602 = out;
    mono_106(out_tmp_ref602, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref603 = out;
    mono_106(out_tmp_ref603, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++++++++", 10));
    mono_42* out_tmp_ref604 = out;
    mono_106(out_tmp_ref604, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("----", 4));
    mono_42* out_tmp_ref605 = out;
    mono_106(out_tmp_ref605, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref606 = out;
    mono_105(out_tmp_ref606, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───────────│ ", 37));
    mono_42* out_tmp_ref607 = out;
    mono_105(out_tmp_ref607, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│ 境 │", 11));
    mono_42* out_tmp_ref608 = out;
    mono_66(out_tmp_ref608);
    mono_42* out_tmp_ref609 = out;
    mono_105(out_tmp_ref609, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref610 = out;
    mono_106(out_tmp_ref610, ((au_index_t) BfetchAust__Fetch____c_nord0), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref611 = out;
    mono_105(out_tmp_ref611, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref612 = out;
    mono_105(out_tmp_ref612, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │────", 16));
    mono_42* out_tmp_ref613 = out;
    mono_106(out_tmp_ref613, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref614 = out;
    mono_106(out_tmp_ref614, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+++++++++++", 11));
    mono_42* out_tmp_ref615 = out;
    mono_106(out_tmp_ref615, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("----", 4));
    mono_42* out_tmp_ref616 = out;
    mono_106(out_tmp_ref616, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref617 = out;
    mono_105(out_tmp_ref617, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─────", 15));
    mono_42* out_tmp_ref618 = out;
    mono_111(out_tmp_ref618, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref619 = out;
    mono_105(out_tmp_ref619, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────│ ", 16));
    mono_42* out_tmp_ref620 = out;
    mono_105(out_tmp_ref620, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│    │", 10));
    mono_42* out_tmp_ref621 = out;
    mono_66(out_tmp_ref621);
    mono_42* out_tmp_ref622 = out;
    mono_105(out_tmp_ref622, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref623 = out;
    mono_106(out_tmp_ref623, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref624 = out;
    mono_105(out_tmp_ref624, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref625 = out;
    mono_105(out_tmp_ref625, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │───", 13));
    mono_42* out_tmp_ref626 = out;
    mono_106(out_tmp_ref626, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref627 = out;
    mono_106(out_tmp_ref627, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++++++++++++++", 16));
    mono_42* out_tmp_ref628 = out;
    mono_105(out_tmp_ref628, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("──────", 18));
    mono_42* out_tmp_ref629 = out;
    mono_106(out_tmp_ref629, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("\\//", 3));
    mono_42* out_tmp_ref630 = out;
    mono_105(out_tmp_ref630, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────│ ", 16));
    mono_42* out_tmp_ref631 = out;
    mono_105(out_tmp_ref631, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│ 界 │", 11));
    mono_42* out_tmp_ref632 = out;
    mono_66(out_tmp_ref632);
    mono_42* out_tmp_ref633 = out;
    mono_105(out_tmp_ref633, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref634 = out;
    mono_106(out_tmp_ref634, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref635 = out;
    mono_105(out_tmp_ref635, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref636 = out;
    mono_105(out_tmp_ref636, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──", 10));
    mono_42* out_tmp_ref637 = out;
    mono_106(out_tmp_ref637, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref638 = out;
    mono_106(out_tmp_ref638, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++", 2));
    mono_42* out_tmp_ref639 = out;
    mono_106(out_tmp_ref639, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("---", 3));
    mono_42* out_tmp_ref640 = out;
    mono_106(out_tmp_ref640, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+", 1));
    mono_42* out_tmp_ref641 = out;
    mono_106(out_tmp_ref641, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref642 = out;
    mono_105(out_tmp_ref642, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("──────────────────────│ ", 70));
    mono_42* out_tmp_ref643 = out;
    mono_105(out_tmp_ref643, ((au_index_t) BfetchAust__Fetch____c_nord11), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("└────┘", 18));
    mono_42* out_tmp_ref644 = out;
    mono_66(out_tmp_ref644);
    mono_42* out_tmp_ref645 = out;
    mono_105(out_tmp_ref645, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref646 = out;
    mono_106(out_tmp_ref646, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref647 = out;
    mono_105(out_tmp_ref647, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref648 = out;
    mono_105(out_tmp_ref648, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─", 7));
    mono_42* out_tmp_ref649 = out;
    mono_106(out_tmp_ref649, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref650 = out;
    mono_106(out_tmp_ref650, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("---", 3));
    mono_42* out_tmp_ref651 = out;
    mono_106(out_tmp_ref651, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+++", 3));
    mono_42* out_tmp_ref652 = out;
    mono_106(out_tmp_ref652, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref653 = out;
    mono_105(out_tmp_ref653, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────────────", 36));
    mono_42* out_tmp_ref654 = out;
    mono_111(out_tmp_ref654, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/+", 2), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref655 = out;
    mono_105(out_tmp_ref655, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───────│", 24));
    mono_42* out_tmp_ref656 = out;
    mono_66(out_tmp_ref656);
    mono_42* out_tmp_ref657 = out;
    mono_105(out_tmp_ref657, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref658 = out;
    mono_106(out_tmp_ref658, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref659 = out;
    mono_105(out_tmp_ref659, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref660 = out;
    mono_105(out_tmp_ref660, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─", 7));
    mono_42* out_tmp_ref661 = out;
    mono_112(out_tmp_ref661, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref662 = out;
    mono_106(out_tmp_ref662, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++", 4));
    mono_42* out_tmp_ref663 = out;
    mono_106(out_tmp_ref663, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref664 = out;
    mono_106(out_tmp_ref664, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref665 = out;
    mono_105(out_tmp_ref665, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─────────────", 39));
    mono_42* out_tmp_ref666 = out;
    mono_106(out_tmp_ref666, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("\\-//", 4));
    mono_42* out_tmp_ref667 = out;
    mono_105(out_tmp_ref667, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───────│", 24));
    mono_42* out_tmp_ref668 = out;
    mono_66(out_tmp_ref668);
    mono_42* out_tmp_ref669 = out;
    mono_105(out_tmp_ref669, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref670 = out;
    mono_106(out_tmp_ref670, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref671 = out;
    mono_105(out_tmp_ref671, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref672 = out;
    mono_105(out_tmp_ref672, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──", 10));
    mono_42* out_tmp_ref673 = out;
    mono_112(out_tmp_ref673, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref674 = out;
    mono_106(out_tmp_ref674, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref675 = out;
    mono_106(out_tmp_ref675, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+++", 3));
    mono_42* out_tmp_ref676 = out;
    mono_112(out_tmp_ref676, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref677 = out;
    mono_105(out_tmp_ref677, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("──────────────────", 54));
    mono_42* out_tmp_ref678 = out;
    mono_111(out_tmp_ref678, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/++", 3), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref679 = out;
    mono_105(out_tmp_ref679, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─│", 6));
    mono_42* out_tmp_ref680 = out;
    mono_66(out_tmp_ref680);
    mono_42* out_tmp_ref681 = out;
    mono_105(out_tmp_ref681, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref682 = out;
    mono_106(out_tmp_ref682, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref683 = out;
    mono_105(out_tmp_ref683, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref684 = out;
    mono_105(out_tmp_ref684, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │───", 13));
    mono_42* out_tmp_ref685 = out;
    mono_112(out_tmp_ref685, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref686 = out;
    mono_106(out_tmp_ref686, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+++", 3));
    mono_42* out_tmp_ref687 = out;
    mono_106(out_tmp_ref687, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref688 = out;
    mono_112(out_tmp_ref688, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref689 = out;
    mono_105(out_tmp_ref689, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─────────────────", 51));
    mono_42* out_tmp_ref690 = out;
    mono_106(out_tmp_ref690, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("\\--//", 5));
    mono_42* out_tmp_ref691 = out;
    mono_105(out_tmp_ref691, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─│", 6));
    mono_42* out_tmp_ref692 = out;
    mono_66(out_tmp_ref692);
    mono_42* out_tmp_ref693 = out;
    mono_105(out_tmp_ref693, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref694 = out;
    mono_106(out_tmp_ref694, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref695 = out;
    mono_105(out_tmp_ref695, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref696 = out;
    mono_105(out_tmp_ref696, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │────", 16));
    mono_42* out_tmp_ref697 = out;
    mono_112(out_tmp_ref697, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref698 = out;
    mono_106(out_tmp_ref698, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref699 = out;
    mono_106(out_tmp_ref699, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++", 4));
    mono_42* out_tmp_ref700 = out;
    mono_106(out_tmp_ref700, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("-+", 2));
    mono_42* out_tmp_ref701 = out;
    mono_106(out_tmp_ref701, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("---", 3));
    mono_42* out_tmp_ref702 = out;
    mono_106(out_tmp_ref702, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+", 1));
    mono_42* out_tmp_ref703 = out;
    mono_106(out_tmp_ref703, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref704 = out;
    mono_106(out_tmp_ref704, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++++", 6));
    mono_42* out_tmp_ref705 = out;
    mono_106(out_tmp_ref705, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref706 = out;
    mono_105(out_tmp_ref706, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("───────│", 24));
    mono_42* out_tmp_ref707 = out;
    mono_66(out_tmp_ref707);
    mono_42* out_tmp_ref708 = out;
    mono_105(out_tmp_ref708, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref709 = out;
    mono_106(out_tmp_ref709, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref710 = out;
    mono_105(out_tmp_ref710, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref711 = out;
    mono_105(out_tmp_ref711, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref712 = out;
    mono_112(out_tmp_ref712, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref713 = out;
    mono_106(out_tmp_ref713, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref714 = out;
    mono_106(out_tmp_ref714, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("+++++++++++++++", 15));
    mono_42* out_tmp_ref715 = out;
    mono_106(out_tmp_ref715, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("--", 2));
    mono_42* out_tmp_ref716 = out;
    mono_106(out_tmp_ref716, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref717 = out;
    mono_105(out_tmp_ref717, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────────│", 27));
    mono_42* out_tmp_ref718 = out;
    mono_66(out_tmp_ref718);
    mono_42* out_tmp_ref719 = out;
    mono_105(out_tmp_ref719, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref720 = out;
    mono_106(out_tmp_ref720, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref721 = out;
    mono_105(out_tmp_ref721, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref722 = out;
    mono_105(out_tmp_ref722, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │──────", 22));
    mono_42* out_tmp_ref723 = out;
    mono_112(out_tmp_ref723, ((au_index_t) BfetchAust__Fetch____c_nord7), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref724 = out;
    mono_106(out_tmp_ref724, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("-", 1));
    mono_42* out_tmp_ref725 = out;
    mono_106(out_tmp_ref725, ((au_index_t) BfetchAust__Fetch____c_nord4), au_make_span_from_string("++++++++++++", 12));
    mono_42* out_tmp_ref726 = out;
    mono_106(out_tmp_ref726, ((au_index_t) BfetchAust__Fetch____c_nord3), au_make_span_from_string("----", 4));
    mono_42* out_tmp_ref727 = out;
    mono_106(out_tmp_ref727, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref728 = out;
    mono_105(out_tmp_ref728, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("─────────│", 30));
    mono_42* out_tmp_ref729 = out;
    mono_66(out_tmp_ref729);
    mono_42* out_tmp_ref730 = out;
    mono_105(out_tmp_ref730, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref731 = out;
    mono_106(out_tmp_ref731, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref732 = out;
    mono_105(out_tmp_ref732, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref733 = out;
    mono_105(out_tmp_ref733, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" └──────────────────────────────────┘", 109));
    mono_42* out_tmp_ref734 = out;
    mono_66(out_tmp_ref734);
    return false;
}

/*
  Function monomorph: gentooArt[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_96(mono_42* out) {
    mono_42* out_tmp_ref390 = out;
    mono_105(out_tmp_ref390, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" ┌──┐", 13));
    mono_42* out_tmp_ref391 = out;
    mono_106(out_tmp_ref391, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" ┌──────────────────────────────────┐ ", 110));
    mono_42* out_tmp_ref392 = out;
    mono_105(out_tmp_ref392, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("┌─────┐", 21));
    mono_42* out_tmp_ref393 = out;
    mono_66(out_tmp_ref393);
    mono_42* out_tmp_ref394 = out;
    mono_105(out_tmp_ref394, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref395 = out;
    mono_106(out_tmp_ref395, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("▒▒", 6));
    mono_42* out_tmp_ref396 = out;
    mono_105(out_tmp_ref396, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref397 = out;
    mono_106(out_tmp_ref397, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │─────────", 31));
    mono_42* out_tmp_ref398 = out;
    mono_107(out_tmp_ref398, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), ((au_index_t) ((au_index_t) 10)));
    mono_42* out_tmp_ref399 = out;
    mono_106(out_tmp_ref399, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("───────────────│ ", 49));
    mono_42* out_tmp_ref400 = out;
    mono_105(out_tmp_ref400, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  G  │", 11));
    mono_42* out_tmp_ref401 = out;
    mono_66(out_tmp_ref401);
    mono_42* out_tmp_ref402 = out;
    mono_105(out_tmp_ref402, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref403 = out;
    mono_106(out_tmp_ref403, ((au_index_t) BfetchAust__Fetch____c_nord0), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref404 = out;
    mono_105(out_tmp_ref404, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref405 = out;
    mono_106(out_tmp_ref405, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │───────", 25));
    mono_42* out_tmp_ref406 = out;
    mono_108(out_tmp_ref406, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//+++++++++++", 13), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref407 = out;
    mono_106(out_tmp_ref407, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("─────────────│ ", 43));
    mono_42* out_tmp_ref408 = out;
    mono_105(out_tmp_ref408, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  e  │", 11));
    mono_42* out_tmp_ref409 = out;
    mono_66(out_tmp_ref409);
    mono_42* out_tmp_ref410 = out;
    mono_105(out_tmp_ref410, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref411 = out;
    mono_106(out_tmp_ref411, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref412 = out;
    mono_105(out_tmp_ref412, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref413 = out;
    mono_106(out_tmp_ref413, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │──────", 22));
    mono_42* out_tmp_ref414 = out;
    mono_105(out_tmp_ref414, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//+++++", 7));
    mono_42* out_tmp_ref415 = out;
    mono_107(out_tmp_ref415, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), ((au_index_t) ((au_index_t) 3)));
    mono_42* out_tmp_ref416 = out;
    mono_108(out_tmp_ref416, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("+++++", 5), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref417 = out;
    mono_106(out_tmp_ref417, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("────────────│ ", 40));
    mono_42* out_tmp_ref418 = out;
    mono_105(out_tmp_ref418, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  n  │", 11));
    mono_42* out_tmp_ref419 = out;
    mono_66(out_tmp_ref419);
    mono_42* out_tmp_ref420 = out;
    mono_105(out_tmp_ref420, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref421 = out;
    mono_106(out_tmp_ref421, ((au_index_t) BfetchAust__Fetch____c_nord11), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref422 = out;
    mono_105(out_tmp_ref422, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref423 = out;
    mono_106(out_tmp_ref423, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref424 = out;
    mono_105(out_tmp_ref424, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//+++++", 7));
    mono_42* out_tmp_ref425 = out;
    mono_105(out_tmp_ref425, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("// ", 3));
    mono_42* out_tmp_ref426 = out;
    mono_106(out_tmp_ref426, ((au_index_t) BfetchAust__Fetch____c_reset), au_make_span_from_string("/", 1));
    mono_42* out_tmp_ref427 = out;
    mono_108(out_tmp_ref427, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("+++++++", 7), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref428 = out;
    mono_106(out_tmp_ref428, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("──────────│ ", 34));
    mono_42* out_tmp_ref429 = out;
    mono_105(out_tmp_ref429, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  t  │", 11));
    mono_42* out_tmp_ref430 = out;
    mono_66(out_tmp_ref430);
    mono_42* out_tmp_ref431 = out;
    mono_105(out_tmp_ref431, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref432 = out;
    mono_106(out_tmp_ref432, ((au_index_t) BfetchAust__Fetch____c_nord12), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref433 = out;
    mono_105(out_tmp_ref433, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref434 = out;
    mono_106(out_tmp_ref434, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │──────", 22));
    mono_42* out_tmp_ref435 = out;
    mono_105(out_tmp_ref435, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("+++++++", 7));
    mono_42* out_tmp_ref436 = out;
    mono_107(out_tmp_ref436, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref437 = out;
    mono_108(out_tmp_ref437, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("++++++++++", 10), ((au_index_t) ((au_index_t) 1)));
    mono_42* out_tmp_ref438 = out;
    mono_106(out_tmp_ref438, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("────────│ ", 28));
    mono_42* out_tmp_ref439 = out;
    mono_105(out_tmp_ref439, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  o  │", 11));
    mono_42* out_tmp_ref440 = out;
    mono_66(out_tmp_ref440);
    mono_42* out_tmp_ref441 = out;
    mono_105(out_tmp_ref441, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref442 = out;
    mono_106(out_tmp_ref442, ((au_index_t) BfetchAust__Fetch____c_nord13), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref443 = out;
    mono_105(out_tmp_ref443, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref444 = out;
    mono_106(out_tmp_ref444, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │────────", 28));
    mono_42* out_tmp_ref445 = out;
    mono_105(out_tmp_ref445, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("++++++++++++++++++", 18));
    mono_42* out_tmp_ref446 = out;
    mono_107(out_tmp_ref446, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), ((au_index_t) ((au_index_t) 2)));
    mono_42* out_tmp_ref447 = out;
    mono_106(out_tmp_ref447, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("──────│ ", 22));
    mono_42* out_tmp_ref448 = out;
    mono_105(out_tmp_ref448, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│  o  │", 11));
    mono_42* out_tmp_ref449 = out;
    mono_66(out_tmp_ref449);
    mono_42* out_tmp_ref450 = out;
    mono_105(out_tmp_ref450, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref451 = out;
    mono_106(out_tmp_ref451, ((au_index_t) BfetchAust__Fetch____c_nord14), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref452 = out;
    mono_105(out_tmp_ref452, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref453 = out;
    mono_106(out_tmp_ref453, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │─────────", 31));
    mono_42* out_tmp_ref454 = out;
    mono_105(out_tmp_ref454, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//++++++++++++++", 16));
    mono_42* out_tmp_ref455 = out;
    mono_105(out_tmp_ref455, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref456 = out;
    mono_106(out_tmp_ref456, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("───────│ ", 25));
    mono_42* out_tmp_ref457 = out;
    mono_105(out_tmp_ref457, ((au_index_t) BfetchAust__Fetch____c_nord15), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("└─────┘", 21));
    mono_42* out_tmp_ref458 = out;
    mono_66(out_tmp_ref458);
    mono_42* out_tmp_ref459 = out;
    mono_105(out_tmp_ref459, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref460 = out;
    mono_106(out_tmp_ref460, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref461 = out;
    mono_105(out_tmp_ref461, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref462 = out;
    mono_106(out_tmp_ref462, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │───────", 25));
    mono_42* out_tmp_ref463 = out;
    mono_105(out_tmp_ref463, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//++++++++++++++", 16));
    mono_42* out_tmp_ref464 = out;
    mono_105(out_tmp_ref464, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref465 = out;
    mono_106(out_tmp_ref465, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("─────────│ ", 31));
    mono_42* out_tmp_ref466 = out;
    mono_66(out_tmp_ref466);
    mono_42* out_tmp_ref467 = out;
    mono_105(out_tmp_ref467, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref468 = out;
    mono_106(out_tmp_ref468, ((au_index_t) BfetchAust__Fetch____c_nord8), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref469 = out;
    mono_105(out_tmp_ref469, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref470 = out;
    mono_106(out_tmp_ref470, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │──── ", 17));
    mono_42* out_tmp_ref471 = out;
    mono_105(out_tmp_ref471, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//++++++++++++++", 16));
    mono_42* out_tmp_ref472 = out;
    mono_105(out_tmp_ref472, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref473 = out;
    mono_106(out_tmp_ref473, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("───────────│ ", 37));
    mono_42* out_tmp_ref474 = out;
    mono_66(out_tmp_ref474);
    mono_42* out_tmp_ref475 = out;
    mono_105(out_tmp_ref475, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref476 = out;
    mono_106(out_tmp_ref476, ((au_index_t) BfetchAust__Fetch____c_nord9), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref477 = out;
    mono_105(out_tmp_ref477, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref478 = out;
    mono_106(out_tmp_ref478, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref479 = out;
    mono_105(out_tmp_ref479, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//++++++++++", 12));
    mono_42* out_tmp_ref480 = out;
    mono_105(out_tmp_ref480, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref481 = out;
    mono_106(out_tmp_ref481, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("───────────────│", 48));
    mono_42* out_tmp_ref482 = out;
    mono_66(out_tmp_ref482);
    mono_42* out_tmp_ref483 = out;
    mono_105(out_tmp_ref483, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref484 = out;
    mono_106(out_tmp_ref484, ((au_index_t) BfetchAust__Fetch____c_nord10), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref485 = out;
    mono_105(out_tmp_ref485, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref486 = out;
    mono_106(out_tmp_ref486, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │─────", 19));
    mono_42* out_tmp_ref487 = out;
    mono_105(out_tmp_ref487, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//+++++++", 9));
    mono_42* out_tmp_ref488 = out;
    mono_105(out_tmp_ref488, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("//", 2));
    mono_42* out_tmp_ref489 = out;
    mono_106(out_tmp_ref489, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string("──────────────────│", 57));
    mono_42* out_tmp_ref490 = out;
    mono_66(out_tmp_ref490);
    mono_42* out_tmp_ref491 = out;
    mono_105(out_tmp_ref491, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref492 = out;
    mono_106(out_tmp_ref492, ((au_index_t) BfetchAust__Fetch____c_nord15), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref493 = out;
    mono_105(out_tmp_ref493, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref494 = out;
    mono_106(out_tmp_ref494, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" │──────", 22));
    mono_42* out_tmp_ref495 = out;
    mono_105(out_tmp_ref495, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("////////", 8));
    mono_42* out_tmp_ref496 = out;
    mono_105(out_tmp_ref496, ((au_index_t) BfetchAust__Fetch____c_nord1), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("────────────────────│", 63));
    mono_42* out_tmp_ref497 = out;
    mono_66(out_tmp_ref497);
    mono_42* out_tmp_ref498 = out;
    mono_105(out_tmp_ref498, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" │", 4));
    mono_42* out_tmp_ref499 = out;
    mono_106(out_tmp_ref499, ((au_index_t) BfetchAust__Fetch____c_nord7), au_make_span_from_string("██", 6));
    mono_42* out_tmp_ref500 = out;
    mono_105(out_tmp_ref500, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string("│", 3));
    mono_42* out_tmp_ref501 = out;
    mono_106(out_tmp_ref501, ((au_index_t) BfetchAust__Fetch____c_nord1), au_make_span_from_string(" └──────────────────────────────────┘", 109));
    mono_42* out_tmp_ref502 = out;
    mono_66(out_tmp_ref502);
    return false;
}

/*
  Function monomorph: tailLine[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_97(mono_42* out) {
    mono_42* out_tmp_ref371 = out;
    mono_105(out_tmp_ref371, ((au_index_t) BfetchAust__Fetch____c_reset), ((au_index_t) BfetchAust__Fetch____c_bold), au_make_span_from_string(" └──┘", 13));
    mono_42* out_tmp_ref372 = out;
    mono_109(out_tmp_ref372, ((au_index_t) BfetchAust__Fetch____c_reset));
    mono_42* out_tmp_ref373 = out;
    mono_66(out_tmp_ref373);
    return false;
}

/*
  Function monomorph: containsAsciiCI[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_98(mono_42* buf, au_span_t lit) {
    au_index_t n;
    n = mono_20(lit);
    au_bool_t _t314 = (n == ((au_index_t) 0));
    if (_t314) {
        return true;
    } else {
    }
    au_bool_t _t313 = (n > mono_45(buf));
    if (_t313) {
        return false;
    } else {
    }
    au_index_t _t306 = ((au_index_t) 0);
    au_index_t _t307 = meth_34(((au_index_t) mono_45(buf)), ((au_index_t) n));
    {
        size_t pos = _t306;
        for (; pos <= _t307; pos++) {
            au_bool_t ok;
            ok = true;
            au_index_t _t309 = ((au_index_t) 0);
            au_index_t _t310 = meth_34(((au_index_t) n), ((au_index_t) 1));
            {
                size_t i = _t309;
                for (; i <= _t310; i++) {
                    au_span_t* lit_tmp_ref269 = &(lit);
                    au_bool_t _t311 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) i)))))) != decl_485(((au_nat8_t) *((au_nat8_t*) au_array_index(lit_tmp_ref269, i, sizeof(au_nat8_t))))));
                    if (_t311) {
                        au_bool_t _t312 = false;
                        ok = _t312;
                    } else {
                    }
                }
            }
            au_bool_t _t308 = ok;
            if (_t308) {
                return true;
            } else {
            }
        }
    }
    return false;
}

/*
  Function monomorph: appendIndexDigits[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_99(mono_42* buf, au_index_t value) {
    au_bool_t _t225 = (value >= ((au_index_t) 10));
    if (_t225) {
        mono_42* buf_tmp_ref136 = buf;
        mono_99(buf_tmp_ref136, ((au_index_t) meth_36(((au_index_t) value), ((au_index_t) ((au_index_t) 10)))));
    } else {
    }
    mono_2 _t224 = meth_162(meth_283(value, ((au_index_t) 10)));
    mono_2 tmp54 = _t224;
    switch ((tmp54).tag) {
        case mono_2_tag_Some:
            {
                au_nat8_t digit = (((tmp54).data).Some).value;
                mono_42* buf_tmp_ref137 = buf;
                mono_76(buf_tmp_ref137, ((au_nat8_t) meth_1(((au_nat8_t) digit), ((au_nat8_t) ((au_nat8_t) 48)))));
            }
            break;
        case mono_2_tag_None:
            {
                decl_9(au_make_span_from_string("appendIndexDigits: digit conversion failed", 42));
            }
            break;
    }
    return false;
}

/*
  Function monomorph: lineEquals[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_100(mono_42* buf, au_span_t lit) {
    au_index_t n;
    n = mono_101(buf);
    au_bool_t _t343 = (n != mono_20(lit));
    if (_t343) {
        return false;
    } else {
    }
    au_bool_t _t339 = (n > ((au_index_t) 0));
    if (_t339) {
        au_index_t _t340 = ((au_index_t) 0);
        au_index_t _t341 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t340;
            for (; i <= _t341; i++) {
                au_span_t* lit_tmp_ref277 = &(lit);
                au_bool_t _t342 = (mono_49(buf, ((au_index_t) i)) != *((au_nat8_t*) au_array_index(lit_tmp_ref277, i, sizeof(au_nat8_t))));
                if (_t342) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function monomorph: lineLength[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_index_t mono_101(mono_42* buf) {
    au_index_t n;
    n = mono_45(buf);
    au_index_t len;
    len = ((au_index_t) 0);
    au_bool_t _t336 = (len < n);
    while (_t336) {
        au_bool_t _t338 = (mono_49(buf, ((au_index_t) len)) == 10);
        if (_t338) {
            return len;
        } else {
        }
        au_index_t _t337 = meth_33(((au_index_t) len), ((au_index_t) ((au_index_t) 1)));
        len = _t337;
        _t336 = (len < n);
    }
    return len;
}

/*
  Function monomorph: matchesAsciiCIAt[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_102(mono_42* buf, au_index_t pos, au_span_t lit) {
    au_index_t n;
    n = mono_20(lit);
    au_bool_t _t355 = (meth_33(((au_index_t) pos), ((au_index_t) n)) > mono_45(buf));
    if (_t355) {
        return false;
    } else {
    }
    au_bool_t _t351 = (n > ((au_index_t) 0));
    if (_t351) {
        au_index_t _t352 = ((au_index_t) 0);
        au_index_t _t353 = meth_34(((au_index_t) n), ((au_index_t) 1));
        {
            size_t i = _t352;
            for (; i <= _t353; i++) {
                au_span_t* lit_tmp_ref280 = &(lit);
                au_bool_t _t354 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) i)))))) != decl_485(((au_nat8_t) *((au_nat8_t*) au_array_index(lit_tmp_ref280, i, sizeof(au_nat8_t))))));
                if (_t354) {
                    return false;
                } else {
                }
            }
        }
    } else {
    }
    return true;
}

/*
  Function monomorph: hex4Matches[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_bool_t mono_103(mono_42* buf, au_index_t pos, au_index_t value) {
    au_bool_t _t438 = (meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 4))) > mono_45(buf));
    if (_t438) {
        return false;
    } else {
    }
    au_bool_t _t437 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 0))))))) != decl_505(((au_index_t) meth_283(meth_36(((au_index_t) value), ((au_index_t) ((au_index_t) 4096))), ((au_index_t) 16)))));
    if (_t437) {
        return false;
    } else {
    }
    au_bool_t _t436 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 1))))))) != decl_505(((au_index_t) meth_283(meth_36(((au_index_t) value), ((au_index_t) ((au_index_t) 256))), ((au_index_t) 16)))));
    if (_t436) {
        return false;
    } else {
    }
    au_bool_t _t435 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 2))))))) != decl_505(((au_index_t) meth_283(meth_36(((au_index_t) value), ((au_index_t) ((au_index_t) 16))), ((au_index_t) 16)))));
    if (_t435) {
        return false;
    } else {
    }
    au_bool_t _t434 = (decl_485(((au_nat8_t) mono_49(buf, ((au_index_t) meth_33(((au_index_t) pos), ((au_index_t) ((au_index_t) 3))))))) != decl_505(((au_index_t) meth_283(value, ((au_index_t) 16)))));
    if (_t434) {
        return false;
    } else {
    }
    return true;
}

/*
  Function monomorph: copyGpuLineName[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_104(mono_42* out, mono_42* src, au_index_t start, au_index_t stop, au_index_t vendor) {
    au_index_t open_br;
    open_br = stop;
    au_index_t close_br;
    close_br = stop;
    au_index_t name_start;
    name_start = start;
    au_index_t name_stop;
    name_stop = stop;
    au_index_t i;
    i = start;
    au_bool_t _t451 = (i < stop);
    while (_t451) {
        au_nat8_t byte;
        byte = mono_49(src, ((au_index_t) i));
        au_bool_t _t456 = ((byte == 91) && (open_br == stop));
        if (_t456) {
            au_index_t _t457 = i;
            open_br = _t457;
        } else {
        }
        au_bool_t _t452 = ((byte == 93) && (open_br != stop));
        if (_t452) {
            au_index_t _t455 = i;
            close_br = _t455;
            au_index_t _t454 = stop;
            i = _t454;
        } else {
            au_index_t _t453 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
            i = _t453;
        }
        _t451 = (i < stop);
    }
    mono_42* out_tmp_ref289 = out;
    mono_74(out_tmp_ref289, au_make_span_from_string("", 0));
    au_bool_t _t446 = (open_br < stop);
    if (_t446) {
        au_bool_t _t447 = (close_br < stop);
        if (_t447) {
            au_bool_t _t448 = (close_br > open_br);
            if (_t448) {
                au_index_t _t450 = meth_33(((au_index_t) open_br), ((au_index_t) ((au_index_t) 1)));
                name_start = _t450;
                au_index_t _t449 = close_br;
                name_stop = _t449;
            } else {
            }
        } else {
        }
    } else {
    }
    au_bool_t _t442 = (name_start < name_stop);
    if (_t442) {
        au_nat8_t first;
        first = mono_49(src, ((au_index_t) name_start));
        au_bool_t _t443 = ((first != 78) && (first != 65));
        if (_t443) {
            au_bool_t _t444 = (vendor == ((au_index_t) 4318));
            if (_t444) {
                mono_42* out_tmp_ref290 = out;
                mono_65(out_tmp_ref290, au_make_span_from_string("NVIDIA ", 7));
            } else {
                au_bool_t _t445 = (vendor == ((au_index_t) 4098));
                if (_t445) {
                    mono_42* out_tmp_ref291 = out;
                    mono_65(out_tmp_ref291, au_make_span_from_string("AMD ", 4));
                } else {
                }
            }
        } else {
        }
    } else {
    }
    au_index_t _t441 = name_start;
    i = _t441;
    au_bool_t _t439 = (i < name_stop);
    while (_t439) {
        mono_42* out_tmp_ref292 = out;
        mono_76(out_tmp_ref292, ((au_nat8_t) mono_49(src, ((au_index_t) i))));
        au_index_t _t440 = meth_33(((au_index_t) i), ((au_index_t) ((au_index_t) 1)));
        i = _t440;
        _t439 = (i < name_stop);
    }
    return false;
}

/*
  Function monomorph: styled2[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_105(mono_42* out, au_index_t code0, au_index_t code1, au_span_t text) {
    mono_42* out_tmp_ref349 = out;
    mono_109(out_tmp_ref349, ((au_index_t) code0));
    mono_42* out_tmp_ref350 = out;
    mono_109(out_tmp_ref350, ((au_index_t) code1));
    mono_42* out_tmp_ref351 = out;
    mono_65(out_tmp_ref351, text);
    return false;
}

/*
  Function monomorph: styled[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_106(mono_42* out, au_index_t code, au_span_t text) {
    mono_42* out_tmp_ref347 = out;
    mono_109(out_tmp_ref347, ((au_index_t) code));
    mono_42* out_tmp_ref348 = out;
    mono_65(out_tmp_ref348, text);
    return false;
}

/*
  Function monomorph: styledBackslashes[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_107(mono_42* out, au_index_t code0, au_index_t code1, au_index_t count) {
    mono_42* out_tmp_ref355 = out;
    mono_109(out_tmp_ref355, ((au_index_t) code0));
    mono_42* out_tmp_ref356 = out;
    mono_109(out_tmp_ref356, ((au_index_t) code1));
    mono_42* out_tmp_ref357 = out;
    mono_113(out_tmp_ref357, ((au_index_t) count));
    return false;
}

/*
  Function monomorph: styled2TrailBackslashes[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_108(mono_42* out, au_index_t code0, au_index_t code1, au_span_t text, au_index_t count) {
    mono_42* out_tmp_ref362 = out;
    mono_105(out_tmp_ref362, ((au_index_t) code0), ((au_index_t) code1), text);
    mono_42* out_tmp_ref363 = out;
    mono_113(out_tmp_ref363, ((au_index_t) count));
    return false;
}

/*
  Function monomorph: ansi[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_109(mono_42* out, au_index_t code) {
    mono_42* out_tmp_ref343 = out;
    mono_76(out_tmp_ref343, ((au_nat8_t) ((au_nat8_t) 27)));
    mono_42* out_tmp_ref344 = out;
    mono_76(out_tmp_ref344, ((au_nat8_t) 91));
    mono_42* out_tmp_ref345 = out;
    mono_75(out_tmp_ref345, ((au_index_t) code));
    mono_42* out_tmp_ref346 = out;
    mono_76(out_tmp_ref346, ((au_nat8_t) 109));
    return false;
}

/*
  Function monomorph: appendValue[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_110(mono_42* out, mono_42* value) {
    mono_42* out_tmp_ref354 = out;
    mono_90(out_tmp_ref354, mono_70(value));
    return false;
}

/*
  Function monomorph: styledTrailBackslashes[(MonoType.MonoRegionTy (Region.Region 0)), (MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_111(mono_42* out, au_index_t code, au_span_t text, au_index_t count) {
    mono_42* out_tmp_ref360 = out;
    mono_106(out_tmp_ref360, ((au_index_t) code), text);
    mono_42* out_tmp_ref361 = out;
    mono_113(out_tmp_ref361, ((au_index_t) count));
    return false;
}

/*
  Function monomorph: coloredBackslashes[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_112(mono_42* out, au_index_t code, au_index_t count) {
    mono_42* out_tmp_ref358 = out;
    mono_109(out_tmp_ref358, ((au_index_t) code));
    mono_42* out_tmp_ref359 = out;
    mono_113(out_tmp_ref359, ((au_index_t) count));
    return false;
}

/*
  Function monomorph: backslashes[(MonoType.MonoRegionTy (Region.Region 0))]
*/
au_unit_t mono_113(mono_42* out, au_index_t count) {
    au_bool_t _t581 = (count > ((au_index_t) 0));
    if (_t581) {
        au_index_t _t582 = ((au_index_t) 1);
        au_index_t _t583 = count;
        {
            size_t i = _t582;
            for (; i <= _t583; i++) {
                mono_42* out_tmp_ref353 = out;
                mono_76(out_tmp_ref353, ((au_nat8_t) 92));
            }
        }
    } else {
    }
    return false;
}
/* --- END translation unit for module 'BfetchAust.Fetch' --- */

/* --- BEGIN translation unit for module 'Austral.Wrappers' --- */

/* --- END translation unit for module 'Austral.Wrappers' --- */

/* --- BEGIN translation unit for module 'Wrappers' --- */

/* --- END translation unit for module 'Wrappers' --- */

int main(int argc, char** argv) {
    au_store_cli_args(argc, argv);
    mono_50 result = decl_562((mono_1){ .value = false });
    switch(result.tag) {
        case mono_50_tag_ExitSuccess:
            return 0;
        case mono_50_tag_ExitFailure:
            return 1;
    }
}