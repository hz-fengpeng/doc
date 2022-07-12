rtmp服务端默认端口1935
RTMP协议里面的 handshake（握手）流程，机制，以及逻辑。

RTMP 协议会 把一个 大的数据，切分成小的 chunk 来发送，例如 把 NALU 结构 拆成小的 chunk 发送。以上就是 chunk 分片 的好处之一，把大数据分成小分，然后小数据就能在中间插进去。大数据不完整，就不能处理，小数据完整可以优选处理。

但是在实际实现中，有些客户端实现并没有把大数据跟小数据分开线程处理。而是一起顺序处理，发完所有大数据的chunk，再发小数据的chunk。

对于 RTMP 协议来说，他的 传输单元是 chunk。一个 RTMP 包就是一个 chunk。所以 chunk 的格式，也就是 RTMP 报文的格式。

### 握手
1. 该协议是应用层协议，基于TCP。RTMP协议建立连接的过程，也需要有rtmp握手的过程
在TCP连接建立以后，再进行RTMP协议层次的握手。
    1. 在rtmp连接建立以后，服务端要与客户端通过3次交换报文完成握手。rtmp协议握手交换的数据报文是固定大小的。客户端向服务端发送的3个报文为c0、c1、c2，服务端向客户端发送的3个报文为s0、s1、s2。c0与s0的大小为1个字节，c1与s1的大小为1536个字节，c2与s2的大小为1536个字节。

    建立连接后，客户端开始发送C0、C1块到服务器；
    服务器端收到C0或C1后发送S0和S1；
    当客户端收齐S0和S1之后，开始发送C2；
    当服务端收齐C0和C1后，开发发送S2；
    当客户端收到S2，服务端收到C2，握手完成。握手完成后客户端和服务端开始交换信息

    在实际工程应用中，一般是客户端将C0、C1块同时发出，服务器在收到C1块之后同时将S0、S1、S2发给客户端。客户端收到S1之后，发送C2给服务端，握手完成。

    C0和S0数据包占用一个字节，表示RTMP版本号。目前RTMP版本定义为3,0-2是早期的专利产品所使用的值,现已经废弃,4-31是预留值,32-255是禁用值。
    C1和S1数据包占用1536个字节。包含4个字节的时间戳，4个字节的0和1528个字节的随机数。
    C2和S2数据包占用1536个字节，包含4个字节的时间戳，4个字节的对端的时间戳（C2数据包为S1数据包的时间戳，S2为C1数据包的时间戳）。

    握手的过程主要完成了两个工作，一是对rtmp的版本进行校验，二是发送了一些随机数据，用于网络状况的检测。握手成功之后，表示客户端和服务器之间可以正常进行网络通信，接下来就可以进行数据的交互了。

### rtmp数据
#### rtmp header

RTMP header的长度不固定，可能的长度为12字节，8字节，4字节，1字节。具体长度为多少个字节，由RTMP header数据包的第一个字节的高2位决定。
| format| chunk stream ID|            Chunk Stream ID用来表示消息的级别
   2bits     6bits

12字节Header

 1 Byte   3 Bytes  3 Bytes  1 Bytes  4 Bytes
|header|timestamp|bodysize|typeId|streamId|   
bodysize: 除去RTMP header后的数据长度
typeId: 消息类型ID
StreamID：通常用以完成某些特定的工作，如使用ID为0的Stream来完成客户端和服务器的连接和控制，使用ID为1的Stream来完成视频流的控制和播放等工作。

8字节Header
| | timestamp delta| bodysize|typeID|
时间戳的delta，简单讲就是时间戳的变化量

4字节头
| |timestamp delta|

#### rtmp body
AMF英文全称Action Message Format，是Adobe定义的一套用来进行数据打包的格式，主要的版本有AFM0和AMF3，不过发展至今，实际场景中AMF0一直用的比较多，AMF3相对少见
RTMP数据包的序列化就是按照AMF的格式进行的。RTMP的客户端和RTMP的服务端约定好，发送方说，我发送你的数据都是按照某种格式组织的，你如果收到了我发给你的数据包，你就按这个数据包格式进行解析就可以了。这个格式就是我们此处说的AMF。

