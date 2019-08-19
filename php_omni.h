// maldivas
// ouro
// sem diamantes
// zirconias com 45 pontos

#ifndef PHP_OMNI_H
#define PHP_OMNI_H

extern zend_module_entry omni_module_entry;
#define phpext_omni_ptr &omni_module_entry

#define PHP_OMNI_VERSION "0.1.0"

#ifdef PHP_WIN32
#   define PHP_OMNI_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_OMNI_API __attribute__ ((visibility("default")))
#else
#   define PHP_OMNI_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

typedef struct _func_t {
    char *class;
    char *fname;
    int   type;
} func_t;

typedef struct _call_t {
    func_t  func;
    char   *filename;
    int     lineno;
} call_t;

#define FUNC_UNKNOWN        0x00
#define FUNC_NORMAL         0x01
#define FUNC_STATIC_MEMBER  0x02
#define FUNC_MEMBER         0x03

#define FUNC_INCLUDES       0x10

#define FUNC_EVAL           0x10
#define FUNC_INCLUDE        0x11
#define FUNC_INCLUDE_ONCE   0x12
#define FUNC_REQUIRE        0x13
#define FUNC_REQUIRE_ONCE   0x14
#define FUNC_MAIN           0x15

#define FUNC_ZEND_PASS      0x20

// ZEND_BEGIN_MODULE_GLOBALS(omni)
//  zend_long global_value;
//  char *global_string;
// ZEND_END_MODULE_GLOBALS(omni)

#define OMNI_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(omni, v)

#if defined(ZTS) && defined(COMPILE_DL_OMNI)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

void (*original_zend_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args);
void (*original_zend_execute_ex) (zend_execute_data *execute_data);

#endif  /* PHP_OMNI_H */
