
#ifndef PHP_PIB_H
#define PHP_PIB_H

extern zend_module_entry pib_module_entry;
#define phpext_pib_ptr &pib_module_entry

#define PHP_PIB_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_PIB_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PIB_API __attribute__ ((visibility("default")))
#else
#	define PHP_PIB_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(pib)
	zend_long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(pib)
*/

/* Always refer to the globals in your function as PIB_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define PIB_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(pib, v)

#if defined(ZTS) && defined(COMPILE_DL_PIB)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

void (*original_zend_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args);
void (*original_zend_execute_ex) (zend_execute_data *execute_data);
void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);

#endif	/* PHP_PIB_H */