AMF定义中，首先使用一个字节来表示数据类型。在数据类型后面紧跟着的就是对应类型数据的长度，每一种类型长度字段所占用的字节数可能也不尽相同。AMF中对于某些类型的数据，没有长度的选项

一个rtmp body中可包含多个AMF。

### connect消息
当rtmp客户端和rtmp服务端握手完成之后，客户端就会向服务端发送connect消息。connect消息的格式按照RTMP Header+RTMP Body的格式组织
包括：
    1. app是application的缩写，代表客户端要链接到的，rtmp服务器的应用程序
    2. flashVer表示flash播放器的版本号
    3. 服务器的URL地址
    4. fpad是一个布尔值，用来表示是否使用代理
    5. capabilities
    6. audioCodes表示支持的音频编码格式，支持的每一种格式用一个bit位来表示，所有的比特位进行或运算得到最终的值。有一个音频表可以查。
    7. videoCodecs表示支持的视频编码格式，有一个视频表可以查
    8. videoFunction也比较简单，先用字符串表明类型，然后紧跟着一个number类型的数据，videoFunction的值为1，表示客户端可以执行精确到帧的搜索。

客户端发送connect命令到服务器，请求与服务端的application进行连接；
服务端收到connect命令后，服务器会发送协议消息“Window Acknowledgement size”消息到客户端。服务端同时连接到connect中请求的application；
服务端发送协议消息“Set Peer BandWidth”到客户端；
客户端在处理完服务端发来的“Set Peer BandWidth”消息后，向服务端发送“Window Acknowledgment”消息；
服务端向客户端发送一条用户控制消息（Stream Begin）;
如果连接成功，服务端向客户端发送_result消息，否则发送_error消息。

#### Window Acknowledgement Size消息
Window Acknowledgement Size用来通知对端，如果收到该大小字节的数据，需要回复一个Acknowledgement消息，也就是ACK。本例中，设置的大小为500000，也就是服务端通知客户端如果收到了**累计**50000字节的数据，需要向服务端发送一个ACK的消息，而实际上一般情况一个会话中能达到如此大的数据量比较少，所以我们也看到会回复ACK消息的消息比较少，更多的只是看到设置接收窗口大小的消息（Window Acknowledgement Size）?????????

Acknowlegement消息可以理解为Window Acknowlegement Size满足条件的触发消息，当一端收到的数据大小满足Window Acknowledgement Size设置的大小时，向对端发送Ack消息。

#### Set Peer Bandwidth
该消息里设置对端输出带宽，对端是通过设置Window Acknowledgement Size来实现流量控制的。超过Window Acknowledgement Size后未确认（不发送Acknowledgement）发送端将不再发送消息。所以对端收到set peer bandwidth后，如果之前发送的Window Acknowledgement Size和这里写的的Window Acknowledgement Size不一样，一般会发送一个Window Acknowledgement Size。

#### StreamBegin
服务端发送Set Peer Bandwidth消息之后，客户端向服务端发送Window Acknowledgement Size消息，服务端再向客户端发送一条用户控制消息StreamBegin。
RTMP服务器发送StreamBegin以通知客户端流已经可以使用并且可以用于通信。默认情况下，从客户端成功接收到connect命令后，将在ID 0上发送StreamBegin。StreamBegin的数据字段占用4字节，其类型占用2个字节，所以RTMP Body部分总共占用6个字节（类型+数据）。其中数据字段代表已开始运行的流的流ID，此例中为1。

#### _result消息
​rtmp客户端发送connect消息之后，rtmp server会给客户端发送_result消息，通过该消息通知客户端连接状态（success/fail）
一个_result消息由4部分组成，类型标识，transaction ID，properties，response related information

类型标识：_result
transaction ID: 默认为1
properties: 看出properties中包含了两个Object类型的数据，一个fmsVer表示了FMS 服务器的版本信息,另外一个capabilites表示容量
response related information: 关于connect连接的响应，以object类型进行组织

### createStream

创建完RTMP连接之后就可以创建或者访问RTMP流，对于推流端，客户端要向服务器发送一个releaseStream命令消息，之后是createStream命令消息，对于拉流端，则要发送play消息请求视频资源。

