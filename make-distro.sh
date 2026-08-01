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
    NAME=binutils
    VERSION=2.47
    [ -f "$ROOTDIR/$PKGNAME-$PKGVER.tar.gz" ] || wget -O $ROOTDIR/binutils-$PKGVER.tar.gz https://ftp.gnu.org/gnu/binutils/binutils-$PKGVER.tar.gz
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/binutils-$PKGVER.tar.gz
    #git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
    [ -f $ROOTDIR/build/$PKGNAME-patched ] || git -C $ROOTDIR/$PKGNAME-$PKGVER apply $PATCHDIR/$PKGNAME-$PKGVER.patch
    touch $ROOTDIR/build/$PKGNAME-patched

    mkdir -p $ROOTDIR/build/$PKGNAME-$PKGVER
    [ -f $ROOTDIR/build/$PKGNAME-configure ] || cd $ROOTDIR/build/$PKGNAME-$PKGVER \
        && ../../$PKGNAME-$PKGVER/configure \
            --target=$TARGET \
            --prefix="$PREFIX" \
            --with-sysroot="$SYSROOT" \
            --disable-nls \
            --disable-werror \
            --enable-default-execstack=no
    touch $ROOTDIR/build/$PKGNAME-configure
    gmake -j$NPROC
    gmake install
}

build_gcc() {
    which -- $TARGET-as || echo $TARGET-as is not in the PATH

    VERSION=16.1.0
    [ -f "$ROOTDIR/gcc-$PKGVER.tar.gz" ] || wget -O $ROOTDIR/gcc-$PKGVER.tar.gz https://mirror.koddos.net/gcc/releases/gcc-$PKGVER/gcc-$PKGVER.tar.gz
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/$PKGNAME-$PKGVER.tar.gz
    #git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
    [ -f $ROOTDIR/build/$PKGNAME-patched ] || git -C $ROOTDIR/$PKGNAME-$PKGVER apply $PATCHDIR/$PKGNAME-$PKGVER.patch
    touch $ROOTDIR/build/$PKGNAME-patched

    mkdir -p $ROOTDIR/build/$PKGNAME-$PKGVER
    [ -f $ROOTDIR/build/$PKGNAME-configure ] || cd $ROOTDIR/build/$PKGNAME-$PKGVER \
        && ../../$PKGNAME-$PKGVER/configure \
            --target=$TARGET \
            --prefix="$PREFIX" \
            --with-sysroot="$SYSROOT" \
            --disable-bootstrap \
            --disable-nls \
            --enable-languages=c,c++ \
            --enable-initfini-array
    touch $ROOTDIR/build/$PKGNAME-configure
    gmake all-gcc -j$NPROC
    gmake all-target-libgcc -j$NPROC
    gmake all-target-libstdc++-v3 -j$NPROC
    gmake install-gcc
    gmake install-target-libgcc
    gmake install-target-libstdc++-v3
}

build_libc() {
    [ -d $ROOTDIR/mlibc ] || git clone --depth=1 https://github.com/managarm/mlibc $ROOTDIR/mlibc
    git -C $ROOTDIR/mlibc diff > $PATCHDIR/mlibc.patch
}

[ -f "$ROOTDIR/.ramdisk" ] || make_ramdisk distro

mkdir -p "$ROOTDIR" "$ROOTDIR/build" "$SYSROOT" "$PREFIX"

build_binutils
build_gcc
build_libc
