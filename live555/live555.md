Live555是一个实现了RTSP协议的开源流媒体框架，Live555包含RTSP服务器端的实现以及RTSP客户端的实现。Live555可以将若干种格式的视频文件或者音频文件转换成视频流或者音频流在网络中通过RTSP协议分发传播，这便是流媒体服务器最核心的功能

经过Live555流化后的视频流或者音频流可以通过实现了标准RTSP协议的播放器（如VLC）来播放

### 基本框架
- UsageEnvironment目录，BasicUsageEnvironment目录
    系统环境的抽象，主要用于消息的输入输出和用户交互功能，包含抽象类UsageEnviroment，TaskScheduler，DelayQueue，HashTable。
    
    UsageEnvironment代表了整个系统运行的环境，它提供了错误记录和错误报告的功能，无论哪一个类要输出错误，都需要保存UsageEnvironment的指针

    TaskScheduler: 实现事件的异步处理、事件处理函数的注册等，它通过维护一个异步读取源实现对诸如通信消息到达等事件的处理，通过DelayQueue实现对其他注册事件的延时调度。TaskScheduler由于全局中只有一个，所以保存在UsageEnvironment中，而所有的类又都保存了UsageEnvironment的指针，所以谁想把自己的任务加入调度中，那是很容易的。

    TaskScheduler中实现的三类调度任务：
        socket任务（HandlerSet类管理）
        延迟任务（DelayQueue类管理）
        Event任务；
        第一个及第三个在加入执行队列后会一直存在，而delay任务在执行完一次后会立即丢掉。

    HashTable类：实现了哈希表。
    HandleSet类：是一种专门用于执行socket操作的任务（函数），被TaskScheduler用来管理所有的socket任务（增删改查）。在整个系统中都可以用到它。

    DelayQueue类：延迟队列，他就是在TaskScheduler中用于管理调度任务的东西。他是一个队列，队列中每一项代表了一个要调度的任务（在fToken变量中保存）。该类同时保存了这个任务离执行时间点的剩余时间。

    BasicUsageEnvironment是UsageEnvoronment的一个控制台的实现。它针对控制台的输入输出和信号响应进行具体实现。

- groupsock目录
    主要被设计用于支持多播，仅仅支持UDP协议。理论上讲那些需要操作udp socket的类应保存GroupSocket的实例，但事实上并不是，RTPSink,RTPSource,RTCPInstance等，他们都没有保存GroupSocket型的变量。 他们是通过RTPInterface类来进行socket操作的！

- liveMedia目录
    该模块是Live555最重要的模块，该模块声明了一个抽象类Medium，其它所有类都派生自该类。
    liveMedia主要运转的是一个source和sink的循环。
       source：数据来源的抽象，比如通过RTP读取数据；
       sink：数据消费者的抽象。比如吧接收到的数据存储到文件，该文件就是一个sink。
       source和sink通过RTP子会话(MediaSubSession)联系在一起， 数据的流通可能会经过多个Source和Sink。。
    源码文件名以sink结尾的就是sink，像MediaSink.cpp，以source结尾的就是source,如MediaSource.cpp。

    TCP传输方式，读取数据发送时注意TCPStreamSink::processBuffer()这个函数。
    MediaSink是各种类型的Sink的基类，MediaSource是各种类型Source的基类。各种类型的流媒体格式和编码的支持就是通过对这两个类的派生实现的。

    该模块的几个主要类：

    RTSPClient：该类实现RTSP请求的发送和响应的解析，同时根据解析的结果创建对应的RTP会话。
    RTSPClientSession：每个该类的实例代表每个客户端的rtsp会话，保存了每个客户的socket。
    MediaSession：用于表示一个RTP会话，一个MediaSession可能包含多个子会话（MediaSubSession），子会话可以是视频子会话、音频子会话等。
    RTCPInstance: 此类实现RTCP协议的通信
    RtspServer：服务器实例，一旦创建就永久存在，其余对象都是由他创建或由它引起它们的创建。直接掌管ServerMediaSession和RTSPClient，只管这两个类的实例的创建不负责销毁。
    ServerMediaSession：对应一个媒体文件，ServerMediaSession在客户端发出对一个新文件的DESCRIBE时创建。建立ServerMediaSession的同时也创建了ServerMediaSubsession，被ServerMediaSession所管理，代表一个文件中的track。
    SercverMediaSubSession：代表的是一个静态的流（或者说tack），也就是可以从它里面获取一个流的各种信息，但是不能获取流的传输状态。
    如果添加多个ServerMediaSubsession那么ServerMediaSession与流名字所指定的文件是没有关系的，也就是说ServerMediaSession不会操作文件，文件的操作是放在ServerMediaSubSession中的。

