
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
    - std::vector<**SrsListener***> listeners;
    - SrsResourceManager* conn_manager; // 每个rtmp连接 SrsRtmpConn
}
```

#### srs中的协程启动
SrsSTCoroutine::start --> impl_::start -->SrsFastCoroutine::start-->SrsFastCoroutine::pfn --> SrsFastCoroutine::cycle --> handler::cycle 
所以 SrsSTCoroutine启动时，最终启动的是传入handle的cycle()函数。

使用imp主要是防止修改了SrsFastCoroutine后导致需要编译很多文件。


srs协程的设计与Listen类差不多，

1. SrsRecvThread, SrsServer, SrsRtmpConn里都有
   协程指针，
2. 协程里传入ISrsCoroutineHandler来标识包含协程指针的类。通过ISrsCoroutineHandler来执行SrsRecvThread，SrsServer，SrsRtmpConn中的cycle()函数。


```plantuml 
ISrsStartable <|-- SrsCoroutine
SrsCoroutine <|-- SrsSTCoroutine
SrsFastCoroutine  *-- SrsSTCoroutine
ISrsCoroutineHandler *-- SrsFastCoroutine
ISrsCoroutineHandler <|-- SrsRecvThread
ISrsCoroutineHandler <|-- SrsServer
ISrsCoroutineHandler <|-- SrsRtmpConn

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
    SrsCoroutine* trd;
}

class SrsServer{
    SrsCoroutine* trd;
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

SrsListener <|-- SrsHttpFlvListener
SrsListener <|-- SrsUdpStreamListener
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

class SrsUdpListener{

}

```

Listen类的设计：
    1. SrsServer 可能有许多监听。用一个vector维护.
   ```
    // All listners, listener manager.
    std::vector<SrsListener*> listeners;
   ```
   所以这个listen监听类设计成 基类。
   SrsHttpFlvListener, SrsUdpStreamListener,SrsBufferListener,是其的子类。

   2. 这些子类内部使用的tcp或udp处理连接有共同的地方，SrsTcpListener和SrsUdpListener，这两个类来真正干活。

    3. listen监听子类与 SrsTcpListener或SrsUdpListener产生联系：
       1. listen监听子类里含有SrsTcpListener的指针。
       2. listen监听子类又继承统一的接口，SrsTcpListener成员中含有这个接口指针，在有连接时，调用该接口函数。

``` plantuml

ISrsStartableConnection <|-- SrsHttpApi
ISrsStartableConnection <|-- SrsRtmpConn
ISrsStartableConnection <|-- SrsResourceManager

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

class SrsResourceManager
{
    SrsCoroutine* trd;
    std::vector<ISrsResource*> conns_;  //The connections without any id.
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

连接管理：
    来一个请求，根据类型生成一个ISrsStartableConnection放到SrsServer中的conn_manager中。

    在连接接收后从中移除

#### SrsRtmpConn 连接协程cycle

```plantuml


ISrsProtocolReader <|-- ISrsProtocolReadWriter
ISrsProtocolWriter <|-- ISrsProtocolReadWriter
ISrsProtocolReadWriter <|-- SrsSslConnection

ISrsProtocolReadWriter *-- SrsRtmpServer
ISrsProtocolReadWriter *-- SrsProtocol
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
    
    SrsFastStream* in_buffer;  // 用于存放skt接收的数据
}

class SrsRtmpConn
{
    SrsTcpConnection* skt;
    SrsRtmpServer* rtmp;
    SrsServer* server;
}

```

一个SrsRtmpConn中有一个SrsRtmpServer，用于与客户端通信，SrsRtmpServer::SrsProtocol主要是rtmp协议相关，ISrsProtocolReadWrite主要是io相关发送接收数据

SrsRtmpConn::cycle()
    SrsRtmpConn:do_cycle()

