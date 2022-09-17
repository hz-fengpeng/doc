```plantuml

rtspServer *-- Core
pathManager *-- Core
path *-- pathManager

serverUDPListener *-- Server

class Core
{
    rtspServer      *rtspServer
	rtspsServer     *rtspServer
    pathManager     *pathManager     //全局路径管理

    done chan struct{}

    createResources(initial bool)
}

class pathManager
{
    // 路径管理
    paths     map[string]*path     // 推流拉流时的url名字与path的对应关系
}

stream  *-- path
ServerStream *-- stream


class path
{
    name      string      // 推流时的url名字
    stream    *stream      // 在record时生成
}

class stream 
{
	nonRTSPReaders *streamNonRTSPReadersMap
	rtspStream     *gortsplib.ServerStream
	streamTracks   []streamTrack
}

class ServerStream
{
    tracks Tracks
	mutex                   sync.RWMutex
	s                       *Server
	readersUnicast          map[*ServerSession]struct{}    // 存放了该流的消费者
}


class rtspServer
{
    srv       *gortsplib.Server    // rtsp server实现

    wg        sync.WaitGroup

    conns     map[*gortsplib.ServerConn]*rtspConn      // 对应Server中的ServerConn
	sessions  map[*gortsplib.ServerSession]*rtspSession     // 对应Server中的Serversion

    pathManager               *pathManager   // 路径管理，对应core里的pathManager
}

class Server
{
    Handler ServerHandler
    RTSPAddress string

    tcpListener        net.Listener   // Listen函数返回

    Listen func(network string, address string) (net.Listener, error) // 默认是net.Listen.

    ListenPacket func(network, address string) (net.PacketConn, error) //默认是net.ListenPacket.

    udpRTPListener     *serverUDPListener

    udpRTPPacketBuffer *rtpPacketMultiBuffer   // 推流时的包buffer

    conns              map[*ServerConn]struct{}  // 管理tcp连接， tcp accept返回一个net.Conn，封装成ServerConn
    sessions           map[string]*ServerSession  // 管理ServerSession， 一个ServerConn会生成一个ServerSession
}

class ServerSession
{
    s        *Server
	secretID string // must not be shared, allows to take ownership of the session
	author   *ServerConn

    request     chan sessionRequestReq

    announcedTracks     Tracks // publish   announce消息时sdp里的音视频轨
    setuppedTracks      map[int]*ServerSessionSetuppedTrack    // 发送setup时的tracks
    setuppedTransport   *Transport                      // 传输协议

    writeBuffer         *ringbuffer.RingBuffer // 包，推流时只分配8个, 拉流时分配好多个

    conns               map[*ServerConn]struct{}

    setuppedStream      *ServerStream // read 拉流时读取的Stream

}

class rtspSession
{
    // 对应ServerSession
    stream          *stream       // 对应path里的stream
    path            *path
    pathManager     rtspSessionPathManager
}

class ServerSessionSetuppedTrack
{
    id               int
	tcpChannel       int
	udpRTPReadPort   int
	udpRTPWriteAddr  *net.UDPAddr
	udpRTCPReadPort  int
	udpRTCPWriteAddr *net.UDPAddr

	// publish
	udpRTCPReceiver *rtcpreceiver.RTCPReceiver
	reorderer       *rtpreorderer.Reorderer   // 存放包 包packet，udp时才分配
	cleaner         *rtpcleaner.Cleaner
}

clientData *-- serverUDPListener
ServerSession *-- clientData

class clientData
{
    ss           *ServerSession
	track        *ServerSessionSetuppedTrack   // 对应的轨
	isPublishing bool
}

class serverUDPListener
{
    s *Server

	pc           *net.UDPConn
	listenIP     net.IP

    clientsMutex sync.RWMutex
	clients      map[clientAddr]*clientData    // 收包
}


Conn *-- ServerConn

class ServerConn
{
    s     *Server
	nconn net.Conn   // accept连接返回的net.Conn
    conn       *conn.Conn   // 封装io
    readFunc   func(readRequest chan readReq) error  //sc.readFuncStandard

    session    *ServerSession  // 该连接对应的session
}

class Conn
{
    w   io.Writer
	br  *bufio.Reader
	req base.Request
	res base.Response
	fr  base.InterleavedFrame
}

class Request
{
    // request method
	Method Method

	// request url
	URL *url.URL

	// map of header values
	Header Header

	// optional body
	Body []byte
}

RingBuffer *-- ServerSession

class RingBuffer
{
    size       uint64
	readIndex  uint64
	writeIndex uint64
	closed     int64
	buffer     []unsafe.Pointer
	event      *event
}

class stream
{
    nonRTSPReaders *streamNonRTSPReadersMap
	rtspStream     *gortsplib.ServerStream
	streamTracks   []streamTrack
}

```

