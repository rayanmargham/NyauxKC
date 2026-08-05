#!/usr/bin/env bash

mkdir -p "$PWD/workspace"
[ -f "$PWD/workspace/.ramdisk" ] || mount -t tmpfs "$PWD/workspace" "$PWD/workspace"
touch "$PWD/workspace/.ramdisk"
cp -rv "$PWD/patches" "$PWD/workspace/patches"
cp -v "$PWD/make-distro.sh" "$PWD/workspace/"

if ! command -v podman >/dev/null 2>&1 
then
    echo "please install podman, follow your posix-like operating systems docs on how to install podman"
    exit 1
fi
sudo podman run \
    --rm -it --os=linux \
    --name arch \
    --security-opt seccomp=unconfined \
    -v "$PWD/workspace":/nyaux:z \
    -w /nyaux docker.io/library/archlinux:latest \
    ./make-distro.sh

sudo chown -R $USER "$PWD/workspace"