RTMP客户端发送此消息到服务端，创建一个逻辑通道，用于消息通信。音频、视频、元数据均通过createStream创建的数据通道进行交互，而releaseStream与createStream相对应，为什么有的时候会在createStream之前先来一次releaseStream呢？这就像我们很多的服务实现中，先进行一次stop，然后再进行start一样。因为我们每次开启新的流程，并不能确保之前的流程是否正常走完，是否出现了异常情况，异常的情况是否已经处理等等，所以，做一个类似于恢复初始状态的操作，releaseStream就是这个作用

有一个 标识事务ID

客户端发送createStream请求之后，服务端会反馈一个结果给客户端，如果成功，则返回_result，如果失败，则返回_error。成功的时候，返回一个streamID，失败的时候返回失败的原因，类似于错误码

### releaseStream
release消息的组织结构，comandName + transactionID + commandObject + 流的一些用户名和密码信息等（可能没有）

### ​publish
对于推流端，经过releaseStream，createStream消息之后，得到了_result消息之后，接下来客户端就可以发起publish消息。推流端使用publish消息向rtmp服务器端发布一个命名的流，发布之后，任意客户端都可以以该名称请求视频、音频和数据。

commandName：使用string类型，表示消息类型（“publish”）;
transactionID：使用number类型表示事物ID；
commandObject：对于publish消息，该部分为空，用null类型表示；
publishName：发布的流的名称，使用string类型表示，比如我们发布到rtmp://192.168.1.101:1935/rtmp_live/test，则test为流名称，也可以省略，此时该字段为空字符；
publishType：发布的流的类型，使用string类型表示，有3种类型，分别为live、record、append，record表示发布的视频流到rtmp服务器application对应的目录下会将发布的流录制成文件，append表示会将发布的视频流追加到原有的文件，如果原来没有文件就创建，live则不会在rtmp服务器上产生文件。

客户端发送publish消息给rtmp服务端后，服务端会向客户端反馈一条消息，该消息采用了onStatus

一般在客户端收到服务端返回的针对publish的onStatus消息之后，如果没有异常，推流端还会向服务器发送一条SetDataFrame的消息，其中包含onMetaData消息，这一条消息的主要作用是告诉服务端，推流段关于音视频的处理采用的一些参数，比如音频的采样率，通道数，帧率，视频的宽，高等信息。

### play拉流
我们来简单介绍一下关于play的流程，客户端向服务端发送play指令之后，服务端收到之后向客户端发送SetChunkSize消息，实际场景中大都在服务器回复客户端connect消息的时候一起发送setChunkSize消息；
服务端向客户端发送StreamIsRecorded消息（实际场景中比较少见）；服务端向客户端发送StreamBegin消息，向客户端指示流传输的开始。还有onStatus-play reset, onStatus-play start等。

play消息：
    commandName：命令的名称，为“connect”;
    transaction ID：事务ID，用number类型表示；
    command Object：如果有，用object类型表示，如果没有，则使用null类型指明；
    stream Name：请求的流的名称，一般在url中application后面的字段，如rtmp://192.17.1.202:1935/rtmp_live/test，rtmp_live为application，test为流的名称；
    start：可选字段，使用number类型表示，指示开始时间，默认值为-2，表示客户端首先尝试命名为streamName的实时流（官方文档中说以秒单位，实际抓包文件中看到的单位应该是毫秒，要注意）；
    duration：可选字段，用number类型表示，指定播放时间，默认值为-1，表示播放到流结束；
    reset：可选字段，用boolean类型表示，用来指示是否刷新之前的播放列表；

### 音视频数据
#### 音频
音频数据也是按照RTMP Header + Rtmp Body的组织结构来进行封装的。
音频的数据的Body部分正是按照FLV的格式进行组装的。而Flv的封装以tag为单位来进行组织，对于音频数据，包含tagHeader + tagData，tagHeader占用一个字节，表明音频编码的相关参数，tagData为具体的音频编码数据。

flv中audioTag的Header是如何组织的：
tag占用1个字节，我们从高到低
    音频编码格式  4bit
    音频采样率：  2bit
    位深         1bit
    声道          1bit

#### 视频
Body中打包视频数据的方式也与音频类似。首先用一个字节表示视频数据的header，之后是压缩后的视频数据（压缩后的数据是使用FLV的标准进行封装的）

header 
    视频帧类型：4bit,   表示该帧视频是关键帧还是非关键帧
    codec ID:  4bit    表示该帧的数据编码codecID

