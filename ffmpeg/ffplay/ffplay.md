
1. flush_pkt 变量初始化，flush_pkt 是一个 AVPacket，它的data不是NULL，所以 flush_pkt 不是用来刷解码器。那既然这个变量叫flush_pkt，它是flush，用来刷什么的呢？解答：flush_pkt 是用来丢弃缓存的packet跟frame的，ffplay的播放模式不是每读一帧数据就播放一帧，而是ffplay开了2个线程分别处理读取数据跟播放数据，读取线程会维护两个队列，packet队列跟frame队列，读取线程不断往这两个队列堆数据，直到达到阈值。播放队列不断从frame队列拿数据出来传给SDL播放。读取线程一般来说是比播放线程要快的，因为视频的帧率是1秒播放24帧，但是读取线程，1秒可以大概解码出100多帧吧。假如播放队列线程播放到 00:00:04 秒，但是读取线程已经缓存了 00:00:04 ~ 00:00:14 秒的frame跟packet，如果这在这个瞬间，你拖动播放进度条，拖到 00:00:20 秒的地方播放，那之前缓存队列的00:00:04 ~ 00:00:14 秒的frame跟packet 是不是就没用了，所以拖动进度条就会产生一个 flush_pkt 丢进去 队列里面，然后导致 PacketQueue::serial + 1，队列里面位于 flush_pkt 之前的pakcet的serial值跟位于 flush_pkt 之前的pakcet的serial值 是不一样的，ffplay 就是根据 serial 决定丢不丢弃 packet跟frame。

2. 调用 stream_open() 开启多线程处理，这里先简单介绍一下有多少个线程。
packet读取线程 read_thread()，不断从文件读取pakcet，然后根据packet类型，传递到 音频pakcet队列，视频packet队列或字幕packet队列。
音频解码线程 audio_thread()，不断从音频pakcet队列拿到 packet，然后解码出frame，插进去 音频frame队列。
视频解码线程 video_thread()，不断从视频pakcet队列拿到 packet，然后解码出frame，插进去 视频frame队列。
字幕解码线程 subtitle_thread()，不断从字幕pakcet队列拿到 packet，然后解码出frame，插进去 字幕frame队列。

3. stream_component_open, 开启解码线程


ffplay跨线程传递包：
    read_thread启动时，传入的 VideoState *is 作为线程的参数，里面有 音频packet队列,视频packet队列,字幕packet队列。
    调用packet_queue_put来向队列中放包。read_thread线程放包时会加锁
    ```
        SDL_LockMutex(q->mutex);
        ret = packet_queue_put_private(q, pkt1);
        SDL_UnlockMutex(q->mutex);
    ```

    音频解码线程，视频解码线程，字幕解码线程 会调用packet_queue_get从队列中取包
    也会加锁
    