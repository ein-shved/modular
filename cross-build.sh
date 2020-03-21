#!/bin/bash

usage() {
    local out="${1-1}"
    cat >&$out << EOUSAGE
Usage: $0 [ARGS] [ -- ] [MAKEARGS]

ARGS are:
    -h              Build for host too.
    -M  MCU         The type of AVR the application is built for.
    -L  L_FUSE      The LOW fuse value for MCU.
    -H  H_FUSE      The HIGH fuse value for MCU.
    -U  UPLOADER    The application used to upload to the MCU (default: avrdude).
    -P  PORT        The port used for the upload tool (default: usb).
    -R  PROGRAMMER  The programmer hardware used (default: usbasp).

If neither -M not -h arguments specified - scripts builds for host and atmega
by default.
EOUSAGE
}

NOT=
MAKEOPTS=
while getopts ":hM:L:H:U:P:R:?u" OPT; do
    case $OPT in
    "\?"|u) usage 1; exit 0;;
    h) BUILD_HOST=y;;
    M) AVR_MCU="$AVR_MCU $OPTARG";;
    L) AVR_L_FUSE="$OPTARG";;
    H) AVR_H_FUSE="$OPTARG";;
    U) AVR_UPLOADTOOL="$OPTARG";;
    P) AVR_UPLOADTOOL_PORT="$OPTARG";;
    R) AVR_PROGRAMMER="$OPTARG";;
    :) [ "$OPTARG" == "--" ] && break || MAKEOPTS="$MAKEOPTS $OPTARG";;
    esac
done

shift $(($OPTIND - 1))

MAKEOPTS="$MAKEOPTS $@"

if [ -z "$BUILD_HOST" ] && [ -z "$AVR_MCU" ]; then
    BUILD_HOST=y
    AVR_MCU=atmega8
fi

AVR_UPLOADTOOL=${AVR_UPLOADTOOL-avrdude}
AVR_UPLOADTOOL_PORT=${AVR_UPLOADTOOL_PORT-usb}
AVR_PROGRAMMER=${AVR_PROGRAMMER-usbasp}

SRCROOT="$PWD"
AVR_TOOLCHAIN="$SRCROOT/cmake-avr/generic-gcc-avr.cmake"


run_cmake() {
    local dir="$1"
    local ret
    shift

    mkdir -p "$dir"
    cd "$dir"
    cmake "$SRCROOT" "$@"
    ret=$?
    cd "$srcroot"
    return $ret
}

run_make() {
    local dir="$1"
    shift
    cd "$dir"
    make "$@"
    ret=$?
    cd "$srcroot"
    return $ret
}

if [ "x$BUILD_HOST" == "xy" ]; then
    run_cmake "$SRCROOT/build/host" || exit $?
fi
for mcu in $AVR_MCU; do
    run_cmake "$SRCROOT/build/$mcu"                                         \
        -DCMAKE_TOOLCHAIN_FILE="$SRCROOT/cmake-avr/generic-gcc-avr.cmake"   \
        -DAVR_MCU="$mcu"                                                    \
        -DAVR_UPLOADTOOL="$AVR_UPLOADTOOL"                                  \
        -DAVR_UPLOADTOOL_PORT="$AVR_UPLOADTOOL_PORT"                        \
        -DAVR_PROGRAMMER="$AVR_PROGRAMMER"                                  \
    || exit $?
done

if [ "x$BUILD_HOST" == "xy" ]; then
    run_make "$SRCROOT/build/host" $MAKEOPTS || exit $?
fi
for mcu in $AVR_MCU; do
    run_make "$SRCROOT/build/$mcu" $MAKEOPTS || exit $?
done