主协程 阻塞在接收Core信道done
主协程在初始化完成后，会开启一个协程，用于处理异常情况，发送时，close(done)

createResources: 创建rtsp协程等

newRTSPServer：生成rtspServer结构体，并启动rtspServer::srv协程
                启动rtspServer.run()协程主要是接收srv的错误


协程: func (pm *pathManager) run()      处理url路径，全局唯一，给rtspServer
协程: func (pa *path) run()             

协程: func (s *rtspServer) run()        主要是等待Server的错误

协程: func (s *Server) run()                   处理各种请求 比如 (1)connNew <- nconn  (2)处理sessionRequest
    协程: s.tcpListener.Accept()               主要监听tcp socket， 生成nconn -> connNew信道
    
    协程: func (sc *ServerConn) run()            sc.runInner   读取信道readReq里的请求并处理。 
                                                (sc *ServerConn) handleRequest(req *base.Request) 

    协程: func (sc *ServerConn) runReader        sc.runReader。 读取请求存放到readReq信道  sc.readFuncStandard

    协程: func (ss *ServerSession) run()         ss.runInner()   处理req请求。
    协程: func (ss *ServerSession) runWriter()   发送包给拉流端协程，或者给推流端rtcp  ss.s.udpRTPListener.write

协程: func (u *serverUDPListener) runReader()    接收udp包协程，processRTP


推流
1. tcp accept返回一个net.Conn，封装成ServerConn。保存在Server::conns.
   同时生成一个rtspConn，保存在rtspServer::conns，以ServerConn作为key

2. 生成一个ServerSession，保存在Server::sessions中。以secretID作为key。
   同时生成一个rtspSession，保存在rtspServer::sessions

3. Annoance时，生成ServerSession的Tracks(接口)，对应sdp里的轨 [(ss *ServerSession) handleRequest]
   同时将path路径名 放入到rtspSession的pathManager.. [s.pathManager.publisherAdd], 生成path
   pathManager生成path，放到pm.paths[]中

4. Setup时，生成ServerSession的setuppedTracks
   
5. Record时，为每一个setuppedTracks生成reorderer包buffer，将推流端的ip, port, serverSession, tracks加入到serverUDPListener的clients里。  三元组(ServerSession, ServerSessionSetuppedTrack, isPublish)
   
   (s *rtspSession) onRecord    func (pa *path) sourceSetReady    path生成stream， ServerStream,  pa.handlePublisherRecord(req). 存在pathManage。
   rtspSession里的stream也是path里的stream。


拉流
1. 发送describe，生成SererSession，rtspConn::OnDescribe中pathManage取ServerStream
从pathManager里根据pathName取一个path，返回path里的stream，ServerStream。根据ServerStream来生成sdp

2. setup，生成setuppedTracks[trackID].   赋值setuppedStream *ServerStream // read
   推流和拉流通过ServerStream来联系。

3. play，ss.setuppedStream.readerSetActive   将st.readersUnicast[ss] = struct{}{}   Serversion放到ServerStream.readersUnicast里, 分配ServerSession里的writebuffer.

4. 终端发送teardown，服务器正常发送。最后ServerConn::runReader会返回EOF err. 
   (sc *ServerConn) run() 里关闭net.Conn.Close() 随后进行收尾工作，从st.readersUnicast中移除


收包处理
在收包协程中，收到一个pkt，根据发送端的ip:port, 找到对应的ServerSessionSetuppedTrack，放到其reorder buffer中。
从中取出顺序的包。调用handle来处理对应的包。提供trackid，packet。找到rtspSession:stream:streamTracks[trackid]
进行包处理后，s.rtspStream.WritePacketRTP。根据ServerStream里的readersUnicast,调用每一个收流端进行发包，写进它们的writebuffer。

发包协程。
发包协程就是从writeBuffer里取包然后发送。