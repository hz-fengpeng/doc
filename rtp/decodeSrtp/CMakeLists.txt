cmake_minimum_required(VERSION 2.8)
project(decodeSrtp)

include_directories("/usr/local/include")
include_directories("/home/fengpeng/decodeSrtp/srtp2/include/srtp2")
link_directories("/usr/local/lib")
link_directories("/home/fengpeng/decodeSrtp/srtp2/lib")
add_executable(decodeSrtp main.cpp)
target_link_libraries(decodeSrtp -lpcap libsrtp2.a libcrypto.a libssl.a -ldl ) 