#!/bin/bash +x

# get the commit ID of the last commit
COMMIT_HASH=`git rev-parse --short HEAD`

QEMU_CONFIG=(
    # build qemu for RISC-V targets
    --target-list=riscv64-softmmu
    # set our own version number including date and commit hash
    --with-pkgversion=hensoldtcyber-"`date +%F`-${COMMIT_HASH}"
    # where to install the modified QEMU
    --prefix=/opt/hc/migv
)

DEB_CONFIG=(
    -y
    # install it in the docker container
    --install=no
    # debian package name
    --pkgname=hc-migv-qemu
    --pkgversion=1
    # Create a monotonically with time increasing version number
    --pkgrelease="`date +"%Y%W%u%H%M"`"
    --maintainer="trentos@hensoldt-cyber.com"
)

mkdir -p ../build_output
cd ../build_output
../migv-qemu/configure "${QEMU_CONFIG[@]}"
make
sudo checkinstall "${DEB_CONFIG[@]}"