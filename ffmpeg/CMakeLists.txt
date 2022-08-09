cmake_minimum_required(VERSION 2.8)
project(decode)

include_directories(/home/fengpeng/ffmpeg/build/include)
link_directories(/home/fengpeng/ffmpeg/build/lib)
add_executable(decode decode.c)
target_link_libraries(decode libavformat.a libavdevice.a libavcodec.a libavutil.a libswscale.a  libswresample.a -lm -lz -lpthread) 