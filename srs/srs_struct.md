```plantuml
class SrsServer{
    - std::vector<**SrsListener**> listeners;
    - SrsResourceManager* conn_manager;
}
```
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

```plantuml 
ISrsFastTimer <|-- SrsHybridServer
ISrsHybridServer *-- SrsHybridServer
ISrsHybridServer <|-- SrsServerAdapter
ISrsHybridServer <|-- RtcServerAdapter

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
```

#### srs中的协程启动
SrsSTCoroutine::start --> impl_::start -->SrsFastCoroutine::start-->SrsFastCoroutine::pfn --> SrsFastCoroutine::cycle --> handler::cycle 
所以 SrsSTCoroutine启动时，最终启动的是传入handle的cycle()函数。

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
- 有连接时调用SrsBufferListener::on_tcp_client --> SrsServer::accept_client -->根据type得到对应的ISrsStartableConneciton, SrsRtmpConn::start()开启SrsRtmpConn中的协程SrsRtmpConn::cycle()

```plantuml 

SrsListener <|-- SrsBufferListener
ISrsTcpHandler <|-- SrsBufferListener
ISrsTcpHandler *-- SrsTcpListener
SrsTcpListener  *-- SrsBufferListener

abstract SrsListener{
    - (abstract) virtual srs_error_t **listen**(std::string i, int p);
    # std::string ip;
    # int port;
    # SrsServer* **server**;
}

interface ISrsTcpHandler{
    srs_error_t on_tcp_client(srs_netfd_t stfd);
}

class SrsBufferListener{
    各种listen类型封装，如SrsListenerRtmpStream
    - SrsTcpListener* listener;
}

class SrsTcpListener{
    - SrsCoroutine* trd;
    -ISrsTcpHandler* handler
    + srs_error_t listen();
    + virtual srs_error_t cycle();

}

```

#### 推流
srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)
取一个SrsLiveSource 来进行推拉流，根据type类型选择推流还是拉流 。SrsRtmpConnFMLEPublish：ffmpeg类型推流。``` srs_error_t SrsRtmpConn::publishing(SrsLiveSource* source)```
 
```plantuml
ISrsStartableConneciton <|-- SrsRtmpConn

interface ISrsStartableConneciton{

}

class SrsRtmpConn{
    - SrsServer* server;
    - SrsRtmpServer* rtmp;

}

class SrsLiveSource{
    - std::vector<SrsLiveConsumer*> consumers;
}
```
