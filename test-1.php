<?php

require_once 'test-2.php';

$anonClass = new class() {
    public function anonMethod()
    {
    }
};

$closure = function() {
};

function normal() {
}

$closure();
normal();

$anonClass->anonMethod();
(new \Foo\Bar\Baz)->bazMethod();
\Foo\Bar\Baz::staticMethod();

$createdFunction = create_function('$a,$b', 'return $a + $b;');
$createdFunction(3, 2);
assert('2 === 2');
