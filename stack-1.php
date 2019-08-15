<?php
$startOf = 'file';

function baz()
{
    $do = 'nothing';

    echo PHP_EOL, PHP_EOL, PHP_EOL, PHP_EOL;
    echo '---- debug_print_backtrace() ----', PHP_EOL;
    debug_print_backtrace();

    echo PHP_EOL, PHP_EOL;
    echo '---- Exception ----', PHP_EOL;
    echo (new \Exception())->getTraceAsString(), PHP_EOL;
}

function bar()
{
    baz();
}

function foo()
{
    bar();
}

foo();
