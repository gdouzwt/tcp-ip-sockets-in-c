cmake_minimum_required(VERSION 3.17)
project(test C)

set(CMAKE_C_STANDARD 99)

#SET(CMAKE_C_COMPILER "/home/tao/Downloads/openwrt-sdk-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-8.4.0_musl/bin/mipsel-openwrt-linux-gcc")
#SET(CMAKE_CXX_COMPILER "/home/tao/Downloads/openwrt-sdk-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-8.4.0_musl/bin/mipsel-openwrt-linux-g++")
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

add_executable(TCPEchoServer6
        TCPEchoServer6.c
        TCPServerUtility.c
        DieWithMessage.c
        AddressUtility.c)

add_executable(TCPEchoClient
        TCPEchoClient.c
        TCPClientUtility.c
        DieWithMessage.c)

add_executable(TCPEchoServer
        TCPEchoServer.c
        TCPServerUtility.c
        DieWithMessage.c
        AddressUtility.c)

add_executable(GetAddrInfo
               GetAddrInfo.c
               DieWithMessage.c
               AddressUtility.c)

add_executable(UDPEchoClient
               UDPEchoClient.c
               DieWithMessage.c
               AddressUtility.c)

add_executable(UDPEchoServer
               UDPEchoServer.c
               DieWithMessage.c
               AddressUtility.c)

add_executable(TestSizes TestSizes.c)

add_executable(MulticastReceiver
               MulticastReceiver.c
               DieWithMessage.c
               AddressUtility.c)

add_executable(encrypt encrypt.c)
target_link_libraries(encrypt crypto)

add_executable(JsonTest JsonTest.c)
target_link_libraries(JsonTest json-c)

add_executable(uboxDemo uboxDemo.c)
target_link_libraries(uboxDemo ubox)

add_executable(client client.c DieWithMessage.c
        AddressUtility.c)

add_executable(uboxClient uboxClient.c)
target_link_libraries(uboxClient ubox json-c crypto)

# set(CMAKE_C_FLAGS "-Werror=deprecated-declarations")
# target_compile_options(JsonTest PRIVATE "-Werror=deprecated-declarations")


