<?php
$startOf = 'file';

include_once 'stack-2.php';

$anonClass = new class() {
    public function anonMethod()
    {
        (new \Foo\Bar\Baz)->bazMethod();
    }
};

$__lambda_func = create_function('', <<<'FN'
$dummy = 'foo';
global $anonClass;
$anonClass->anonMethod();
return 4;
FN);

function my_assert_handler($file, $line, $code)
{
    $dummy = 3;
    global $__lambda_func;
    $__lambda_func();
}

assert_options(ASSERT_ACTIVE,     1);
assert_options(ASSERT_WARNING,    0);
assert_options(ASSERT_QUIET_EVAL, 1);
assert_options(ASSERT_CALLBACK,   'my_assert_handler');

assert(false, 'is false!!!');
