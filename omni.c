
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_omni.h"
#include "Zend/zend_smart_str.h"

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

void my_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    ///////////////////
    // ORIGINAL CALL //
    ///////////////////

    original_zend_execute_ex(execute_data TSRMLS_CC);

    ///////////////////////////
    // INFORMATION GATHERING //
    ///////////////////////////

    zend_string *class_name;
    const char *function_name;
    const char *call_type;
    zend_execute_data *prev_execute_data;
    zend_object *object;
    zend_function *func;

    prev_execute_data = execute_data->prev_execute_data;
    if (prev_execute_data && prev_execute_data->func) {

        func = prev_execute_data->func;

        object = (Z_TYPE(prev_execute_data->This)) == IS_OBJECT ? Z_OBJ(prev_execute_data->This) : NULL;

        zend_string *zend_function_name;
        if (func->common.scope && func->common.scope->trait_aliases) {
            // Exemplo #5 Conflict Resolution
            // https://www.php.net/manual/pt_BR/language.oop5.traits.php
            zend_function_name = zend_resolve_method_name(object ? object->ce : func->common.scope, func);
        } else {
            zend_function_name = func->common.function_name;
        }

        if (zend_function_name != NULL) {
            function_name = ZSTR_VAL(zend_function_name);
        } else {
            function_name = NULL;
        }

        if (function_name) {
            if (object) {
                if (func->common.scope) {
                    class_name = func->common.scope->name;
                } else if (object->handlers->get_class_name == std_object_handlers.get_class_name) {
                    class_name = object->ce->name;
                } else {
                    class_name = object->handlers->get_class_name(object);
                }

                call_type = "->";
            } else if (func->common.scope) {
                class_name = func->common.scope->name;
                call_type = "::";
            } else {
                class_name = NULL;
                call_type = NULL;
            }
        } else {
            switch (prev_execute_data->opline->extended_value) {
                case ZEND_EVAL:
                    function_name = "eval";
                    break;
                case ZEND_INCLUDE:
                    function_name = "include";
                    break;
                case ZEND_REQUIRE:
                    function_name = "require";
                    break;
                case ZEND_INCLUDE_ONCE:
                    function_name = "include_once";
                    break;
                case ZEND_REQUIRE_ONCE:
                    function_name = "require_once";
                    break;
                default:
                    function_name = "unknown";
            }

            call_type = NULL;
        }
    } else {
        class_name = NULL;
        call_type = NULL;
        function_name = "main";
    }

    if (class_name) {
        ZEND_PUTS(ZSTR_VAL(class_name));
        ZEND_PUTS(call_type);

        if (object && !func->common.scope && object->handlers->get_class_name != std_object_handlers.get_class_name) {
            zend_string_release(class_name);
        }
    }

    zend_printf("%s(", function_name);
}

PHP_MINIT_FUNCTION(omni)
{
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = my_execute_ex;

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(omni)
{
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
