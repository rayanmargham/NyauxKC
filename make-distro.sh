#!/bin/sh -ex

PATCHDIR="$PWD/patches"
ROOTDIR="$PWD/distro"
SYSROOT="$ROOTDIR/sysroot"
PREFIX="$ROOTDIR/prefix"
TARGET=x86_64-pc-nyaux-elf
NPROC=8

export PATH="$PREFIX/bin:$PATH"

make_ramdisk() {
    mkdir -p $1
    mount -t tmpfs $1 $1
    touch $1/.ramdisk
}

build_binutils() {
    VERSION=2.47
    [ -f "$ROOTDIR/binutils-$VERSION.tar.gz" ] || wget -O $ROOTDIR/binutils-$VERSION.tar.gz https://ftp.gnu.org/gnu/binutils/binutils-$VERSION.tar.gz
    [ -d "$ROOTDIR/binutils-$VERSION" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/binutils-$VERSION.tar.gz
    git -C $ROOTDIR/binutils-$VERSION diff > $PATCHDIR/binutils-$VERSION.patch

    mkdir -p $ROOTDIR/build/binutils-$VERSION
    cd $ROOTDIR/build/binutils-$VERSION \
        && ../../binutils-$VERSION/configure \
            --target=$TARGET \
            --prefix="$PREFIX" \
            --with-sysroot="$SYSROOT" \
            --disable-nls \
            --disable-werror \
            --enable-default-execstack=no
    gmake -j$NPROC
    gmake install
}

build_gcc() {
    which -- $TARGET-as || echo $TARGET-as is not in the PATH

    VERSION=16.1.0
    [ -f "$ROOTDIR/gcc-$VERSION.tar.gz" ] || wget -O $ROOTDIR/gcc-$VERSION.tar.gz https://mirror.koddos.net/gcc/releases/gcc-$VERSION/gcc-$VERSION.tar.gz
    [ -d "$ROOTDIR/gcc-$VERSION" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/gcc-$VERSION.tar.gz
    git -C $ROOTDIR/gcc-$VERSION diff > $PATCHDIR/gcc-$VERSION.patch

    mkdir -p $ROOTDIR/build/gcc-$VERSION
    cd $ROOTDIR/build/gcc-$VERSION \
        && ../../gcc-$VERSION/configure \
            --target=$TARGET \
            --prefix="$PREFIX" \
            --with-sysroot="$SYSROOT" \
            --disable-bootstrap \
            --disable-nls \
            --enable-languages=c \
            --enable-initfini-array
    gmake all-gcc -j$NPROC
    gmake all-target-libgcc -j$NPROC
    gmake all-target-libstdc++-v3 -j$NPROC
    gmake install-gcc
    gmake install-target-libgcc
    gmake install-target-libstdc++-v3
}

build_libc() {
    [ -d $ROOTDIR/mlibc ] || git clone --depth=1 https://github.com/managarm/mlibc distro/mlibc
    git -C $ROOTDIR/mlibc add .
    git -C $ROOTDIR/mlibc diff
}

[ -f "$ROOTDIR/.ramdisk" ] || make_ramdisk distro

mkdir -p "$ROOTDIR"
mkdir -p "$SYSROOT"
mkdir -p "$PREFIX"

build_binutils
build_gcc
build_libc
