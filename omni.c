
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_omni.h"

#define XFUNC_UNKNOWN        0x00
#define XFUNC_NORMAL         0x01
#define XFUNC_STATIC_MEMBER  0x02
#define XFUNC_MEMBER         0x03

#define XFUNC_INCLUDES       0x10
#define XFUNC_EVAL           0x10
#define XFUNC_INCLUDE        0x11
#define XFUNC_INCLUDE_ONCE   0x12
#define XFUNC_REQUIRE        0x13
#define XFUNC_REQUIRE_ONCE   0x14
#define XFUNC_MAIN           0x15

#define XFUNC_ZEND_PASS      0x20

typedef struct _func_t {
    char *class;
    char *function;
    int   type;
    int   internal;
} func_t;

typedef struct _function_stack_entry_t {
    func_t function;
} function_stack_entry_t;

// ZEND_DECLARE_MODULE_GLOBALS(omni)

void my_error_cb(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    char *buffer;
    vspprintf(&buffer, 0, format, args);

    zend_printf(
        "type: %d\nfilename: %s\nline: %u\nreason: %s.\n\n\n",
        type,
        error_filename,
        error_lineno,
        buffer
    );

    efree(buffer);

    original_zend_error_cb(type, error_filename, error_lineno, format, args_copy);
}

char *xdebug_sprintf(const char* fmt, ...)
{
    char   *new_str;
    int     size = 1;
    va_list args;

    new_str = (char *) malloc(size);

    for (;;) {
        int n;

        va_start(args, fmt);
        n = vsnprintf(new_str, size, fmt, args);
        va_end(args);

        if (n > -1 && n < size) {
            break;
        }
        if (n < 0) {
            size *= 2;
        } else {
            size = n + 1;
        }
        new_str = (char *) realloc(new_str, size);
    }

    return new_str;
}

int function_name_is_closure(char *fname)
{
    int length = strlen(fname);
    int closure_length = strlen("{closure}");

    if (length < closure_length) {
        return 0;
    }

    if (strcmp(fname + length - closure_length, "{closure}") == 0) {
        return 1;
    }

    return 0;
}

