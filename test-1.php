<?php

error_reporting(0);
require_once 'test-2.php';
require_once 'test-3.php';

$anonClass = new class() {
    public function anonMethod()
    {
        (new \Foo\Bar\Baz)->bazMethod();
    }
};

$closure = function() use ($anonClass) {
    $anonClass->anonMethod();
};

function normal() {
    global $closure;
    $closure();
}

$__lambda_func = create_function('', <<<'FN'
$dummy = 'foo';
normal();
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

