```plantuml

RTPMemoryObject <|--  RTPSession
RTPMemoryObject <|-- RTPTransmitter
RTPMemoryManager *-- RTPMemoryObject
RTPTransmitter *-- RTPSession

abstract RTPMemoryManager{
    virtual void *AllocateBuffer(size_t numbytes, int memtype) = 0;
    virtual void FreeBuffer(void *buffer) = 0;
}

class RTPMemoryObject{
    - RTPMemoryManager *mgr;
}

class RTPSession{
    - RTPSessionSources sources;
    + Create()
    - bool created; // 是否create标记位
    - RTPTransmitter *rtptrans;
    - RTPPacketBuilder packetbuilder;
    - RTPPollThread *pollthread; // 线程
}

RTPTransmitter <|-- RTPUDPv4Transmitter
RTPTransmitter <|-- RTPTCPTransmitter

abstract RTPTransmitter{
    virtual int Poll() = 0; // 接受数据

}

class RTPUDPv4Transmitter{
    - SocketType rtpsock,rtcpsock;
    - destinations;  // 目的地，AddDestination,路由hash表
    - rawpacketlist; // 包list

}

class RTPTCPTransmitter{

}

```

- RTPTransmitter在create的时候申请socket，（该socket还是阻塞的）进行绑定等工作，负责数据收发和接受 ```int RTPUDPv4Transmitter::Poll()```,将收到的数据生成RTPRawPacket, 放到```std::list<RTPRawPacket*> rawpacketlist;```
- ```RTPSession::ProcessPolledData()``` poll 之后进行ProcessPolledData，```RTPSources::ProcessRawPacket``` 根据RTPRawPacket生成RTPPacket-->```RTPSources::ProcessRTPPacket```--> ```RTPInternalSourceData::ProcessRTPPacket```
  -> 在收到数据时，RTPTransmitter会把收到的包根据ssrc分到RTPSources里的RTPInternalSourceData，每一个源都对应一个RTPInternalSourceData。```std::list<RTPPacket *> packetlist;```
  
```

#0  0x00005555555ac436 in jrtplib::RTPInternalSourceData::ProcessRTPPacket(jrtplib::RTPPacket*, jrtplib::RTPTime const&, bool*, jrtplib::RTPSources*) ()
#1  0x0000555555580bb7 in jrtplib::RTPSources::ProcessRTPPacket(jrtplib::RTPPacket*, jrtplib::RTPTime const&, jrtplib::RTPAddress const*, bool*) ()
#2  0x00005555555806e4 in jrtplib::RTPSources::ProcessRawPacket(jrtplib::RTPRawPacket*, jrtplib::RTPTransmitter**, int, bool) ()
#3  0x00005555555803a7 in jrtplib::RTPSources::ProcessRawPacket(jrtplib::RTPRawPacket*, jrtplib::RTPTransmitter*, bool) ()
#4  0x000055555557c8cc in jrtplib::RTPSession::ProcessPolledData() ()
#5  0x000055555557bbf6 in jrtplib::RTPSession::Poll() ()
#6  0x000055555559e84e in MyRTPUDPSession::GetMyRTPData(unsigned char*, unsigned long*, unsigned long) ()
#7  0x0000555555562a32 in RtspClient::GetMediaData(MediaSession*, unsigned char*, unsigned long*, unsigned long) ()
#8  0x000055555556d224 in RtspClient::GetMediaData(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, unsigned char*, unsigned long*, unsigned long) ()
#9  0x0000555555560341 in main ()

```
```plantuml

RTPSources <|-- RTPSessionSources
RTPInternalSourceData *-- RTPSources
RTPSourceData <|-- RTPInternalSourceData

class RTPSourceData{
    RTPAddress *rtpaddr  // 源地址
    std::list<RTPPacket *> packetlist;
}

class RTPInternalSourceData{
    // 一些处理rtp的函数
    - GetNextPacket()
}

class RTPSources{
   - RTPInternalSourceData *owndata  \n// 根据自己的ssrc生成
   - RTPKeyHashTable sourcelist   \n// 存放ssrc和RTPInterlSourceData键值对

}

class RTPSessionSources{
    - RTPSession &rtpsession;

}

```

JThread start函数会调用pthread函数，将TheThread传入pthread函数。其中最终启动 虚函数 Thread(),由继承类实现

RTPSession::Poll() 起线程来拉数据
```plantuml

JThread <|-- RTPPollThread

class JThread{
    JThread();
    int Start();
    virtual void *Thread() = 0;
    static void *TheThread(void *param);

}

class RTPPollThread{
   + int Start(RTPTransmitter *trans);
   + void Stop();
   - void *Thread();
}

```