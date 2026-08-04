#!/usr/bin/env bash
drop_into_container() {
    if ! command -v podman >/dev/null 2>&1 
    then
        echo "please install podman, follow your posix-like operating systems docs on how to install podman"
        exit 1
    fi
    podman run --rm -it --name arch -v "$(pwd)":/nyaux:z -w /nyaux docker.io/library/archlinux:latest ./make-distro.sh
}
drop_into_container