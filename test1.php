<?php

require_once 'test2.php';

// $anonClass = new class() {
//     public function anonMethod()
//     {
//     }
// };

// $closure = function() {
// };

// function normal() {
// }

// $anonClass->anonMethod();
// $closure();
// normal();

(new \Foo\Bar\Baz)->bazMethod();
\Foo\Bar\Baz::staticMethod();


// $createdFunction = create_function('', '');
// $createdFunction();
// assert('2 === 2');
