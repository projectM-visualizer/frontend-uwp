
#ifndef PROJECTM_CXX_EXPORT_H
#define PROJECTM_CXX_EXPORT_H

#ifdef PROJECTM_CXX_STATIC_DEFINE
#  define PROJECTM_CXX_EXPORT
#  define PROJECTM_CXX_NO_EXPORT
#else
#  ifndef PROJECTM_CXX_EXPORT
#    ifdef projectM_api_EXPORTS
        /* We are building this library */
#      define PROJECTM_CXX_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define PROJECTM_CXX_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef PROJECTM_CXX_NO_EXPORT
#    define PROJECTM_CXX_NO_EXPORT 
#  endif
#endif

#ifndef PROJECTM_CXX_DEPRECATED
#  define PROJECTM_CXX_DEPRECATED __declspec(deprecated)
#endif

#ifndef PROJECTM_CXX_DEPRECATED_EXPORT
#  define PROJECTM_CXX_DEPRECATED_EXPORT PROJECTM_CXX_EXPORT PROJECTM_CXX_DEPRECATED
#endif

#ifndef PROJECTM_CXX_DEPRECATED_NO_EXPORT
#  define PROJECTM_CXX_DEPRECATED_NO_EXPORT PROJECTM_CXX_NO_EXPORT PROJECTM_CXX_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef PROJECTM_CXX_NO_DEPRECATED
#    define PROJECTM_CXX_NO_DEPRECATED
#  endif
#endif

#endif /* PROJECTM_CXX_EXPORT_H */
