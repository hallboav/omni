PHP_ARG_ENABLE(omni, whether to enable omni support,
[  --enable-omni           Enable omni support])

if test "$PHP_OMNI" != "no"; then
    PHP_NEW_EXTENSION(omni, omni.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
