
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_omni.h"
#include "Zend/zend_smart_str.h"

// ZEND_DECLARE_MODULE_GLOBALS(omni)

// void my_error_cb(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args)
// {
//     va_list args_copy;
//     va_copy(args_copy, args);

//     char *buffer;
//     vspprintf(&buffer, 0, format, args);

//     zend_printf(
//         "type: %d\nfilename: %s\nline: %u\nreason: %s.\n\n\n",
//         type,
//         error_filename,
//         error_lineno,
//         buffer
//     );

//     efree(buffer);

//     original_zend_error_cb(type, error_filename, error_lineno, format, args_copy);
// }

char *my_sprintf(const char* fmt, ...)
{
    char   *new_str;
    int     size = 1;
    va_list args;

    new_str = (char *) emalloc(size);

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

        new_str = (char *) erealloc(new_str, size);
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

char* wrap_closure_location(zend_op_array *opa, char *fname)
{
    smart_str str = {0};
    smart_str_appendl(&str, fname, strlen(fname) - 1);

    char *tmp = my_sprintf(
        ":%s:%d-%d}",
        opa->filename->val,
        opa->line_start,
        opa->line_end
    );

    smart_str_appends(&str, tmp);
    efree(tmp);

    smart_str_0(&str);

    char* new_fname = estrdup(str.s->val);
    smart_str_free(&str);

    return new_fname;
}

void build_fname(func_t *func, zend_execute_data *edata TSRMLS_DC)
{
    memset(func, 0, sizeof(func_t));

#if PHP_VERSION_ID >= 70100
    if (edata && edata->func && edata->func == (zend_function*) &zend_pass_function) {
        func->type     = FUNC_ZEND_PASS;
        func->fname = estrdup("{zend_pass}");
    } else
#endif

    if (edata && edata->func) {
        func->type = FUNC_NORMAL;

        ////////////////
        // CLASSNAMES //
        ////////////////

#if PHP_VERSION_ID >= 70100
        if ((Z_TYPE(edata->This)) == IS_OBJECT) {
#else
        if (edata->This.value.obj) {
#endif
            func->type = FUNC_MEMBER;

            if (edata->func->common.scope && strcmp(edata->func->common.scope->name->val, "class@anonymous") == 0) {
                func->class = my_sprintf(
                    "{anonymous-class:%s:%d-%d}",
                    edata->func->common.scope->info.user.filename->val,
                    edata->func->common.scope->info.user.line_start,
                    edata->func->common.scope->info.user.line_end
                );
            } else {
                func->class = estrdup(edata->This.value.obj->ce->name->val);
            }
        } else if (edata->func->common.scope) {
            func->type = FUNC_STATIC_MEMBER;
            func->class = estrdup(edata->func->common.scope->name->val);
        }

        /////////////
        // FUNÇÕES //
        /////////////

        if (edata->func->common.function_name) {
            if (function_name_is_closure(edata->func->common.function_name->val)) {
                func->fname = wrap_closure_location(&edata->func->op_array, edata->func->common.function_name->val);
            } else {
                func->fname = estrdup(edata->func->common.function_name->val);
            }
        } else if (
            edata->func->type == ZEND_EVAL_CODE &&
            edata->prev_execute_data &&
            edata->prev_execute_data->func &&
            edata->prev_execute_data->func->common.function_name &&
            ((strncmp(edata->prev_execute_data->func->common.function_name->val, "assert", 6) == 0) ||
             (strncmp(edata->prev_execute_data->func->common.function_name->val, "create_function", 15) == 0))
        ) {
            func->fname = estrdup("{internal eval}");
        } else if (
            edata->prev_execute_data &&
            edata->prev_execute_data->func->type == ZEND_USER_FUNCTION &&
            edata->prev_execute_data->opline &&
            edata->prev_execute_data->opline->opcode == ZEND_INCLUDE_OR_EVAL
        ) {
            switch (edata->prev_execute_data->opline->extended_value) {
                case ZEND_EVAL:
                    func->type = FUNC_EVAL;
                    break;
                case ZEND_INCLUDE:
                    func->type = FUNC_INCLUDE;
                    break;
                case ZEND_REQUIRE:
                    func->type = FUNC_REQUIRE;
                    break;
                case ZEND_INCLUDE_ONCE:
                    func->type = FUNC_INCLUDE_ONCE;
                    break;
                case ZEND_REQUIRE_ONCE:
                    func->type = FUNC_REQUIRE_ONCE;
                    break;
                default:
                    func->type = FUNC_UNKNOWN;
            }
        } else if (edata->prev_execute_data) {
            build_fname(func, edata->prev_execute_data);
        } else {
            func->type = FUNC_UNKNOWN;
        }
    } // edata && edata->func
}

void my_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    original_zend_execute_ex(execute_data TSRMLS_CC);

    func_t *func = emalloc(sizeof(func_t));

    build_fname(func, execute_data);
    if (NULL != func->class) {
        zend_printf("%s%s", func->class, FUNC_MEMBER == func->type ? "->" : "::");
    }

    zend_printf("%s", func->fname);
    efree(func);
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

    // original_zend_error_cb = zend_error_cb;
    // zend_error_cb = my_error_cb;

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = my_execute_ex;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(omni)
{
	// UNREGISTER_INI_ENTRIES();

	// zend_error_cb = original_zend_error_cb;
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
