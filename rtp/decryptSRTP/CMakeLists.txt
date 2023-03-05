project(decryptSRTP)

set(includeDir ${CMAKE_CURRENT_SOURCE_DIR}/srtp2/include/srtp2)
set(libDir ${CMAKE_CURRENT_SOURCE_DIR}/srtp2/lib)

include_directories("${includeDir}")
link_directories("${libDir}")
add_executable(decryptSRTP main.cpp)
#target_link_libraries(decryptSRTP -lpcap libsrtp2.a libcrypto.a libssl.a -ldl ) 
target_link_libraries(decryptSRTP -lpcap libsrtp2.a -lcrypto -ldl ) 