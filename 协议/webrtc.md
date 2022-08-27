ICE Candidate (Interactive Connectivity Establishment Candidate)
ICE是交互式连接建立的意思，它是一种用于NAT（网络地址转换器）的技术，用于建立VOIP、点对点、即时通信和其他类型的交互式媒体的通信。通常情况下，ICE Candidate 的信息是关于数据将被交换的IP地址和端口。

WebRTC将Candidates分为三种类型。
- host 类型 (Host Candidate): 即本地内网的IP和端口。IPv6也是属于这一种。这个地址来源于本地的物理网卡或逻辑网卡上的地址，对于具有公网地址或者同一内网的端可以用。host 类型的 Candidate 是最容易收集的。
- srflx 类型 (Server Reflexive Candidate): 这是本地机器上NAT映射后的外部网络的IP和端口。是通过信令方式向 STUN 服务器发送 binding request 找到NAT映射后的地址。
- relay 类型 (Relayed Candidate): 即中继服务器的IP和端口。这个地址是端发送 Allocate 请求到 TURN 服务器 ，由 TURN server 用于中继的地址和端口

WebRTC 中我们会遇到三种类型的服务器，分别是：STUN服务器、TURN服务器和信令服务器 

STUN服务器 :
用于帮助通信双方发现有关其公网 IP 的信息并打开防火墙端口。这要解决的主要问题是，很多设备都在小型专用网络中的 NAT 路由器后面；NAT 基本上允许传出请求及其响应，但会阻止任何其他“未经请求的”传入请求。
STUN 服务器充当向其发出请求的临时中间人，它在 NAT 设备上打开一个端口以允许响应返回，这意味着现在有一个已知的开放端口可供其他对等方使用。这就被称为NAT打洞技术。
是否能打洞成功需要看NAT的类型。

TURN服务器 :
TURN 服务器是公共可访问位置的中继，以防无法进行 P2P 连接。 仍然存在打孔不成功的情况，例如由于更严格的防火墙。在这些情况下，两个对等方根本无法直接进行P2P通信。
它们的所有流量都通过 TURN 服务器进行中继(非常占用带宽) 。
一般是通信双方都可以不受限制地连接的第三方服务器，并且只是将数据从一个对等点转发到另一个。TURN 服务器的一种流行实现是coTURN。

信令服务器 :
WebRTC 规范对信令服务器没有任何说明，因为每个应用程序的信令机制都是非常独特的，并且可以采用多种不同的形式。

STUN 是 NAT 打洞的，而 TURN 是做流量转发的，TURN 这种流量会经过relay服务器
这里需要说明一点，relay 服务是通过 TURN 协议实现的。所以我们经常说的 relay 服务器或 TURN 服务器它们是同一个意思，都是指中继服务器。上一章的 croc 的中继服务器其实就相当于这里的TURN服务器。

