#!/bin/bash +x

# get the commit ID of the last commit
COMMIT_HASH=`git rev-parse --short HEAD`

QEMU_CONFIG=(
    # build qemu for ARM targets
    --target-list=arm-softmmu,arm-linux-user
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
    --maintainer="trentos@hensoldt-cyber.com"
    # piggyback on the unmodified version to sort out dependencies
    --requires="qemu-system-arm \\( \\>= 4.2 \\)"
)

mkdir -p ../build_output
cd ../build_output
../qemu/configure "${QEMU_CONFIG[@]}"
make
sudo checkinstall "${DEB_CONFIG[@]}"