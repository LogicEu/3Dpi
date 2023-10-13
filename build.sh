#!/bin/bash

cc=gcc
src=*.c
exe=3Dpi

if echo "$OSTYPE" | grep -q "linux"; then
    rflag="-Wl,--whole-archive"
    lflag="-Wl,--no-whole-archive"
fi

flags=(
    -std=c99
    -Wall
    -Wextra
    -O2
)

inc=(
    -Imass/
    -Ifract/
    -Iutopia/
    -Iglee/
    -Igleex/
    -Iimgtool/
)

lib=(
    -Llib/
    $rflag
    -lfract
    -lutopia
    -lglee
    -lgleex
    -limgtool
    -lmass
    $lflag
    -lglfw
    -lfreetype
    -lz
    -lpng
    -ljpeg
)

linux=(
    -lm
    -lGL
    -lGLEW
)

mac=(
    -framework OpenGL
)

compile() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        $cc ${flags[*]} ${inc[*]} ${lib[*]} ${mac[*]} $src -o $exe
    elif echo "$OSTYPE" | grep -q "linux"; then
        $cc ${flags[*]} ${inc[*]} ${lib[*]} ${linux[*]} $src -o $exe
    else
        echo "This OS is not supported yet..." && exit
    fi
}

build_lib() {
    pushd $1/ && ./build.sh $2 && mv *.a ../lib/ && popd
}

build() {
    mkdir lib/
    build_lib fract static
    build_lib utopia static
    build_lib imgtool static
    build_lib mass static
    build_lib glee static
    build_lib gleex -s
}

clean() {
    [ -d lib ] && rm -r lib && echo "deleted lib"
    [ -f $exe ] && rm $exe && echo "deleted $exe"
    return 0
}

case "$1" in
    "comp")
        compile;;
    "build")
        build;;
    "all")
        build && compile;;
    "clean")
        clean;;
    *)
        echo "Run with 'build' or 'comp' to compile dependencies and executable"
        echo "Use 'clean' to remove local builds";;
esac
