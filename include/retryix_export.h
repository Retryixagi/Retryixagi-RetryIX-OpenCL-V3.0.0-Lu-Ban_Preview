#pragma once
/* Centralized export / calling convention definitions for RetryIX */

/*
 * Windows branch: treat Cygwin as POSIX/ELF, not Windows DLL.
 * Use RETRYIX_STATIC to signal a static build (no dllimport).
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
  /* Windows / MSVC / MinGW-w64 */
  /* Only mark dllexport when building the DLL itself. Do NOT default to
     dllimport because that causes definition-vs-declaration conflicts when
     compiling the library sources or when headers are parsed without
     RETRYIX_STATIC/RETRYIX_BUILD_DLL set. Use .def if you need explicit
     export control for the DLL consumer. */
  #if defined(RETRYIX_BUILD_DLL)
    #define RETRYIX_API __declspec(dllexport)
  #else
    #define RETRYIX_API
  #endif
  /* Keep cdecl for unmangled names and .def consistency */
  #define RETRYIX_CALL __cdecl
  #define RETRYIX_HIDDEN /* no-op on Windows */
#else
  /* ELF-like platforms (Linux, macOS, *nix). Recommend building with
     -fvisibility=hidden so only RETRYIX_API symbols are exported. */
  #if defined(__GNUC__) || defined(__clang__)
    #define RETRYIX_API    __attribute__((visibility("default")))
    #define RETRYIX_HIDDEN __attribute__((visibility("hidden")))
  #else
    #define RETRYIX_API
    #define RETRYIX_HIDDEN
  #endif
  #define RETRYIX_CALL
#endif

/* Extern "C" helpers – please wrap public headers with these */
#ifdef __cplusplus
  #define RETRYIX_EXTERN_C_BEGIN extern "C" {
  #define RETRYIX_EXTERN_C_END   }
#else
  #define RETRYIX_EXTERN_C_BEGIN
  #define RETRYIX_EXTERN_C_END
#endif

/* Additional utility attributes */
#if defined(__GNUC__) || defined(__clang__)
  #define RETRYIX_DEPRECATED(msg) __attribute__((deprecated(msg)))
  #define RETRYIX_NODISCARD       __attribute__((warn_unused_result))
#else
  #ifdef _MSC_VER
    #define RETRYIX_DEPRECATED(msg) __declspec(deprecated(msg))
    #define RETRYIX_NODISCARD       _Check_return_
  #else
    #define RETRYIX_DEPRECATED(msg)
    #define RETRYIX_NODISCARD
  #endif
#endif

