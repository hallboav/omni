<?php
$startOf = 'file';

include_once 'stack-2.php';

trait CustomTrait
{
    public function traitMethod()
    {
        (new \Foo\Bar\Baz)->bazMethod();
    }
}

$anonClass = new class() {
    use CustomTrait;

    public function anonMethod()
    {
        $this->traitMethod();
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
