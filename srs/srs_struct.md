
configure文件：
    1. 在configure时会编译第3方库放到编译目录
    2. 生成对应的C head文件
    3. 生成makefile文件

makefile文件：
    有2个makefile，最后会调用objs/Makefile

**每个端口监听一个协程，每个客户端连接对应一个协程**

#### srs中的协程
- 主协程启动完后等待wg.wait()。```SrsHybridServer::run```
- fd接收连接协程 ```SrsTcpListener::cycle()```
  - 有新连接时，SrsRtmpConn协程，处理rtmp协商 ```SrsRtmpConn::cycle()```
  - 协商完成后会开启SrsRecvThread协程，数据接收 ```SrsRecvThread::cycle()```,SrsRtmpConn协程会统计接收时的一些信息。所以有新连接时会开启2个协程。
  - 在拉流时，SrsRtmpConn协程主要是向客户端发包，也会新起一个协程来处理客户端的通信信令。


SRS 服务器启动之后，会开启一个协程 （SrsTcpListener::cycle） 来 监听 1935 的RTMP端口，不断 accept 客户端请求。

2，始祖协程 利用 wg.wait() 等等其他的服务结束，其他服务是指 RTMP服务，SRT服务，RTC 服务。

3，有RTMP推流客户端请求来了，新开一个协程D（SrsRtmpConn::cycle） 来处理 请求，包括RTMP握手，传输音视频数据前的交互。处理完前期的交互工作之后，发现客户端是一个推流客户端，就会再开一个协程 E （SrsRecvThread::cycle）来处理推过来的音视频数据流。之前的协程D 会阻塞不断循环统计一些信息。

#### srs程序启动
程序启动，主要是调用SrsHybirdServer::servers中的每一个servers中的run函数，然后调用wait等待函数，使原始协程阻塞在这里。
```
#0  SrsServer::listen (this=0x5555560b7700) at src/app/srs_app_server.cpp:750
#1  0x00005555557c14cc in SrsServerAdapter::run (this=0x5555560b8e90, wg=0x7fffffffdf10) at src/app/srs_app_hybrid.cpp:160
#2  0x00005555557c201a in SrsHybridServer::run (this=0x5555560a2990) at src/app/srs_app_hybrid.cpp:275
  SrsHybridServer注册SrsServerAdapter，SrtServerAdapter，RtcServerAdapter
#3  0x0000555555829ad2 in run_hybrid_server () at src/main/srs_main_server.cpp:477
#4  0x00005555558295bd in run_directly_or_daemon () at src/main/srs_main_server.cpp:407
#5  0x0000555555827eb5 in do_main (argc=3, argv=0x7fffffffe448) at src/main/srs_main_server.cpp:198
#6  0x00005555558280f2 in main (argc=3, argv=0x7fffffffe448) at src/main/srs_main_server.cpp:207

```
SrsServerAdapter::run函数，主要调用成员函数SrsServer中的函数，启动监听协程。

SrsTcpListener::listen()时，申请一个监听fd后，会开启一个协程。trd->start(), 此时其实没有启动协程，只是将该协程放入runable队列。在最后调用wg.wait()时，main协程会释放控制权，让各个协程运行起来。


这里为什么用一个SrsServerAdapter， adapter

### 适配器模式
适配器模式（Adapter Pattern）是作为两个不兼容的接口之间的桥梁。这种类型的设计模式属于结构型模式，它结合了两个独立接口的功能。

这种模式涉及到一个单一的类，该类负责加入独立的或不兼容的接口功能。举个真实的例子，读卡器是作为内存卡和笔记本之间的适配器。您将内存卡插入读卡器，再将读卡器插入笔记本，这样就可以通过笔记本来读取内存卡

相当于SrsServer中没有ISrsHybirdServer中的接口。
生成一个SrsServerAdapter来封装这个SrsServer。

