
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_omni.h"
#include "Zend/zend_smart_str.h"

// void build_function_name(func_t *si, zend_execute_data *data)
// {
//     memset(si, 0, sizeof(func_t));

//     if (!(NULL != data && data->func)) {
//         zend_printf("debug: !(NULL != data && data->func)\n");
//         return;
//     }

//     // if (data->prev_execute_data && !ZEND_USER_CODE(data->prev_execute_data->func->common.type)) {
//         // zend_printf("debug: data->prev_execute_data && !ZEND_USER_CODE(data->prev_execute_data->func->common.type)\n");
//         // si->function = data->prev_execute_data->func->common.function_name->val;

//         // return;
//     // }

//     if (!ZEND_USER_CODE(data->func->common.type)) {
//         zend_printf("debug: !ZEND_USER_CODE(data->func->common.type) ");
//         zend_printf("which is \"%s\"\n", data->func->common.function_name->val);
//         return;
//     }

//     zend_object *object = (Z_TYPE(data->This) == IS_OBJECT) ? Z_OBJ(data->This) : NULL;
//     zend_string *zend_function_name = data->func->common.scope && data->func->common.scope->trait_aliases ?
//         zend_resolve_method_name(object ? object->ce : data->func->common.scope, data->func) :
//         data->func->common.function_name;

//     if (zend_function_name != NULL) {
//         si->function = ZSTR_VAL(zend_function_name);
//     } else if (data->prev_execute_data) {
//         // zend_printf("debug: tem prev e func e scope!\n");
//         // zend_execute_data *data_ptr = data;
//         // si->function = data->prev_execute_data->func->common.function_name->val;
//         build_function_name(si, data->prev_execute_data);
//     } else {
//         si->function = "{else}";
//     }
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