    SrsRtmpConn::SrsRtmpServer成员主要来进行rtmp协商
        rtmp->handshake()   // 3次握手
        握手时主要是通过SrsTcpConnection中的SrsStSocket来进行读取tcp数据，握手时没有用的SrsProtocol.

        read_fully(): 读1537个字节, c0c1
        io->write(): 发送s0s1s2
    

        代码跑到这里的时候，客户端已经开始发 connect 指令，如下图抓包：connect_app() 函数做的事情就是 把 客户端的 connect 请求的信息提取出来，放到 req 变量
        SrsRequest* req = info->req;
        if ((err = rtmp->connect_app(req)) != srs_success) {
            return srs_error_wrap(err, "rtmp connect tcUrl");
        }

        SrsRtmpConn::service_cycle()
        前面的一堆逻辑，都是处理 RTMP 协议的交互协商的，就是 window size，chunk size，bandwidth，之类的

        err = stream_service_cycle();
        这个函数是处理 RTMP 推流，跟 播放两个业务的



#### Srs中的超时处理
- 有新连接时，SrsRtmpConn来接收客户端握手消息时
  ```
    SrsRtmpConn::do_cycle()
        rtmp->set_recv_timeout(SRS_CONSTS_RTMP_TIMEOUT);
        rtmp->set_send_timeout(SRS_CONSTS_RTMP_TIMEOUT);

    调用io->read_fully，来读取固定的长度大小的tcp数据，利用poll的超时机制来处理
    
    普通read也是，利用poll的超时
  ```

- 在推流收数据包时，SrsRtmpConn线程有设置一段时间后来统计收到的包是否增加。

#### 推流
srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)
取一个SrsLiveSource 来进行推拉流，

```
// Global singleton instance.
extern SrsLiveSourceManager* _srs_sources; 居然是全局变量。
 // std::map<std::string, SrsLiveSource*> pool;

 SrsLiveSource里有一个 SrsLiveConsumer列表，有Gop
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

    SrsRtmpJitter* jitter;  // 抖动算法

    srs_cond_t mw_wait;  // 条件变量
}

class SrsLiveSourceManager
{
    // The source manager to create and refresh all stream sources.
}

```

根据type类型选择推流还是拉流 。SrsRtmpConnFMLEPublish：ffmpeg类型推流。``` srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)```

rtmp协商完成后，会开启一个协程进行收发数据 ```SrsRecvThread::cycle()```
同时协商协程```SrsRtmpConn::do_publishing```会记录一些数据, 如果一段时间没有包会断开连接。
 

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
    std::vector<SrsCommonMessage*> queue;  // 拉流时收到客户端的消息
    SrsRecvThread trd;
}

```

SrsPublishRecvThread和SrsQueueRecvThread里都有
SrsRecvThread，它们都继承自ISrsMessagePumper。两个协程收到包时相应的处理不同。通过ISrsMessagePumper来处理。



```
在客户端推流时，SrsRecvThread用于接收客户端的数据。
主要的就是收到一个包，把包放到每个SrsLiveSource中的SrsLiveConsume里的queue里。

SrsRecvThread::cycle()
    SrsRecvThread::do_cycle()

        rtmp->recv_message(&msg) //处理收到的消息
        pumper->consume(msg); //数据读取出来后去消费这个数据

        SrsCommonMessage* msg

            SrsRtmpConn::handle_publish_message
                SrsRtmpConn::process_publish_message
                    SrsLiveSource::on_video_imp
                    
                    SrsCommonMessage* msg -> SrsSharedPtrMessage msg

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

class SrsCommonMessage
{
    SrsMessageHeader header; // 报文头
    int size;
    char* payload;  // 数据
}

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

class SrsFastStream
{
    char* p;   // ptr to the current read position. 已经读取的消息的指针头
    char* end;  // ptr to the content end.
    char* buffer;    // ptr to the buffer.
    int nb_buffer;   // the size of buffer.
}


