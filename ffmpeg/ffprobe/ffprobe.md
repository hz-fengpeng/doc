```
'ffprobe解析文件过程
'probe_file
    'open_input_file
        'avformat_open_input // ffmpeg打开媒体函数，AVInputFormat，AVStream中的参数
            'init_input // 找到AVInputFormat, 解封装demuxer，输入的容器格式
                'av_probe_input_format2 (is_opened=0)//不打开文件时probe, 主要是rtp，rtsp这类可以根据文件名得到的format
                'AVFormatContext::io_open(io_open_default)   // 读取一段数据进行probe, 其实和avio_open2函数功能相同，主要是生成AVIOContext
                    'ffurl_open_whitelist // 找到URLContext
                        'ffurl_alloc // 根据文件名找到URLProtocol，即读取文件协议(如:ff_file_protocol)，然后利用URLProtocol生成URLContext
                        'ffurl_connect // 打开文件，调用URLProtocol中的url_open或url_open2,如fopen
                    'ffio_fdopen // 根据URLContext生成AVIOContext, (avio_alloc_context)
                'av_probe_input_buffer2 // 调用avio_read读取文件到buffer，进行文件格式判断 （ff_wav_demuxer)
            'read_header // 读头文件，此时会在AVFormatContext中生成相应的AVStream, 并赋值给AVStream中的AVCodecParameters，包含解码器的参数;
        
        'avformat_find_stream_info  // read_header中得到的流信息可能不全，AVSampleFormat需要解码得到。得到流信息，赋值给AVCodecParameters
            '1.avcodec_parameters_to_context // 将读文件头中的AVStream的AVCodecParameters赋值给AVCodecContext
            '2.find_probe_decoder // 根据codec_id(AVCodecID)找到对应的解码器,AVCodec, 如（ff_pcm_s16le_decoder)
            '3. avcodec_open2 //打开解码器，调用AVcodec的init函数(如pcm_decode_init) 主要是对AVCodecContext进行赋值
            'read_frame_internal // 读取一帧压缩数据  AVPacket, 调用AVInputFormat中的read_packet读取一帧
                '可能会把packet放到AVFormatInternal中的packet_buffer中，是一个PacketList。
                ' 和av_read_frame函数功能类似。
            'try_decode_frame // 尝试解码一帧数据   AVFrame， 根据has_codec_parameters来判断是否参数全了 
                ' 为什么解码就能得到更多的数据。
                'avcodec_send_packet
                    'decode_receive_frame_internal
                        'decode_simple_receive_frame // 调用编解码器的decode函数进行解码
                'avcodec_receive_frame
            'avcodec_close // 关闭解码器 
            '主要是通过读取数据进行编解码来进一步赋值AVCodecParameters
        'av_dump_format // 打印流信息
    'show
    'close_input_file
        'avcodec_free_context   // 关闭解码器
        'avformat_close_input   
```