```plantuml 
ISrsFastTimer <|-- SrsHybridServer
ISrsHybridServer *-- SrsHybridServer
ISrsHybridServer <|-- SrsServerAdapter
ISrsHybridServer <|-- RtcServerAdapter

SrsServer *-- SrsServerAdapter

interface ISrsFastTimer{
    (abstract) virtual srs_error_t on_timer(srs_utime_t interval);
}

interface ISrsHybridServer{
    virtual srs_error_t initialize() = 0;
    virtual srs_error_t run(SrsWaitGroup* wg) = 0;
    virtual void stop() = 0;
}

class SrsHybridServer{
    - std::vector<**ISrsHybridServer***> servers;
    + virtual void register_server(ISrsHybridServer* svr);
    + virtual SrsServerAdapter* srs();

}

class SrsServerAdapter{
    - SrsServer* srs;  // 主要的功能实现
    + virtual SrsServer* instance();
}
class RtcServerAdapter{

}

class SrsServer{
    - std::vector<**SrsListener**> listeners;
    - SrsResourceManager* conn_manager; // 每个rtmp连接 SrsRtmpConn
}
```

#### srs中的协程启动
SrsSTCoroutine::start --> impl_::start -->SrsFastCoroutine::start-->SrsFastCoroutine::pfn --> SrsFastCoroutine::cycle --> handler::cycle 
所以 SrsSTCoroutine启动时，最终启动的是传入handle的cycle()函数。

使用imp主要是防止修改了SrsFastCoroutine后导致需要编译很多文件。

```plantuml 
ISrsStartable <|-- SrsCoroutine
SrsCoroutine <|-- SrsSTCoroutine
SrsFastCoroutine  *-- SrsSTCoroutine
ISrsCoroutineHandler *-- SrsFastCoroutine
ISrsCoroutineHandler <|-- SrsRecvThread
ISrsCoroutineHandler <|-- SrsServer

interface ISrsStartable{
   'Start the object, generally a croutine.
   {abstract} virtual srs_error_t start();
}

interface ISrsCoroutineHandler{
    (abstract) virtual srs_error_t cycle() = 0;
}

abstract SrsCoroutine{
    ' The corotine object.
    {abstract} virtual void stop();
    {abstract} virtual void interrupt();
    {abstract} virtual srs_error_t pull();
    {abstract} virtual const SrsContextId& cid();
}

class SrsSTCoroutine{
    - SrsFastCoroutine* impl_;
    + SrsSTCoroutine(std::string n, ISrsCoroutineHandler* h);
    + SrsSTCoroutine(std::string n, ISrsCoroutineHandler* h, SrsContextId cid);
    + void set_stack_size(int v);
}

class SrsFastCoroutine{
    - std::string name;
    - int stack_size;
    - ISrsCoroutineHandler* handler;
    + SrsFastCoroutine(std::string n, ISrsCoroutineHandler* h);
    + SrsFastCoroutine(std::string n, ISrsCoroutineHandler* h, SrsContextId cid);
    + void set_stack_size(int v);
    + srs_error_t start();
    + void stop();
    + void interrupt();
    + inline srs_error_t pull();
    + const SrsContextId& cid();
    - srs_error_t cycle();
    - static void* pfn(void* arg);
}

class SrsRecvThread{

}

class SrsServer{

}

```


#### SrsServer::listen_rtmp

- SrsServer::listen_rtmp时，创建SrsBufferListener,调用其listen函数。
- 创建新的SrsTcpListener，调用成员SrsTcpListener::listen函数，启动其中的协程，调用SrsTcpListener::cycle，阻塞在监听

- 其实这里协程还没开始运行，因为还没有上下文切换
-
- 有连接时SrsTcpListener::cycle()里调用handler->on_tcp_client(fd))  --- SrsBufferListener::on_tcp_client --> SrsServer::accept_client -->根据type得到对应的ISrsStartableConneciton, SrsRtmpConn::start()开启SrsRtmpConn中的协程SrsRtmpConn::cycle()

- 收到一个新连接，开始一个新的协程来处理这个连接。

此时有多个协程，原来的监听协程还在，新的协程来处理连接请求。