```


- rtmp握手期间，直接解析c0c1c2, s0s1s2
```
SrsProtocol::recv_message
    SrsProtocol::recv_interlaced_message
    通过SrsProtocol来生成SrsCommonMessage.

```
- 客户端推流，通过SrsProtocol将收到数据封装成 SrsCommonMessage
- 调用SrsSharedPtrMessage::create(),从SrsCommonMessage里生成SrsSharedPtrMessage
  - create完成后，会将SrsCommonMessage里的指针置NULL，防止二次释放。
- consumer->enqueue，将SrsSharedPtrMessage放入到消费者的SrsMessageQueue里
- 拉流时，SrsLiveConsumer::dump_packets -> queue->dump_packets, 将SrsMessageQueue里的SrsSharedPtrMessage取出来，全部取出来。放到SrsMessageArray里。
- rtmp->send_and_free_messages。将SrsSharedPtrMessage发送给客户端，同时将SrsSharedPtrMessage释放。

读数据主要通过SrsFastStream类来进行读取数据。
使用协程可以方便的处理一次read时数据不够的问题。

最后是通过memcpy将SrsFastStream里的buffer copy到
SrsCommonMessage中的。

#### 拉流

```
在拉流过程中，也是新起一个协程来接收客户端的一些通信信息。

SrsRtmpConn::cycle()协程
    SrsRtmpConn::service_cycle
        stream_service_cycle();
            playing(source), 这个source也是从_srs_sources取的，如果这时候没有这个流呢。
                : 创建SrsLiveConsumer, source->create_consumer(consumer)
                : SrsLiveSource::consumer_dumps, gop_cache->dump, 将gop中的缓存全部给client

                :SrsRtmpConn::do_playing, 主协程统计一些信息,并且发送给客户端
                    : consumer->dump_packets
                    : rtmp->send_and_free_messages

                :新起一个协程来接收客户端的消息。

```


#### 线程同步
在 Linux 系统使用 多线程的时候，线程间通信，可以使用 条件变量 以及 互斥锁。例如 线程 A 是生产者，不断写入任务到队列，线程 B 是消费者，不断从队列读取任务，没有任务的时候，线程B会阻塞，等待 线程A通知。这个通知就需要用到 条件变量 以及 互斥锁。

```

#include <stdio.h>
#include <memory.h>
#include "st.h"
#include "../common.h"
_st_cond_t *cond_name;
_st_mutex_t* mutex_name;
char name[100] = {};
​
void *publish(void *arg) {
    st_usleep(2 * 1000000LL);
    st_cond_signal(cond_name);   // 唤醒消费者
​
    strcpy(name,"Loken");
    st_utime_t time_now = st_utime();
    printf("Pulish name %lld\r\n", time_now);
​
    return NULL;
}

void *consume(void *arg) {
    st_mutex_lock(mutex_name);
    st_cond_wait(cond_name);    // 等待唤醒
    st_mutex_unlock(mutex_name);
​
    st_utime_t time_now = st_utime();
    printf("Consume name %s , %lld\r\n",name, time_now);
    return NULL;
}

int main(int argc, char *argv[]) {
    cond_name = st_cond_new();  // 生成cond
    mutex_name = st_mutex_new();  // 生成mutex
    st_init();
​
    st_utime_t time_now = st_utime();
    printf("start %lld\r\n", time_now);
​
    st_thread_create(publish, NULL, 0, 0);
    st_thread_create(consume, NULL, 0, 0);
    st_thread_create(consume, NULL, 0, 0);
    st_thread_create(consume, NULL, 0, 0);
​
    st_thread_exit(NULL);
​
    /* NOTREACHED */
    return 1;
}
​
```

每个SrsLiveConsumer中含有一个条件变量```mw_wait = srs_cond_new();```

consumer->wait(mw_msgs, mw_sleep); 
    srs_cond_wait(mw_wait);
等待生产者唤醒。

生产者队列中有数据时```srs_cond_signal(mw_wait);```


