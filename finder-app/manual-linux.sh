#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]; then
    echo "Using default directory ${OUTDIR} for output"
else
    OUTDIR=$(realpath "$1")
    echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p "${OUTDIR}"

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    # Clone only if the repository does not exist.
    echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Kernel build steps: clean, defconfig, build Image and dtbs only
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) Image
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) dtbs

    cd ..
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]; then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # Configure busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# Make and install busybox
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "program interpreter"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "Shared library"

# Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -a $SYSROOT/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp -a $SYSROOT/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp -a $SYSROOT/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/

# Make device nodes
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

# Copy the finder related scripts and executables to the /home directory on the target rootfs
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home

# Fix paths and shebangs
sed -i 's|\.\./conf/assignment.txt|assignment.txt|' ${OUTDIR}/rootfs/home/finder-test.sh
sed -i 's|conf/username.txt|username.txt|' ${OUTDIR}/rootfs/home/finder-test.sh
sed -i 's|#!/bin/bash|#!/bin/sh|' ${OUTDIR}/rootfs/home/finder.sh

# Change ownership to root
sudo chown -R root:root ${OUTDIR}/rootfs

# Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz

echo "Build complete! Kernel Image and initramfs are ready in ${OUTDIR}"

