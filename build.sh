#!/bin/bash

cc=gcc
src=*.c
exe=3Dpi
std='-std=c99'

if echo "$OSTYPE" | grep -q "linux"; then
    rflag="-Wl,--whole-archive"
    lflag="-Wl,--no-whole-archive"
fi

flags=(
    $std
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
    -mmacos-version-min=10.9
)


compile() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        $cc ${flags[*]} ${inc[*]} ${lib[*]} ${mac[*]} $src -o $exe
    elif echo "$OSTYPE" | grep -q "linux"; then
        $cc ${flags[*]} ${inc[*]} ${lib[*]} ${linux[*]} $src -o $exe
    else
        echo "OS is not supported yet..." && exit
    fi
}

build_lib() {
    pushd $1/ && ./build.sh $2 && mv *.a ../lib/ && popd
}

build() {
    mkdir lib/
    build_lib fract -s
    build_lib utopia -slib
    build_lib imgtool -slib
    build_lib mass -s
    build_lib glee -s
    build_lib gleex -s
}

clean() {
    rm -r lib && rm $exe
}

fail() {
    echo "Use 'comp' to compile game and 'build' to build libraries." && exit
}

case "$1" in
    "run")
        shift
        compile && ./$exe "$@";;
    "comp")
        compile;;
    "build")
        build;;
    "all")
        shift
        build && compile && ./$exe "$@";;
    "clean")
        clean;;
    *)
        fail;;
esac
