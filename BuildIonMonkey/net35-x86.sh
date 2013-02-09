cd /C/Data/ionmonkey/nsprpub
rm build-release-net35-x86 -rf
mkdir build-release-net35-x86
 
cd build-release-net35-x86
../configure \
    --host=x86_64-pc-mingw32 --target=x86_64-pc-mingw32 \
    --disable-debug --enable-optimize
 
make

cd /C/Data/ionmonkey/js/src
autoconf-2.13
 
rm build-release-net35-x86 -rf
mkdir build-release-net35-x86
cd build-release-net35-x86
../configure \
   --host=x86_64-pc-mingw32 --target=i686-pc-mingw32 \
   --enable-ctypes --enable-threadsafe \
   --with-nspr-cflags="-IC:/Data/ionmonkey/nsprpub/build-release-net35-x86/dist/include/nspr" \
   --with-nspr-libs=" \
      C:\Data\ionmonkey\nsprpub\build-release-net35-x86\dist\lib\libnspr4.lib \
      C:\Data\ionmonkey\nsprpub\build-release-net35-x86\dist\lib\libplds4.lib \
      C:\Data\ionmonkey\nsprpub\build-release-net35-x86\dist\lib\libplc4.lib"
make