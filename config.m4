PHP_ARG_ENABLE(pib, whether to enable pib support,
[  --enable-pib           Enable pib support])

if test "$PHP_PIB" != "no"; then
  PHP_NEW_EXTENSION(pib, pib.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
