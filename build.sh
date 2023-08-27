#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -Werror `pkg-config --cflags sdl2`"
LIBS=`pkg-config --libs sdl2`

cc $CFLAGS -o chip8.test $LIBS chip8_test.c
cc $CFLAGS -o chip8.sdl $LIBS chip8_sdl.c
