<?php

namespace App\Foo\Bar {
    class Baz
    {
        public function bazMethod()
        {
            return 'Baz::bazMethod';
        }
    }
}

namespace App {
    $anonClass = new class() {
        public function anonMethod()
        {
            return 'anonymous method';
        }
    };

    $closure = function() {
        return 'closure function';
    };

    echo $closure(), PHP_EOL;
    echo $anonClass->anonMethod(), PHP_EOL;
    echo (new \App\Foo\Bar\Baz)->bazMethod(), PHP_EOL;
}