```plantuml

TaskScheduler *-- UsageEnvironment
UsageEnvironment <|-- BasicUsageEnvironment0 
BasicUsageEnvironment0 <|-- BasicUsageEnvironment

class UsageEnvironment 
{
    - TaskScheduler& fScheduler;
    void* liveMediaPriv; //可以是env的全局hash表
}

class BasicUsageEnvironment0
{

}

class BasicUsageEnvironment
{

}

class TaskScheduler
{

}

TaskScheduler <|-- BasicTaskScheduler0
BasicTaskScheduler0 <|-- BasicTaskScheduler


class BasicTaskScheduler0
{
    virtual TaskToken scheduleDelayedTask(int64_t microseconds, TaskFunc* proc,
				void* clientData); //设置延迟事件

    virtual void triggerEvent(EventTriggerId eventTriggerId, void* clientData = NULL); //设置触发事件

    EventTriggerId volatile fTriggersAwaitingHandling; // implemented as a 32-bit bitmap ,触发事件表示

     
  DelayQueue fDelayQueue;// To implement delayed operations:

 
  HandlerSet* fHandlers; // IO事件，双向循环链表
}

class BasicTaskScheduler
{
    virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData); //设置IO事件，将其加入到fHandlers

    fd_set fReadSet;  // select()中的readset
    fd_set fWriteSet;
    fd_set fExceptionSet;
}

```
#### 程序运行select
程序执行env->taskScheduler().doEventLoop()后就一直在里面不停的执行，既是进入消息循环。即是一个select消息驱动
    1. 首先处理IO操作
    2. 处理触发事件
    3. 处理延迟任务


```plantuml

title IO

class HandlerDescriptor   
{
    int socketNum;
    int conditionSet;  // 是否可读可写
    TaskScheduler::BackgroundHandlerProc* handlerProc;
    void* clientData;

    HandlerDescriptor* fNextHandler;
    HandlerDescriptor* fPrevHandler;
}

class HandlerSet
{
     HandlerDescriptor fHandlers;  // handler链表头节点
}

class HandlerIterator
{
    //HandlerSet迭代器
}

```
BasicTaskScheduler0::fHandlers为HandlerSet*，每当调用setBackgroundHandling时，生成新的HandleDescriptor节点，加入到IO双向链表中

```plantuml
title delay

DelayQueueEntry <|-- DelayQueue
DelayQueueEntry <|-- AlarmHandler

class DelayQueueEntry
{
    DelayQueueEntry* fNext;
    DelayQueueEntry* fPrev;
    DelayInterval fDeltaTimeRemaining;
}

class DelayQueue
{
    _EventTime fLastSyncTime;
}

class AlarmHandler
{
    TaskFunc* fProc;  // handle
    void* fClientData;  // 数据

}

```
延迟队列记录的是 两个节点间的时间差
调用scheduleDelayedTask加入延迟任务，生成AlarmHandler，插入到DelayQueue中。
每次select返回后，调用DelayQueue::handleAlarm()判断到期的时间，好像一次只能處理一個。


### live555 作为rtsp客户端

testRtspClient.cpp

```plantuml

Medium <|-- RTSPClient
RTSPClient <|-- ourRTSPClient


class Medium
{

}

class RTSPClient
{
    int fInputSocketNum, fOutputSocketNum; // 与服务器连接的本地socket
    struct sockaddr_storage fServerAddress; //rtsp服务器地址，端口
}

class ourRTSPClient
{
    StreamClientState scs;
}

StreamClientState *-- ourRTSPClient

class StreamClientState 
{
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
}


```

#### 客户端发送Describe

将Describe协商信令封装成一个RequestRecord。
第一次发送时没有建立tcp连接，所以会先进行tcp连接。
RTSPClient::openConnection，并设置该socket回调RTSPClient::connectionHandler。将Describe请求放入fRequestsAwaitingConnection队列。

当socket连接成功后，在RTSPClient::connectionHandler回调中。将该socket回调设置为RTSPClient::incomingDataHandler，并将fRequestsAwaitingConnection队列里的请求依次发出

```
    live555建立sock时对sock属性的设置 // todo

```

当请求发出后，服务端返回消息后。进入RTSPClient::incomingDataHandler。找到该请求对应的handle调用。
这里即为 continueAfterDESCRIBE;

#### 服务端Describe返回sdp
continueAfterDESCRIBE

根据sdp中的媒体信息生成MediaSession，MediaSession里有MediaSubsession链表，每一路流对应一个MediaSubsession。

