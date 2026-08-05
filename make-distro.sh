#!/usr/bin/bash -ex
cat /etc/os-release
PATCHDIR="$PWD/patches"
ROOTDIR="$PWD/distro"
SYSROOT="$ROOTDIR/sysroot"
PREFIX="$ROOTDIR/prefix"
TARGET=x86_64-pc-nyaux-elf
NPROC=$(nproc || 8)

export PATH="$PREFIX/bin:$PATH"

make_ramdisk() {
    # not needed as we mount podman volume as ramdisk
    mkdir -p $1
}

prepare_deps() {
    echo 'DisableSandboxSyscalls' >> /etc/pacman.conf
    pacman -Sy --noconfirm base-devel rustup git meson make wget curl
    rustup default nightly
    git config --global --add safe.directory /nyaux
    git config --global user.email "sample@example.com"
    git config --global user.name "sample"
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

    [ -L "$SYSROOT/include/asm" ] || ln -s "abi-bits" "$SYSROOT/include/asm"

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
cpp_args = ['--sysroot=$SYSROOT', '-D__nyaux__']

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
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER/.git" ] || git -C "$ROOTDIR/$PKGNAME-$PKGVER" init && git add -A && git commit -m "init"
    git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
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
    echo still in build?
    touch $ROOTDIR/build/$PKGNAME-configure
    make -j$NPROC
    make install
}

build_gcc_stage1() {
    which -- $TARGET-as || echo $TARGET-as is not in the PATH
    PKGNAME=gcc
    PKGVER=16.1.0
    [ -f "$ROOTDIR/gcc-$PKGVER.tar.gz" ] || wget -O $ROOTDIR/gcc-$PKGVER.tar.gz https://mirror.koddos.net/gcc/releases/gcc-$PKGVER/gcc-$PKGVER.tar.gz
    [ -d "$ROOTDIR/$PKGNAME-$PKGVER" ] || tar -C $ROOTDIR -xvzf $ROOTDIR/$PKGNAME-$PKGVER.tar.gz
    git -C $ROOTDIR/$PKGNAME-$PKGVER diff > $PATCHDIR/$PKGNAME-$PKGVER.patch
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
            --enable-initfini-array
    touch $ROOTDIR/build/$PKGNAME-configure
    make all-gcc -j$NPROC
    make install-gcc
}

build_gcc_stage2() {
    PKGNAME=gcc
    PKGVER=16.1.0
    cd $ROOTDIR/build/$PKGNAME-$PKGVER
    make all-target-libgcc -j$NPROC
    make install-target-libgcc
}

build_gcc_stage3() {
    PKGNAME=gcc
    PKGVER=16.1.0
    cd $ROOTDIR/build/$PKGNAME-$PKGVER
    make all-target-libstdc++-v3 -j$NPROC
    make install-target-libstdc++-v3
}

build_mlibc_stage1() {
    PKGNAME=mlibc
    [ -d $ROOTDIR/$PKGNAME ] || git clone --depth=1 https://github.com/managarm/mlibc $ROOTDIR/mlibc
    git -C $ROOTDIR/$PKGNAME diff --cached > $PATCHDIR/mlibc.patch

    cp -rv $ROOTDIR/$PKGNAME/options/ansi/include/* $SYSROOT/include/
    cp -rv $ROOTDIR/$PKGNAME/options/posix/include/* $SYSROOT/include/
    cp -rv $ROOTDIR/$PKGNAME/options/internal/include/* $SYSROOT/include/
    cp -rv $ROOTDIR/$PKGNAME/sysdeps/nyaux/include/* $SYSROOT/include/
    cp -rv $PATCHDIR/mlibc-config.h $SYSROOT/include/mlibc-config.h
    mkdir -p "$SYSROOT/include/abi-bits"
    #cp -v $ROOTDIR/$PKGNAME/abis/nyaux/*.h $SYSROOT/include/abi-bits/
}

build_mlibc_stage2() {
    PKGNAME=mlibc
    pushd "$ROOTDIR/$PKGNAME"
    meson setup \
        --prefix="$PREFIX" \
        --cross-file "$ROOTDIR/build/$TARGET.txt" \
        "$ROOTDIR/build/$PKGNAME" \
        -Dposix_option=disabled \
        -Dlinux_option=disabled \
        -Dglibc_option=disabled \
        -Dbsd_option=disabled \
        -Dbuild_tests_host_libc=false \
        -Dlibgcc_dependency=false

    meson compile -C "$ROOTDIR/build/$PKGNAME"
    meson install -C "$ROOTDIR/build/$PKGNAME"
    popd
}

make_ramdisk distro

mkdir -p "$ROOTDIR" "$ROOTDIR/build" "$SYSROOT" "$PREFIX"
prepare_deps
prepare_sysroot

build_binutils
#build_gcc_stage1
#build_mlibc_stage1

#build_gcc_stage2
#build_mlibc_stage2

#build_gcc_stage2
