源码解析

```
全局变量
InputStream **input_streams = NULL;
int        nb_input_streams = 0;
InputFile   **input_files   = NULL;
int        nb_input_files   = 0;

OutputStream **output_streams = NULL;
int         nb_output_streams = 0;
OutputFile   **output_files   = NULL;
int         nb_output_files   = 0;
```

1. 在解析输入参数时，parse_options(), 
   1. open_input_file()函数中
      1. avformat_open_input(): 打开文件获得AVFormatContext结构体
      2. avformat_find_stream_info(): 进一步获取streamInfo
      3. add_input_streams：主要是申请InputStream，并为其中的解码器赋值AVCodecContext, 从AVFormatContext结构体中拷贝。
   2. open_output_file()函数中
      1. avformat_alloc_output_context2()初始化AVFormatContext
      2. 分别调用new_video_stream()，new_audio_stream()，new_subtitle_stream()等创建不同的AVStream
         1. 调用avformat_new_stream函数生成新的stream
      3. 调了 avio_open2() 打开输出文件AVIOContext

2. transcode转码
   1. transcode_init(): 
      1. init_input_stream()：其中调用了avcodec_open2()打开解码器AVCodecContext
      2. init_output_stream_wrapper/init_output_stream/check_init_output_file：avformat_write_header，写文件头, AVOutputFormat中的write_header函数。
   2. transcode_step：转码步骤
      1. process_input:
         1. get_input_packet: 调用av_read_frame读取packet AVPacket
         2. process_input_packet：似乎是解码 decode_audio，decode_video
            1. send_frame_to_filters, 把解码后的帧放到filters里
      2. reap_filters 完成编码
         1. do_audio_out， do_video_out 进行编码
            1. output_packet， write_packet/ av_interleaved_write_frame将packet写进去。
               1. 最终调用s->oformat->write_packet