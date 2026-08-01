#!/usr/local/bin/bash -ex

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

prepare_sysroot() {
    mkdir -p \
        "$SYSROOT/usr" \
        "$SYSROOT/bin" \
        "$SYSROOT/include" \
        "$SYSROOT/lib" \
        "$SYSROOT/lib32" \
        "$SYSROOT/libexec" \
        "$SYSROOT/local" \
        "$SYSROOT/sbin" \
        "$SYSROOT/share"

    [ -L "$SYSROOT/usr/bin" ] || ln -s "../bin" "$SYSROOT/usr/bin"
    [ -L "$SYSROOT/usr/include" ] || ln -s "../include" "$SYSROOT/usr/include"
    [ -L "$SYSROOT/usr/lib" ] || ln -s "../lib" "$SYSROOT/usr/lib"
    [ -L "$SYSROOT/usr/lib32" ] || ln -s "../lib32" "$SYSROOT/usr/lib32"
    [ -L "$SYSROOT/usr/libexec" ] || ln -s "../libexec" "$SYSROOT/usr/libexec"
    [ -L "$SYSROOT/usr/local" ] || ln -s "../local" "$SYSROOT/usr/local"
    [ -L "$SYSROOT/usr/sbin" ] || ln -s "../sbin" "$SYSROOT/usr/sbin"
    [ -L "$SYSROOT/usr/share" ] || ln -s "../share" "$SYSROOT/usr/share"

    cat <<EOF >"$ROOTDIR/build/$TARGET.txt"
[binaries]
c = '$TARGET-gcc'
cpp = '$TARGET-g++'
ar = '$TARGET-ar'
strip = '$TARGET-strip'
pkg-config = 'none'

[built-in options]
c_args = ['--sysroot=$SYSROOT', '-D__nyaux__']
c_link_args = []
cpp_args = ['--sysroot=$SYSROOT', '-D__nyaux__']
cpp_link_args = []

[host_machine]
system = 'nyaux'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
EOF
}

build_binutils() {
    PKGNAME=binutils
    PKGVER=2.47
    [ -f "$ROOTDIR/$PKGNAME-$PKGVER.tar.gz" ] || wget -O $ROOTDIR/binutils-$PKGVER.tar.gz https://ftp.gnu.org/gnu/binutils/binutils-$PKGVER.tar.gz
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/binutils-$PKGVER.tar.gz
    #git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
    [ -f $ROOTDIR/build/$PKGNAME-patched ] || git -C $ROOTDIR/$PKGNAME-$PKGVER apply $PATCHDIR/$PKGNAME-$PKGVER.patch
    touch $ROOTDIR/build/$PKGNAME-patched

    mkdir -p $ROOTDIR/build/$PKGNAME-$PKGVER
    cd $ROOTDIR/build/$PKGNAME-$PKGVER
    [ -f $ROOTDIR/build/$PKGNAME-configure ] || ../../$PKGNAME-$PKGVER/configure \
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

build_gcc_stage1() {
    which -- $TARGET-as || echo $TARGET-as is not in the PATH
    PKGNAME=gcc
    PKGVER=16.1.0
    [ -f "$ROOTDIR/gcc-$PKGVER.tar.gz" ] || wget -O $ROOTDIR/gcc-$PKGVER.tar.gz https://mirror.koddos.net/gcc/releases/gcc-$PKGVER/gcc-$PKGVER.tar.gz
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/$PKGNAME-$PKGVER.tar.gz
    #git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
    [ -f $ROOTDIR/build/$PKGNAME-patched ] || git -C $ROOTDIR/$PKGNAME-$PKGVER apply $PATCHDIR/$PKGNAME-$PKGVER.patch
    touch $ROOTDIR/build/$PKGNAME-patched
    mkdir -p $ROOTDIR/build/$PKGNAME-$PKGVER
    cd $ROOTDIR/build/$PKGNAME-$PKGVER
    [ -f $ROOTDIR/build/$PKGNAME-configure ] || ../../$PKGNAME-$PKGVER/configure \
            --target=$TARGET \
            --prefix="$PREFIX" \
            --with-sysroot="$SYSROOT" \
            --disable-bootstrap \
            --disable-nls \
            --enable-languages=c,c++ \
            --enable-initfini-array \
            --without-headers
    touch $ROOTDIR/build/$PKGNAME-configure
    gmake all-gcc -j$NPROC
    gmake all-target-libgcc -j$NPROC
    gmake install-gcc
    gmake install-target-libgcc
}

build_gcc_stage2() {
    PKGNAME=gcc
    PKGVER=16.1.0
    cd $ROOTDIR/build/$PKGNAME-$PKGVER
    gmake all-target-libstdc++-v3 -j$NPROC
    gmake install-target-libstdc++-v3
}

build_libc() {
    [ -d $ROOTDIR/mlibc ] || git clone --depth=1 https://github.com/managarm/mlibc $ROOTDIR/mlibc
    git -C $ROOTDIR/mlibc diff --cached > $PATCHDIR/mlibc.patch

    pushd "$ROOTDIR/mlibc"
    meson setup \
        --prefix="$PREFIX" \
        --cross-file "$ROOTDIR/build/$TARGET.txt" \
        "$ROOTDIR/build/mlibc"
    meson compile -C "$ROOTDIR/build/mlibc"
    meson install -C "$ROOTDIR/build/mlibc"
    popd
}

[ -f "$ROOTDIR/.ramdisk" ] || make_ramdisk distro

mkdir -p "$ROOTDIR" "$ROOTDIR/build" "$SYSROOT" "$PREFIX"
prepare_sysroot
build_binutils
build_gcc_stage1
build_libc
build_gcc_stage2
