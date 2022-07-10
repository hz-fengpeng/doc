ffmpeg udp 推流
ffmpeg.exe -re -i juren.mp4 -vcodec h264 -acodec aac -f rtsp -rtsp_transport udp rtsp://192.168.0.123/live/test

ffplay 拉流
ffplay  -fflags nobuffer -f rtsp -i  rtsp://192.168.0.123/live/test

文本协议就是用 ASII 码来表达一些逻辑，或者数据结构，文本协议通常需要经过 parse 的过程，什么是 parse？json 就是一种文本表达方式，把 json 转成 C 语言的结构体，这就是 parse。特别是复杂的数据，parse 的过程会有点慢。

二进制协议 就是 直接传输 原始的数据，也就是比特流，例如 C 语言变量 int a = 0x11223344， 通过 udp 传输 这个 int 变量，4个字节，直接 send 4个字节的数据就行，对面接受 4个字节，直接把 4个字节强制转成 int 类型就能直接用。这里不需要经过 parse，因为内存数据可以直接用，网络传输传的就是内存数据。不过这是多字节传输，通常需要处理大小端问题，不过大端机器已经很少，有一些服务器不搞大端，例如SRS。