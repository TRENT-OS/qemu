#!/bin/bash +x

# get the commit ID of the last commit
COMMIT_HASH=`git rev-parse --short HEAD`

    QEMU_TARGETS=(
        i386
        x86_64
        arm
        aarch64
        riscv32
        riscv64
        #microblazeel
    )
QEMU_CONFIG=(
    --static # aim for a stand-alone binary with few library dependencies
    --target-list=$(IFS=,; echo "${QEMU_TARGETS[*]/%/-softmmu}")
    --audio-drv-list="" # don't require libpulse
    --disable-brlapi
    --disable-gio
    --disable-gtk
    --disable-kvm
    --disable-libiscsi
    --disable-libnfs
    --disable-pa
    --disable-rbd
    --disable-sdl
    --disable-snappy
    --disable-vnc
    --disable-xen
    # set our own version number including date and commit hash
    --with-pkgversion=hensoldtcyber-"`date +%F`-${COMMIT_HASH}"
    # where to install the modified QEMU
    --prefix=/opt/hc
)

DEB_CONFIG=(
    -y
    # install it in the docker container
    --install=no
    # debian package name
    --pkgname=hc-qemu
    --pkgversion=1
    # Create a monotonically with time increasing version number
    --pkgrelease="`date +"%Y%W%u%H%M"`"
    --maintainer="info.cyber@hensoldt.net"
    # piggyback on the unmodified version to sort out dependencies
    --requires="qemu-system-arm \\( \\>= 7.1 \\)"
)

mkdir -p ../build_output
(
    cd ../build_output
    ../qemu/configure "${QEMU_CONFIG[@]}"
    make -j
    sudo checkinstall "${DEB_CONFIG[@]}"
)