```plantuml 

SrsListener <|-- SrsBufferListener
ISrsTcpHandler <|-- SrsBufferListener
ISrsTcpHandler *-- SrsTcpListener
SrsTcpListener  *-- SrsBufferListener

abstract SrsListener{
    // tcp listener
    - (abstract) virtual srs_error_t **listen**(std::string i, int p);
    # std::string ip;
    # int port;
    # SrsServer* **server**;
}

interface ISrsTcpHandler{
    srs_error_t on_tcp_client(srs_netfd_t stfd);
}

class SrsBufferListener{
    // buffered TCP listener
    各种listen类型封装，如SrsListenerRtmpStream
    - SrsTcpListener* listener;
}

class SrsTcpListener{
    - SrsCoroutine* trd;        // 监听协程
    - srs_netfd_t lfd;   // 监听fd
    -ISrsTcpHandler* handler  // 其实就是SrsBufferListener
    + srs_error_t listen();
    + virtual srs_error_t cycle();

}

```

``` plantuml

ISrsStartableConnection <|-- SrsHttpApi
ISrsStartableConnection <|-- SrsRtmpConn
ISrsResource <|-- ISrsConnection
ISrsConnection <|-- ISrsStartableConnection
ISrsStartable <|-- ISrsStartableConnection
ISrsKbpsDelta <|-- ISrsStartableConnection

interface ISrsResource
{
    // The resource managed by ISrsResourceManager.

    virtual const SrsContextId& get_id() = 0;
    virtual std::string desc() = 0;


}

interface ISrsConnection
{
    // The connection interface for all HTTP/RTMP/RTSP object.
    virtual std::string remote_ip() = 0;
}

interface ISrsStartable
{
    virtual srs_error_t start() = 0;
}

interface ISrsStartableConnection
{
    //  Interface for connection that is startable.

}

interface ISrsKbpsDelta
{
    virtual void remark(int64_t* in, int64_t* out) = 0;
}

class SrsRtmpConn
{
    SrsRtmpServer* rtmp;
    SrsServer* server;

    SrsRtmpConn::handle_publish_message()

}

class SrsHttpApi
{

}

```

```
// OK, we start SRS server.
    wg_ = wg;
    wg->add(1);
```
wg.wait()，真正开始切换上下文，让之前创建的协程全部跑起来，我猜测就是在这里做的。

代码运行到 wg.wait() 的时候，之前的协程已经开始跑起来


#### SrsRtmpConn 连接协程cycle

```plantuml


ISrsProtocolReader <|-- ISrsProtocolReadWriter
ISrsProtocolWriter <|-- ISrsProtocolReadWriter

ISrsProtocolReadWriter *-- SrsRtmpServer
ISrsProtocolReadWriter <|-- SrsTcpConnection
ISrsProtocolReadWriter <|-- SrsStSocket
SrsProtocol *-- SrsRtmpServer
SrsStSocket *-- SrsTcpConnection

interface ISrsProtocolReader
{

}

interface ISrsProtocolWriter
{

}

interface ISrsProtocolReadWriter
{

}

class SrsRtmpServer
{
    ISrsProtocolReadWriter* io;  // 就是SrsTcpConnection
    SrsProtocol* protocol;  
    SrsHandshakeBytes* hs_bytes;  // rtmp握手时数据

    handshake()
}

class SrsTcpConnection
{
    // 连接
    srs_netfd_t stfd;   // 传入的fd
    SrsStSocket* skt;   // 用于从socket中读数据
}

class SrsStSocket
{

}


class SrsProtocol
{
    // 用于解析rtmp协议，和发送
    ISrsProtocolReadWriter* skt;  // 就是SrsTcpConnection
}

```
SrsRtmpConn::cycle()
    SrsRtmpConn:do_cycle()

    SrsRtmpConn::SrsRtmpServer成员主要来进行rtmp协商
        rtmp->handshake()   // 3次握手
        握手时主要是通过SrsTcpConnection中的SrsStSocket来进行读取tcp数据。
    

        代码跑到这里的时候，客户端已经开始发 connect 指令，如下图抓包：connect_app() 函数做的事情就是 把 客户端的 connect 请求的信息提取出来，放到 req 变量
        SrsRequest* req = info->req;
        if ((err = rtmp->connect_app(req)) != srs_success) {
            return srs_error_wrap(err, "rtmp connect tcUrl");
        }

        SrsRtmpConn::service_cycle()
        前面的一堆逻辑，都是处理 RTMP 协议的交互协商的，就是 window size，chunk size，bandwidth，之类的

        err = stream_service_cycle();
        这个函数是处理 RTMP 推流，跟 播放两个业务的




