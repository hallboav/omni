
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

void build_function_name(func_t *si, zend_execute_data *edata)
{
    memset(si, 0, sizeof(func_t));

#if PHP_VERSION_ID >= 70100
    if (edata && edata->func && edata->func == (zend_function *) &zend_pass_function) {
        si->function = "{zend_pass}";

        return;
    } else
#endif

    if (!(edata && edata->func)) {
        return;
    }

#if PHP_VERSION_ID >= 70100
    if ((Z_TYPE(edata->This)) == IS_OBJECT) {
#else
    if (edata->This.value.obj) {
#endif
        if (edata->func->common.scope) {
            si->class_name = edata->func->common.scope->name->val;
        } else {
            si->class_name = edata->This.value.obj->ce->name->val;
        }

        si->call_type = "->";
    } else if (edata->func->common.scope) {
        si->class_name = edata->func->common.scope->name->val;
        si->call_type = "::";
    }

    if (edata->func->common.function_name) {
        si->function = edata->func->common.function_name->val;
    } else if (
        edata->func->type == ZEND_EVAL_CODE &&
        edata->prev_execute_data &&
        edata->prev_execute_data->func &&
        edata->prev_execute_data->func->common.function_name
    ) {
        si->function = "{internal_eval}";
    } else if (
        edata->prev_execute_data &&
        edata->prev_execute_data->func->type == ZEND_USER_FUNCTION &&
        edata->prev_execute_data->opline &&
        edata->prev_execute_data->opline->opcode == ZEND_INCLUDE_OR_EVAL
    ) {
        switch (edata->prev_execute_data->opline->extended_value) {
            case ZEND_EVAL:
                si->function = "{eval}";
                break;
            case ZEND_INCLUDE:
                si->function = "{include}";
                break;
            case ZEND_REQUIRE:
                si->function = "{require}";
                break;
            case ZEND_INCLUDE_ONCE:
                si->function = "{include_once}";
                break;
            case ZEND_REQUIRE_ONCE:
                si->function = "{require_once}";
                break;
            default:
                si->function = "{unknown}";
        }
    } else if (edata->prev_execute_data) {
        build_function_name(si, edata);
    }
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

    func_t *si = (func_t *) emalloc(sizeof(func_t));
    if (NULL == si) {
        return;
    }

    build_function_name(&si, execute_data TSRMLS_CC);
    if (NULL != si->class_name) {
        zend_printf("%s%s", si->class_name, si->call_type);
    }

    zend_printf("%s\n", si->function);

    efree(si);
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