void build_function_name(func_t *si, zend_execute_data *edata TSRMLS_DC)
{
    memset(si, 0, sizeof(func_t));

#if PHP_VERSION_ID >= 70100
    if (edata && edata->func && edata->func == (zend_function *) &zend_pass_function) {
        si->type = ZEND_PASS_FUNCTION_TYPE;
        si->function = "{zend_pass}";

        return;
    } else
#endif

    if (edata && edata->func) {
        si->type = NORMAL_FUNCTION_TYPE;

#if PHP_VERSION_ID >= 70100
        if ((Z_TYPE(edata->This)) == IS_OBJECT) {
#else
        if (edata->This.value.obj) {
#endif

            si->type = MEMBER_FUNCTION_TYPE;
            if (edata->func->common.scope) {
                si->class_name = edata->func->common.scope->name->val;
            } else {
                si->class_name = edata->This.value.obj->ce->name->val;
            }

        } else if (edata->func->common.scope) {
            si->type = STATIC_MEMBER_FUNCTION_TYPE;
            si->class_name = edata->func->common.scope->name->val;
        }

        if (edata->func->common.function_name) {
            si->function = edata->func->common.function_name->val;
        } else if (
            edata->func->type == ZEND_EVAL_CODE &&
            edata->prev_execute_data &&
            edata->prev_execute_data->func &&
            edata->prev_execute_data->func->common.function_name &&
            ((strncmp(edata->prev_execute_data->func->common.function_name->val, "assert", 6) == 0) ||
            (strncmp(edata->prev_execute_data->func->common.function_name->val, "create_function", 15) == 0))
        ) {
            si->function = edata->prev_execute_data->func->common.function_name->val;
        } else if (
            edata->prev_execute_data &&
            edata->prev_execute_data->func->type == ZEND_USER_FUNCTION &&
            edata->prev_execute_data->opline &&
            edata->prev_execute_data->opline->opcode == ZEND_INCLUDE_OR_EVAL
        ) {
            switch (edata->prev_execute_data->opline->extended_value) {
                case ZEND_EVAL:
                    si->type = EVAL_FUNCTION_TYPE;
                    si->function = "{eval}";
                    break;
                case ZEND_INCLUDE:
                    si->type = INCLUDE_FUNCTION_TYPE;
                    si->function = "{include}";
                    break;
                case ZEND_REQUIRE:
                    si->type = REQUIRE_FUNCTION_TYPE;
                    si->function = "{require}";
                    break;
                case ZEND_INCLUDE_ONCE:
                    si->type = INCLUDE_ONCE_FUNCTION_TYPE;
                    si->function = "{include_once}";
                    break;
                case ZEND_REQUIRE_ONCE:
                    si->type = REQUIRE_ONCE_FUNCTION_TYPE;
                    si->function = "{require_once}";
                    break;
                default:
                    si->type = UNKNOWN_FUNCTION_TYPE;
                    si->function = "{unknown}";
            }
        } else if (edata->prev_execute_data) {
            build_function_name(si, edata->prev_execute_data);
        } else {
            si->type = UNKNOWN_FUNCTION_TYPE;
        }
    }
}

static int indent = 0;

void my_execute_ex(zend_execute_data *data TSRMLS_DC)
{
    // xdebug: {main}
    // xdebug: (null)
    // xdebug: {internal eval}
    // xdebug: init
    // xdebug: assert_callback
    // xdebug: __lambda_func
    // xdebug: anonMethod
    // xdebug: bazMethod
    // xdebug: staticMethod
    // xdebug: __invoke
    // xdebug: traitMethod
    // xdebug: Foo\Bar\normal

    //   omni: {main}()
    //   omni: {include_once}()
    //   omni: {internal_eval}()
    //   omni: init()
    //   omni: assert_callback()
    //   omni: __lambda_func()
    //   omni: anonMethod()
    //   omni: bazMethod()
    //   omni: staticMethod()
    //   omni: __invoke()
    //   omni: traitMethod()
    //   omni: Foo\Bar\normal()

    const char *function_name;
    if (data && data->func) {

        zend_object *object = (Z_TYPE(data->This) == IS_OBJECT) ? Z_OBJ(data->This) : NULL;
        zend_string *zend_function_name = data->func->common.scope && data->func->common.scope->trait_aliases ?
            zend_resolve_method_name(object ? object->ce : data->func->common.scope, data->func) :
            data->func->common.function_name;

        if (zend_function_name != NULL) {
            function_name = ZSTR_VAL(zend_function_name);
        } else if (
            data->func->type == ZEND_EVAL_CODE &&
            data->prev_execute_data &&
            data->prev_execute_data->func &&
            data->prev_execute_data->func->common.function_name &&
            ((strncmp(data->prev_execute_data->func->common.function_name->val, "assert", 6) == 0) ||
            (strncmp(data->prev_execute_data->func->common.function_name->val, "create_function", 15) == 0))
        ) {
            function_name = "{internal_eval}";
        } else if (
            data->prev_execute_data &&
            data->prev_execute_data->func->type == ZEND_USER_FUNCTION &&
            data->prev_execute_data->opline &&
            data->prev_execute_data->opline->opcode == ZEND_INCLUDE_OR_EVAL
        ) {
            switch (data->prev_execute_data->opline->extended_value) {
                case ZEND_EVAL:
                    function_name = "{eval}";
                    break;
                case ZEND_INCLUDE:
                    function_name = "{include}";
                    break;
                case ZEND_REQUIRE:
                    function_name = "{require}";
                    break;
                case ZEND_INCLUDE_ONCE:
                    function_name = "{include_once}";
                    break;
                case ZEND_REQUIRE_ONCE:
                    function_name = "{require_once}";
                    break;
                default:
                    function_name = "{unknown}";
            }
        } else {
            function_name = "{main}";
        }
    }

    if (NULL != function_name) {
        zend_printf("#%-2d ", indent++);

        zend_printf("%s()\n", function_name);
    }

    ///////////////////
    // ORIGINAL CALL //
    ///////////////////

    original_zend_execute_ex(data TSRMLS_CC);
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
