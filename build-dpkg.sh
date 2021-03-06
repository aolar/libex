#!/bin/sh

mkdir -p libex/usr/include/libex
mkdir -p libex/usr/lib/pkgconfig
mkdir -p libex/usr/share/doc/libex-dev
cp libex.pc libex/usr/lib/pkgconfig
cp .zbuild/_standard/_libs/libex.a libex/usr/lib
cp include/libex/* libex/usr/include/libex
cp copyright libex/usr/share/doc/libex-dev
cp changelog libex/usr/share/doc/libex-dev/changelog.Debian
gzip libex/usr/share/doc/libex-dev/changelog.Debian

cd libex
md5deep -r -l usr > DEBIAN/md5sums
cd ..
fakeroot dpkg-deb --build libex
mv libex.deb libex-dev_`cat libex/DEBIAN/control | grep Version | awk '{print $2}'`_`cat libex/DEBIAN/control | grep Architecture | awk '{print $2}'`.deb
rm -fr libex/usr/*
rmdir libex/usr
rm libex/DEBIAN/md5sums
