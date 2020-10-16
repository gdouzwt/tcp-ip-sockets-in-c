cmake_minimum_required(VERSION 3.17)
project(test C)

set(CMAKE_C_STANDARD 99)

#set(CMAKE_C_COMPILER "/home/tao/Downloads/openwrt-sdk-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-8.4.0_musl/bin/mipsel-openwrt-linux-gcc")
#set(CMAKE_CXX_COMPILER "/home/tao/Downloads/openwrt-sdk-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-8.4.0_musl/bin/mipsel-openwrt-linux-g++")
#SET(CMAKE_AR "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-uclibc-ar")
#SET(CMAKE_LINKER "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-uclibc-ld")
#SET(CMAKE_RANLIB "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-ranlib")
#SET(CMAKE_NM "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-nm")
#SET(CMAKE_OBJDUMP "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-objdump")
#SET(CMAKE_OBJCOPY "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-objcopy")
#SET(CMAKE_STRIP "/home/test/openwrt/Toolchain/toolchain/bin/mipsel-openwrt-linux-strip")

include_directories(.)

add_executable(TCPEchoClient4
        TCPEchoClient4.c
        DieWithMessage.c)

add_executable(TCPEchoServer4
        TCPEchoServer4.c
        TCPServerUtility.c
        DieWithMessage.c
        AddressUtility.c)