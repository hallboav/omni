<?php

namespace Foo\Bar;

function normal()
{
    $do = 'nothing';

    echo PHP_EOL, PHP_EOL;
    echo '---- debug_print_backtrace() ----', PHP_EOL;
    debug_print_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);

    echo PHP_EOL, PHP_EOL;
    echo '---- Exception ----', PHP_EOL;
    echo (new \Exception())->getTraceAsString(), PHP_EOL;
    echo PHP_EOL, PHP_EOL;
}

trait CustomTrait
{
    public function traitMethod()
    {
        normal();
    }
}

class Qux
{
    use CustomTrait;

    public function __invoke()
    {
        $this->traitMethod();
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
