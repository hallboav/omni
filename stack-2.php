<?php

namespace Foo\Bar;

function normal()
{
    $do = 'nothing';

    echo PHP_EOL, PHP_EOL;
    echo '---- debug_print_backtrace() ----', PHP_EOL;
    debug_print_backtrace();

    echo PHP_EOL, PHP_EOL;
    echo '---- Exception ----', PHP_EOL;
    echo (new \Exception())->getTraceAsString(), PHP_EOL;
    echo PHP_EOL, PHP_EOL;
}

class Qux
{
    public function __invoke()
    {
        normal();
    }
}

class Baz
{
    public function bazMethod()
    {
        self::staticMethod();
    }

    public static function staticMethod()
    {
        $qux = new Qux();
        $qux();
    }
}
