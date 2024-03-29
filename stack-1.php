<?php
$startOf = 'file';

include_once 'stack-2.php';

$anonClass = new class() {
    public function anonMethod()
    {
        (new \Foo\Bar\Baz())->bazMethod();
    }
};

$runtimeCreatedFunction = create_function('', <<<'FN'
$dummy = 'foo';
global $anonClass;
$anonClass->anonMethod();
return 4;
FN);

function assert_callback($file, $line, $code)
{
    $dummy = 3;
    global $runtimeCreatedFunction;
    $runtimeCreatedFunction();
}

function init()
{
    assert_options(ASSERT_ACTIVE,     1);
    assert_options(ASSERT_WARNING,    0);
    assert_options(ASSERT_QUIET_EVAL, 1);
    assert_options(ASSERT_CALLBACK,   'assert_callback');

    assert(false, 'is false!!!');
}

init();
