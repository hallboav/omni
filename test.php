<?php

function baz()
{
    return 'baz';
}

function bar()
{
    return baz();
}

function foo()
{
    return bar();
}

var_dump(foo());
