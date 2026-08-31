PREFIX=/opt/devkitpro/devkitARM/bin/arm-none-eabi
CC_ARM64=${PREFIX}-g++
CC=${CC_ARM64}
CXX=${CC_ARM64}
AR=${PREFIX}-ar
RANLIB=${PREFIX}-ranlib
ARCH="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -O2"
CFLAGS="${ARCH} -mword-relocations -D__3DS__"
LDFLAGS="-specs=3dsx.specs -g ${ARCH}"
set -x
CXX="${CXX} ${CFLAGS}"
MYLDFLAGS="${LDFLAGS}" LDFLAGS="${LDFLAGS}" CFLAGS="${CFLAGS}" MYCFLAGS="${CFLAGS}" CXX=$CXX CC=$CC AR=$AR RANLIB=$RANLIB make -j liblua.a