#### 推流
srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)
取一个SrsLiveSource 来进行推拉流，

```
// Global singleton instance.
extern SrsLiveSourceManager* _srs_sources;
```

```plantuml
class SrsLiveSource
{
    // The live streaming source.
    - std::vector<SrsLiveConsumer*> consumers;
    -  SrsGopCache* gop_cache;

}

class SrsLiveConsumer
{
    // The consumer for SrsLiveSource, that is a play client.

    SrsMessageQueue* queue;  // 包的队列
    SrsLiveSource* source;
}

class SrsLiveSourceManager
{
    // The source manager to create and refresh all stream sources.
}

```

根据type类型选择推流还是拉流 。SrsRtmpConnFMLEPublish：ffmpeg类型推流。``` srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)```

rtmp协商完成后，会开启一个协程进行收发数据 ```SrsRecvThread::cycle()```
同时协商协程会记录一些数据。
 

```plantuml
SrsRecvThread <|-- SrsPublishRecvThread
SrsRecvThread <|-- SrsQueueRecvThread

class SrsPublishRecvThread
{
    SrsRecvThread trd;
}

class SrsRecvThread
{
    SrsCoroutine* trd;
    ISrsMessagePumper* pumper;  // 其实就是SrsPublishRecvThread, \n或者是SrsQueueRecvThread
}

class SrsQueueRecvThread
{
    std::vector<SrsCommonMessage*> queue;
    SrsRecvThread trd;
}

```
```
SrsRecvThread::cycle()
    SrsRecvThread::do_cycle()

        rtmp->recv_message(&msg) //处理收到的消息
        pumper->consume(msg); //数据读取出来后去消费这个数据
            SrsRtmpConn::handle_publish_message
                SrsRtmpConn::process_publish_message
                    SrsLiveSource::on_video_imp
                    
                    for (int i = 0; i < (int)consumers.size(); i++) {
                        SrsLiveConsumer* consumer = consumers.at(i);
                        if ((err = consumer->enqueue(msg, atc, jitter_algorithm)) != srs_success) {
                            return srs_error_wrap(err, "consume video");
                        }
                    }

                SrsLiveConsumer::enqueue
                SrsMessageQueue::enqueue

                SrsGopCache::cache

```


### srs中的消息队列

```plantuml

SrsSharedPtrPayload *-- SrsSharedPtrMessage

class SrsSharedPtrPayload
{
    // 这个是共享的
    SrsSharedMessageHeader header;
    char* payload;
    int size;
    int shared_count; // 计数
}

class SrsSharedPtrMessage
{
    int size;
    char* payload;
    SrsSharedPtrPayload* ptr;

    SrsSharedPtrMessage::copy()
}

class SrsMessageQueue
{
    SrsFastVector msgs;
    或者std::vector<SrsSharedPtrMessage*> msgs

}

class SrsGopCache
{
    // cache a gop of video/audio data,
    // cached gop.
    std::vector<SrsSharedPtrMessage*> gop_cache;

    SrsGopCache::cache(SrsSharedPtrMessage* shared_msg)
}


```

SrsRtmpConn::playing
source->consumer_dumps(consumer)

#### 拉流

```
SrsRtmpConn::cycle()协程
    SrsRtmpConn::service_cycle
        stream_service_cycle();
            playing(source)
                : 创建SrsLiveConsumer
                : SrsLiveSource::consumer_dumps, gop_cache->dump, 将gop中的缓存全部给client

                :SrsRtmpConn::do_playing, 主协程统计一些信息,并且发送给客户端

                :新起一个协程来接收客户端的消息。




```