```

#0  udp_write (h=0x555557a6ea40, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>, size=28) at libavformat/udp.c:1020
#1  0x0000555555a8d08b in retry_transfer_wrapper (h=0x555557a6ea40, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>,
    size=28, size_min=28, transfer_func=0x555555c1b479 <udp_write>) at libavformat/avio.c:370
#2  0x0000555555a8d27c in ffurl_write (h=0x555557a6ea40, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>, size=28)
    at libavformat/avio.c:423
#3  0x0000555555bddd04 in rtp_write (h=0x555557a571c0, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>, size=28)
    at libavformat/rtpproto.c:496
#4  0x0000555555a8d08b in retry_transfer_wrapper (h=0x555557a571c0, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>,
    size=28, size_min=28, transfer_func=0x555555bdd7a1 <rtp_write>) at libavformat/avio.c:370
#5  0x0000555555a8d27c in ffurl_write (h=0x555557a571c0, buf=0x555557ab9fc0 "\200", <incomplete sequence \310>, size=28)
    at libavformat/avio.c:423
#6  0x0000555555a8e0d7 in writeout (s=0x555557ab9500, data=0x555557ab9fc0 "\200", <incomplete sequence \310>, len=28)
    at libavformat/aviobuf.c:160
#7  0x0000555555a8e22b in flush_buffer (s=0x555557ab9500) at libavformat/aviobuf.c:181
#8  0x0000555555a8e508 in avio_flush (s=0x555557ab9500) at libavformat/aviobuf.c:238
#9  0x0000555555bd6397 in rtcp_send_sr (s1=0x555557abef40, ntp_time=3857979849041000, bye=0) at libavformat/rtpenc.c:327
#10 0x0000555555bd6cfc in rtp_write_packet (s1=0x555557abef40, pkt=0x7fffffffd8c0) at libavformat/rtpenc.c:531
#11 0x0000555555b8126f in write_packet (s=0x555557abef40, pkt=0x7fffffffd8c0) at libavformat/mux.c:712
#12 0x0000555555b8257b in interleaved_write_packet (s=0x555557abef40, pkt=0x0, flush=0) at libavformat/mux.c:1087
#13 0x0000555555b82754 in write_packet_common (s=0x555557abef40, st=0x555557ad7c00, pkt=0x555557af0800, interleaved=1)
    at libavformat/mux.c:1114
#14 0x0000555555b82a46 in write_packets_common (s=0x555557abef40, pkt=0x555557af0800, interleaved=1) at libavformat/mux.c:1171
#15 0x0000555555b82bf9 in av_interleaved_write_frame (s=0x555557abef40, pkt=0x555557af0800) at libavformat/mux.c:1227
#16 0x000055555566baf2 in write_packet (of=0x555557a57040, pkt=0x555557af0800, ost=0x555557ab87c0, unqueue=0)
    at fftools/ffmpeg.c:866
#17 0x000055555566bcec in output_packet (of=0x555557a57040, pkt=0x555557af0800, ost=0x555557ab87c0, eof=0) at fftools/ffmpeg.c:912
#18 0x00005555556712e6 in do_streamcopy (ist=0x555557abc580, ost=0x555557ab87c0, pkt=0x555557abc4c0) at fftools/ffmpeg.c:2129
#19 0x0000555555673ebd in process_input_packet (ist=0x555557abc580, pkt=0x555557abc4c0, no_eof=0) at fftools/ffmpeg.c:2809
#20 0x000055555567b4fa in process_input (file_index=0) at fftools/ffmpeg.c:4636
#21 0x000055555567b9ee in transcode_step () at fftools/ffmpeg.c:4776
#22 0x000055555567bb56 in transcode () at fftools/ffmpeg.c:4830
#23 0x000055555567c564 in main (argc=12, argv=0x7fffffffe3f8) at fftools/ffmpeg.c:5035


```

mp4是ff_mov_demuxer
我们从flv和mp4等文件解封装读取的AVPacket并没有SPS、PPS数据，而是保存在 AVFormatContext -> streams -> codecpar -> extradata里。

在解码过程中，需要设置SPS/PPS等解码信息，才能够初始化解码器。
有两种方式可以设置SPS/PPS，
一种是手动指定SPS/PPS内容，指定AVCodecContext结构体中extradata的值；
    memcpy(pFormatContext->streams[0]->codecpar->extradata, szSPSPPS, sizeof(szSPSPPS));
    通过avcodec_parameters_to_context将信息从pFormatContext->streams[0]->codecpar拷贝到m_pAVCodecContext
avcodec_open2函数在调用的时候，会解析extradata数据的内
h264_decode_init(AVCodecContext * avctx) 然后在函数体中，开始解析
ff_h264_decode_seq_parameter_set.

```
#0  h264_decode_init (avctx=0x5555573c2b40) at libavcodec/h264dec.c:366
#1  0x0000555555888adc in avcodec_open2 (avctx=0x5555573c2b40, codec=0x555556cd1c80 <ff_h264_decoder>, options=0x0)
    at libavcodec/avcodec.c:323
#2  0x000055555562fa1e in main () at /home/fengpeng/ffmpegUser/decode.c:44

```

一种是让FFmpeg通过读取传输数据来分析SPS/PPS信息，一般情况下在每一个I帧之前都会发送一个SPS帧和PPS帧



