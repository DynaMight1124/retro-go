#!/bin/sh -e

if [ $# -ne 1 ]; then
	echo "Usage: $0 platform"
	echo "	platforms: funkey-s, gcw0, retrofw"

	exit 1
fi

rm -f *.opk
make distclean
#convert textures/segment2/segment2.05A00.rgba16.png -resize 32x32! build/icon.png
#mksquashfs build/us_pc/sm64.us.f3dex2e ./run.sh build/icon.png default."$1".desktop sm64-port-"$1".opk
if [ -e ./baserom.us.z64 ]; then
	make -j16 VERSION=us
	mksquashfs build/us_funkey/sm64.us.f3dex2e opk/sm64.png opk/sm64_us."$1".desktop sm64_us_v1.3_"$1".opk
fi
if [ -e ./baserom.jp.z64 ]; then
	make -j16 VERSION=jp
	mksquashfs build/jp_funkey/sm64.jp.f3dex2e opk/sm64.png opk/sm64_jp."$1".desktop sm64_jp_v1.3_"$1".opk
fi
