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
    char   *class_name;
    char   *function;
    uint8_t type;
} func_t;

// typedef struct _call_t {
//     func_t  func;
//     char   *filename;
//     int     lineno;
// } call_t;

#define UNKNOWN_FUNCTION_TYPE        0b00000
#define NORMAL_FUNCTION_TYPE         0b00001
#define STATIC_MEMBER_FUNCTION_TYPE  0b00010
#define MEMBER_FUNCTION_TYPE         0b00011

#define INCLUDES_FUNCTION_TYPE       0b10000

#define EVAL_FUNCTION_TYPE           0b10000
#define INCLUDE_FUNCTION_TYPE        0b10001
#define INCLUDE_ONCE_FUNCTION_TYPE   0b10010
#define REQUIRE_FUNCTION_TYPE        0b10011
#define REQUIRE_ONCE_FUNCTION_TYPE   0b10100
#define MAIN_FUNCTION_TYPE           0b10101

#define ZEND_PASS_FUNCTION_TYPE      0b100000

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
