#ifndef configH
#define configH

#ifdef _WIN32
#  ifdef CPPCHECKLIB_EXPORT
#    define CPPCHECKLIB __declspec(dllexport)
#  elif defined(CPPCHECKLIB_IMPORT)
#    define CPPCHECKLIB __declspec(dllimport)
#  else
#    define CPPCHECKLIB
#  endif
#else
#  define CPPCHECKLIB
#endif

// MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

// C++11 override
#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC >= 5)) || defined(__CPPCHECK__)
#  define OVERRIDE override
#else
#  define OVERRIDE
#endif

#include <string>
static const std::string emptyString;

#endif // configH
