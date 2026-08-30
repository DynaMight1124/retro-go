ANDROIDVERSION=30
OS_NAME=linux-x86_64
TOOLCHAINS=$(find ${ANDROID_HOME} -type d -name toolchains -maxdepth 3)
CC_ARM64=${TOOLCHAINS}/llvm/prebuilt/${OS_NAME}/bin/aarch64-linux-android${ANDROIDVERSION}-clang++
CC=${CC_ARM64}
CXX=${CC_ARM64}
AR=${TOOLCHAINS}/llvm/prebuilt/${OS_NAME}/bin/aarch64-linux-android-ar
RANLIB=${TOOLCHAINS}/llvm/prebuilt/${OS_NAME}/bin/aarch64-linux-android-ranlib
CFLAGS="-O2"
CFLAGS=$CFLAGS CXX=$CXX CC=$CC AR=$AR RANLIB=$RANLIB make -j liblua.a
