
cd /C/Data/ionmonkey/nsprpub
rm build-release-net40-amd64 -rf
mkdir build-release-net40-amd64
 
cd build-release-net40-amd64
../configure \
    --host=x86_64-pc-mingw32 --target=x86_64-pc-mingw32 \
    --enable-64bit \
    --disable-debug --enable-optimize
 
make

cd /C/Data/ionmonkey/js/src
autoconf-2.13
 
rm build-release-net40-amd64 -rf
mkdir build-release-net40-amd64
cd build-release-net40-amd64
../configure \
   --host=x86_64-pc-mingw32 --target=x86_64-pc-mingw32 \
   --enable-ctypes --enable-threadsafe \
   --with-nspr-cflags="-IC:/Data/ionmonkey/nsprpub/build-release-net40-amd64/dist/include/nspr" \
   --with-nspr-libs=" \
      C:\Data\ionmonkey\nsprpub\build-release-net40-amd64\dist\lib\libnspr4.lib \
      C:\Data\ionmonkey\nsprpub\build-release-net40-amd64\dist\lib\libplds4.lib \
      C:\Data\ionmonkey\nsprpub\build-release-net40-amd64\dist\lib\libplc4.lib"
make