void build_fname(func_t *tmp, zend_execute_data *edata TSRMLS_DC)
{
    memset(tmp, 0, sizeof(func_t));

#if PHP_VERSION_ID >= 70100
    if (edata && edata->func && edata->func == (zend_function*) &zend_pass_function) {
        tmp->type     = XFUNC_ZEND_PASS;
        tmp->function = estrdup("{zend_pass}");
    } else
#endif

    if (edata && edata->func) {
        tmp->type = XFUNC_NORMAL;

#if PHP_VERSION_ID >= 70100
        if ((Z_TYPE(edata->This)) == IS_OBJECT) {
#else
        if (edata->This.value.obj) {
#endif

            tmp->type = XFUNC_MEMBER;
            if (edata->func->common.scope && strcmp(edata->func->common.scope->name->val, "class@anonymous") == 0) {
                tmp->class = xdebug_sprintf(
                    "{anonymous-class:%s:%d-%d}",
                    edata->func->common.scope->info.user.filename->val,
                    edata->func->common.scope->info.user.line_start,
                    edata->func->common.scope->info.user.line_end
                );
            } else {
                tmp->class = estrdup(edata->This.value.obj->ce->name->val);
            }
        } else {
            if (edata->func->common.scope) {
                tmp->type = XFUNC_STATIC_MEMBER;
                tmp->class = estrdup(edata->func->common.scope->name->val);
            }
        }

        if (edata->func->common.function_name) {
            if (function_name_is_closure(edata->func->common.function_name->val)) {
                tmp->function = estrdup("{closure_mas_tem_que_arrumar}");
            } else if (strncmp(edata->func->common.function_name->val, "call_user_func", 14) == 0) {
                tmp->function = estrdup("{call_user_func}");
            } else {
                tmp->function = estrdup(edata->func->common.function_name->val);
            }
        } else if (
            edata &&
            edata->func &&
            edata->func->type == ZEND_EVAL_CODE &&
            edata->prev_execute_data &&
            edata->prev_execute_data->func &&
            edata->prev_execute_data->func->common.function_name &&
            ((strncmp(edata->prev_execute_data->func->common.function_name->val, "assert", 6) == 0) ||
            (strncmp(edata->prev_execute_data->func->common.function_name->val, "create_function", 15) == 0))
        ) {
            tmp->type = XFUNC_NORMAL;
            tmp->function = estrdup("{internal eval}");
        } else if (
            edata &&
            edata->prev_execute_data &&
            edata->prev_execute_data->func->type == ZEND_USER_FUNCTION &&
            edata->prev_execute_data->opline &&
            edata->prev_execute_data->opline->opcode == ZEND_INCLUDE_OR_EVAL
        ) {
            switch (edata->prev_execute_data->opline->extended_value) {
                case ZEND_EVAL:
                    tmp->type = XFUNC_EVAL;
                    break;
                case ZEND_INCLUDE:
                    tmp->type = XFUNC_INCLUDE;
                    break;
                case ZEND_REQUIRE:
                    tmp->type = XFUNC_REQUIRE;
                    break;
                case ZEND_INCLUDE_ONCE:
                    tmp->type = XFUNC_INCLUDE_ONCE;
                    break;
                case ZEND_REQUIRE_ONCE:
                    tmp->type = XFUNC_REQUIRE_ONCE;
                    break;
                default:
                    tmp->type = XFUNC_UNKNOWN;
                    break;
            }
        } else if (edata && edata->prev_execute_data) {
            build_fname(tmp, edata->prev_execute_data);
        } else {
            tmp->type = XFUNC_UNKNOWN;
        }
    } // edata && edata->func
}

function_stack_entry_t *add_stack_frame(zend_execute_data *zdata)
{
    function_stack_entry_t *tmp = emalloc(sizeof(function_stack_entry_t));
    build_fname(&(tmp->function), zdata TSRMLS_CC);

    if (!tmp->function.type) {
        tmp->function.function = estrdup("{main}");
        tmp->function.class    = NULL;
        tmp->function.type     = XFUNC_MAIN;
    }

    return tmp;
}

void profiler_function_begin(function_stack_entry_t *fse TSRMLS_DC)
{

}

void profiler_function_end(function_stack_entry_t *fse TSRMLS_DC)
{

}

void my_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    function_stack_entry_t *fse = add_stack_frame(execute_data);
    zend_printf("->%s\n", fse->function.function);

    profiler_function_begin(fse TSRMLS_CC);
    original_zend_execute_ex(execute_data TSRMLS_CC);
    profiler_function_end(fse TSRMLS_CC);

    efree(fse->function.function);
    efree(fse);
}

// PHP_INI_BEGIN()
//     STD_PHP_INI_ENTRY("omni.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_omni_globals, omni_globals)
//     STD_PHP_INI_ENTRY("omni.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_omni_globals, omni_globals)
// PHP_INI_END()

// static void php_omni_init_globals(zend_omni_globals *omni_globals)
// {
// 	omni_globals->global_value = 0;
// 	omni_globals->global_string = NULL;
// }

PHP_MINIT_FUNCTION(omni)
{
	// REGISTER_INI_ENTRIES();

    original_zend_error_cb = zend_error_cb;
    zend_error_cb = my_error_cb;

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = my_execute_ex;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(omni)
{
	// UNREGISTER_INI_ENTRIES();

	zend_error_cb = original_zend_error_cb;
    zend_execute_ex = original_zend_execute_ex;

	return SUCCESS;
}

PHP_RINIT_FUNCTION(omni)
{
#if defined(COMPILE_DL_OMNI) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(omni)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(omni)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "omni support", "enabled");
	php_info_print_table_end();

	// DISPLAY_INI_ENTRIES();
}

const zend_function_entry omni_functions[] = {
	PHP_FE_END
};

zend_module_entry omni_module_entry = {
	STANDARD_MODULE_HEADER,
	"omni",
	omni_functions,
	PHP_MINIT(omni),
	PHP_MSHUTDOWN(omni),
	PHP_RINIT(omni),
	PHP_RSHUTDOWN(omni),
	PHP_MINFO(omni),
	PHP_OMNI_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_OMNI
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(omni)
#endif