setupNextSubsession
    MediaSubsession::initiate
        申请udp socket。此时还没有确定是tcp还是udp
        createSourceObjects。设置fRTPSource，fReadSource

#### 服务端发送setup消息
根据宏定义选择tcp还是udp

#### 服务端setup消息返回
在tcp回调中调用handleSETUPResponse，根据是否是tcp还是udp，设置目的端口。tcp的话，会移除之前建立的udp socket，并设置tcp回调SocketDescriptor::tcpReadHandler.

continueAfterSETUP中，MediaSubsession生成MediaSink，即DummySink。调用DummySink的startPlaying。

#### 客户端发送play消息
在tcp回调中调用handlePLAYResponse，处理play返回的结果
continueAfterPLAY中，不做什么。

#### MediaSession管理
```plantuml
class MediaSession
{
    MediaSubsession* fSubsessionsHead;
    MediaSubsession* fSubsessionsTail;
}

class MediaSubsession
{
    MediaSession& fParent;
    MediaSubsession* fNext;

    MediaSink* sink;  // 数据处理

    RTPSource* fRTPSource; RTCPInstance* fRTCPInstance;
    FramedSource* fReadSource;
}

```

```plantuml

MediaSource <|-- FramedSource
FramedSource <|-- RTPSource
RTPSource <|-- MultiFramedRTPSource
MultiFramedRTPSource <|-- H264VideoRTPSource
MultiFramedRTPSource <|-- RawAMRRTPSource


FramedSource <|-- AMRAudioSource
AMRAudioSource <|-- AMRDeinterleaver

class MediaSource
{

}

class FramedSource
{
    void getNextFrame( );
    afterGettingFunc* fAfterGettingFunc;  // 得到数据的回调
    void* fAfterGettingClientData;
}

class RTPSource
{
    RTPInterface fRTPInterface; //接收rtp包
    RTPReceptionStatsDB* fReceptionStatsDB; // rtp管理
}

class MultiFramedRTPSource
{
    BufferedPacket* fPacketReadInProgress;  // 正在处理的packet
    ReorderingPacketBuffer* fReorderingBuffer;  //packet包管理
}

FramedFilter <|-- H264or5Fragmenter
class H264or5Fragmenter
{

}

Mediasink <|-- DummySink
Mediasink <|-- RTPSink

class Mediasink
{
    FramedSource* fSource; // 数据源

    Boolean startPlaying();

    afterPlayingFunc* fAfterFunc;
    void* fAfterClientData;
}

class DummySink
{
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
}

FramedSource <|-- FramedFileSource
FramedFileSource <|-- ByteStreamFileSource
class FramedFileSource
{
    FILE* fFid;
}

class ByteStreamFileSource
{

}

FramedSource <|-- FramedFilter
FramedFilter <|-- MPEGVideoStreamFramer
MPEGVideoStreamFramer <|-- H264or5VideoStreamFramer

H264or5VideoStreamFramer <|-- H264VideoStreamFramer


class FramedFilter
{
    FramedSource* fInputSource;
}

class MPEGVideoStreamFramer
{
    class MPEGVideoStreamParser* fParser;
}

class H264or5VideoStreamFramer
{

}

class H264VideoStreamFramer
{

}

RTPSink <|-- MultiFramedRTPSink
MultiFramedRTPSink <|-- VideoRTPSink
VideoRTPSink <|-- H264or5VideoRTPSink
H264or5VideoRTPSink <|-- H264VideoRTPSink


class RTPSink
{
    RTPInterface fRTPInterface;
}

class MultiFramedRTPSink
{
    void buildAndSendPacket(Boolean isFirstPacket);
  void packFrame();
}

class VideoRTPSink
{

}

class H264or5VideoRTPSink
{
     FramedFilter* fOurFragmenter;
}

class H264VideoRTPSink
{

}

StreamParser <|-- MPEGVideoStreamParser
MPEGVideoStreamParser <|-- H264or5VideoStreamParser

class StreamParser
{
    FramedSource* fInputSource;
}

class MPEGVideoStreamParser
{
    MPEGVideoStreamFramer* fUsingSource;
}

class H264or5VideoStreamParser
{

}

```

```plantuml
class RTPInterface 
{
     Medium* fOwner;
     Groupsock* fGS;   // udp收包
     tcpStreamRecord* fTCPStreams;  // tcp包

     TaskScheduler::BackgroundHandlerProc* fReadHandlerProc;   
}

```


#### tcp管理和收发包









GenericMediaServer::incomingConnectionHandlerOnSocket