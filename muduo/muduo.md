- [bind](#bind)
- [listen](#listen)
- [socket listen epoll_ctl](#socket-listen-epoll_ctl)
- [readv](#readv)

``` plantuml

title Reactor事件处理
class EventLoop
{
    Poller poller_;
    TimerQueue timerQueue_;  //超时管理

    EventLoop::loop()
}

class Channel
{
    const int  fd_; /// A selectable I/O channel.
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
    EventLoop* loop_; // 属于哪个eventloop
}

class TimerQueue
{
     EventLoop* loop_;
    const int timerfd_;  // 一次只监听队列中的一个
    Channel timerfdChannel_;
    // Timer list sorted by expiration
    TimerList timers_;  //定时器链表
}

class EventLoopThreadPool
{
    boost::ptr_vector<EventLoopThread> threads_; // 存放每一个EventLoopThread
    std::vector<EventLoop*> loops_;  // 存放每一个EventLoopThread中的EventLoop
}

class EventLoopThread
{
    EventLoop* loop_;
    Thread thread_;
}

```

- EventLoop事件循环器，其中有Poller, 来进行poll或epoll。
- 把poll的数据封装成一个Channel。里面有fd，对应的回调函数。
- 定时器也用timefd来实现，定时一到调用TimerQueue::handleRead，只监听队列中的最早的channel。
- TcpServer里有EventLoopThreadPool，相当于一个线程池，管理EventLoopThread。

```plantuml
actor user
user ->EventLoop: loop()
EventLoop ->Poller: pool()
Poller -> Poller: fillActiveChannels
Poller -> EventLoop: active Channels
EventLoop -> ChannelA: handleEvent
EventLoop -> ChannelB: handleEvent

EventLoop ->Poller: pool()


```

```plantuml
title TcpServer服务器

class TcpServer
{
    // 管理accept后的TcpConnection
    Acceptor acceptor_;  // 处理客户端连接
    EventLoopThreadPool threadPool_;  // 线程池
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    EventLoop* loop_;  // 事件循环
    ConnectionMap connections_;  // map<string, TcpConnectionPtr> 管理连接
}

class TcpConnection
{
    boost::scoped_ptr<Socket> socket_;
    boost::scoped_ptr<Channel> channel_;
}

class Acceptor
{
    // 用于tcp连接
    Channel acceptChannel_;
    Socket acceptSocket_;

    NewConnectionCallback newConnectionCallback_; // 新连接来时的handle
}

class socket
{   
    //tcp listen时的监听socket
    const int sockfd_;
}

```
- TcpServer在初始化时，accptor还没监听，只是申请了socket
- TcpServer在start()后，调用Acceptor::listen，对socket进行监听。
- 在有新连接时，创建TcpConnection，由TcpServer管理。同时调用TcpConnection::connectEstablished()，将起加入poll，同时调用用户设置的connectionCallback。


```plantuml

actor user
user -> EventLoop: loop()
EventLoop -> Channel: handleEvent()
Channel -> Acceptor: handleRead()
Acceptor -> Acceptor: accept
Acceptor -> TcpServer: newConnection
TcpServer -> TcpConnection: connectEstablished

```


```
echo

- 注册TcpServer的回调函数，ConnectionCallback，MessageCallback
- TcpServer初始化时，设置acceptor的ConnectionCallback

```
#### bind
```
#0  bind () at ../sysdeps/unix/syscall-template.S:78
#1  0x00005555555d1f62 in muduo::net::sockets::bindOrDie (sockfd=6, addr=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/SocketsOps.cc:95
#2  0x00005555555f0b35 in muduo::net::Socket::bindAddress (this=0x555555832928, addr=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Socket.cc:63
#3  0x00005555555ebf7a in muduo::net::Acceptor::Acceptor (this=0x555555832920, loop=0x7fffffffd2d0, listenAddr=...,
    reuseport=false) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Acceptor.cc:36
#4  0x00005555555dd2b9 in muduo::net::TcpServer::TcpServer (this=0x7fffffffd380, loop=0x7fffffffd2d0, listenAddr=...,
    nameArg=..., option=muduo::net::TcpServer::kNoReusePort)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpServer.cc:31
#5  0x00005555555c21af in EchoServer::EchoServer (this=0x7fffffffd380, loop=0x7fffffffd2d0, listenAddr=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/echo.cc:12
#6  0x00005555555c5394 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:14

```
#### listen
```
#0  muduo::net::Acceptor::listen (this=0x555555832920) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Acceptor.cc:50
#1  0x00005555555e408b in boost::_mfi::mf0<void, muduo::net::Acceptor>::operator() (this=0x7fffffffd238, p=0x555555832920)
    at /usr/include/boost/bind/mem_fn_template.hpp:49
#2  0x00005555555e372b in boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> >::operator()<boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list0> (this=0x7fffffffd248, f=..., a=...) at /usr/include/boost/bind/bind.hpp:259
#3  0x00005555555e2c90 in boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >::operator() (this=0x7fffffffd238) at /usr/include/boost/bind/bind.hpp:1294
#4  0x00005555555e1d1e in boost::detail::function::void_function_obj_invoker0<boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >, void>::invoke (function_obj_ptr=...)
    at /usr/include/boost/function/function_template.hpp:159
#5  0x00005555555c7bbb in boost::function0<void>::operator() (this=0x7fffffffd230)
    at /usr/include/boost/function/function_template.hpp:759
#6  0x00005555555c62d4 in muduo::net::EventLoop::runInLoop(boost::function<void ()>&&) (this=0x7fffffffd2d0, cb=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:196
#7  0x00005555555dda59 in muduo::net::TcpServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpServer.cc:70
#8  0x00005555555c239e in EchoServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/echo.cc:22
#9  0x00005555555c53a3 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:15

```

```
定时器
#0  epoll_ctl () at ../sysdeps/unix/syscall-template.S:78
#1  0x00005555555cbe19 in muduo::net::EPollPoller::update (this=0x555555832530, operation=1, channel=0x555555832680)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:177
#2  0x00005555555cb8f6 in muduo::net::EPollPoller::updateChannel (this=0x555555832530, channel=0x555555832680)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:127
#3  0x00005555555c6601 in muduo::net::EventLoop::updateChannel (this=0x7fffffffd2d0, channel=0x555555832680)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:244
#4  0x00005555555ecc28 in muduo::net::Channel::update (this=0x555555832680)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:56
#5  0x00005555555c748c in muduo::net::Channel::enableReading (this=0x555555832680)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.h:73
#6  0x00005555555e4c21 in muduo::net::TimerQueue::TimerQueue (this=0x555555832670, loop=0x7fffffffd2d0)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TimerQueue.cc:102
#7  0x00005555555c564f in muduo::net::EventLoop::EventLoop (this=0x7fffffffd2d0)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:71
#8  0x00005555555c535b in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:12
```

```
#0  muduo::net::EPollPoller::poll (this=0x555555832530, timeoutMs=10000, activeChannels=0x7fffffffd318)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:57
#1  0x00005555555c5df7 in muduo::net::EventLoop::loop (this=0x7fffffffd2d0)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:113
#2  0x00005555555c53b2 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:16

```

```
#0  epoll_ctl () at ../sysdeps/unix/syscall-template.S:78
#1  0x00005555555cbe19 in muduo::net::EPollPoller::update (this=0x555555832530, operation=1, channel=0x555555832820)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:177
#2  0x00005555555cb8f6 in muduo::net::EPollPoller::updateChannel (this=0x555555832530, channel=0x555555832820)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:127
#3  0x00005555555c6601 in muduo::net::EventLoop::updateChannel (this=0x7fffffffd2d0, channel=0x555555832820)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:244
#4  0x00005555555ecc28 in muduo::net::Channel::update (this=0x555555832820)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:56
#5  0x00005555555c748c in muduo::net::Channel::enableReading (this=0x555555832820)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.h:73
#6  0x00005555555c594b in muduo::net::EventLoop::EventLoop (this=0x7fffffffd2d0)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:89
#7  0x00005555555c535b in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:12


```

```
#0  epoll_ctl () at ../sysdeps/unix/syscall-template.S:78
#1  0x00005555555cbe19 in muduo::net::EPollPoller::update (this=0x555555832530, operation=1, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:177
#2  0x00005555555cb8f6 in muduo::net::EPollPoller::updateChannel (this=0x555555832530, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:127
#3  0x00005555555c6601 in muduo::net::EventLoop::updateChannel (this=0x7fffffffd2d0, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:244
#4  0x00005555555ecc28 in muduo::net::Channel::update (this=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:56
#5  0x00005555555c748c in muduo::net::Channel::enableReading (this=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.h:73
#6  0x00005555555ec128 in muduo::net::Acceptor::listen (this=0x555555832920)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Acceptor.cc:53
#7  0x00005555555e408b in boost::_mfi::mf0<void, muduo::net::Acceptor>::operator() (this=0x7fffffffd238, p=0x555555832920)
    at /usr/include/boost/bind/mem_fn_template.hpp:49
#8  0x00005555555e372b in boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> >::operator()<boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list0> (this=0x7fffffffd248, f=..., a=...) at /usr/include/boost/bind/bind.hpp:259
#9  0x00005555555e2c90 in boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >::operator() (this=0x7fffffffd238) at /usr/include/boost/bind/bind.hpp:1294
#10 0x00005555555e1d1e in boost::detail::function::void_function_obj_invoker0<boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >, void>::invoke (function_obj_ptr=...)
    at /usr/include/boost/function/function_template.hpp:159
#11 0x00005555555c7bbb in boost::function0<void>::operator() (this=0x7fffffffd230)
    at /usr/include/boost/function/function_template.hpp:759
#12 0x00005555555c62d4 in muduo::net::EventLoop::runInLoop(boost::function<void ()>&&) (this=0x7fffffffd2d0, cb=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:196
#13 0x00005555555dda59 in muduo::net::TcpServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpServer.cc:70
#14 0x00005555555c239e in EchoServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/echo.cc:22
#15 0x00005555555c53a3 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:15

```

#### socket listen epoll_ctl
```
#0  epoll_ctl () at ../sysdeps/unix/syscall-template.S:78
#1  0x00005555555cbe19 in muduo::net::EPollPoller::update (this=0x555555832530, operation=1, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:177
#2  0x00005555555cb8f6 in muduo::net::EPollPoller::updateChannel (this=0x555555832530, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/poller/EPollPoller.cc:127
#3  0x00005555555c6601 in muduo::net::EventLoop::updateChannel (this=0x7fffffffd2d0, channel=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:244
#4  0x00005555555ecc28 in muduo::net::Channel::update (this=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:56
#5  0x00005555555c748c in muduo::net::Channel::enableReading (this=0x555555832930)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.h:73
#6  0x00005555555ec128 in muduo::net::Acceptor::listen (this=0x555555832920)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Acceptor.cc:53
#7  0x00005555555e408b in boost::_mfi::mf0<void, muduo::net::Acceptor>::operator() (this=0x7fffffffd238, p=0x555555832920)
    at /usr/include/boost/bind/mem_fn_template.hpp:49
#8  0x00005555555e372b in boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> >::operator()<boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list0> (this=0x7fffffffd248, f=..., a=...) at /usr/include/boost/bind/bind.hpp:259
#9  0x00005555555e2c90 in boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >::operator() (this=0x7fffffffd238) at /usr/include/boost/bind/bind.hpp:1294
#10 0x00005555555e1d1e in boost::detail::function::void_function_obj_invoker0<boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >, void>::invoke (function_obj_ptr=...)
    at /usr/include/boost/function/function_template.hpp:159
#11 0x00005555555c7bbb in boost::function0<void>::operator() (this=0x7fffffffd230)
    at /usr/include/boost/function/function_template.hpp:759
#12 0x00005555555c62d4 in muduo::net::EventLoop::runInLoop(boost::function<void ()>&&) (this=0x7fffffffd2d0, cb=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:196
#13 0x00005555555dda59 in muduo::net::TcpServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpServer.cc:70
#14 0x00005555555c239e in EchoServer::start (this=0x7fffffffd380)
    at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/echo.cc:22
#15 0x00005555555c53a3 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:15
```

```
#0  EchoServer::onConnection (this=0x7fffffffd380, conn=...) at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/echo.cc:26
#1  0x00005555555c4f3d in boost::_mfi::mf1<void, EchoServer, boost::shared_ptr<muduo::net::TcpConnection> const&>::operator() (this=0x555555832b98, p=0x7fffffffd380, a1=...)
    at /usr/include/boost/bind/mem_fn_template.hpp:165
#2  0x00005555555c4b32 in boost::_bi::list2<boost::_bi::value<EchoServer*>, boost::arg<1> >::operator()<boost::_mfi::mf1<void, EchoServer, boost::shared_ptr<muduo::net::TcpConnection> const&>, boost::_bi::rrlist1<boost::shared_ptr<muduo::net::TcpConnection> const&> > (this=0x555555832ba8, f=..., a=...) at /usr/include/boost/bind/bind.hpp:319
#3  0x00005555555c47a1 in boost::_bi::bind_t<void, boost::_mfi::mf1<void, EchoServer, boost::shared_ptr<muduo::net::TcpConnection> const&>, boost::_bi::list2<boost::_bi::value<EchoServer*>, boost::arg<1> > >::operator()<boost::shared_ptr<muduo::net::TcpConnection> const&> (this=0x555555832b98, a1=...) at /usr/include/boost/bind/bind.hpp:1306
#4  0x00005555555c4331 in boost::detail::function::void_function_obj_invoker1<boost::_bi::bind_t<void, boost::_mfi::mf1<void, EchoServer, boost::shared_ptr<muduo::net::TcpConnection> const&>, boost::_bi::list2<boost::_bi::value<EchoServer*>, boost::arg<1> > >, void, boost::shared_ptr<muduo::net::TcpConnection> const&>::invoke (function_obj_ptr=..., a0=...)
    at /usr/include/boost/function/function_template.hpp:159
#5  0x00005555555d7af2 in boost::function1<void, boost::shared_ptr<muduo::net::TcpConnection> const&>::operator() (this=0x555555832b90, a0=...)
    at /usr/include/boost/function/function_template.hpp:759
#6  0x00005555555d4aa6 in muduo::net::TcpConnection::connectEstablished (this=0x555555832b20) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpConnection.cc:300
#7  0x00005555555dcf42 in boost::_mfi::mf0<void, muduo::net::TcpConnection>::call<boost::shared_ptr<muduo::net::TcpConnection> > (this=0x555555832e60, u=...)
    at /usr/include/boost/bind/mem_fn_template.hpp:40
#8  0x00005555555dcd47 in boost::_mfi::mf0<void, muduo::net::TcpConnection>::operator()<boost::shared_ptr<muduo::net::TcpConnection> > (this=0x555555832e60, u=...)
    at /usr/include/boost/bind/mem_fn_template.hpp:55
#9  0x00005555555dc3a2 in boost::_bi::list1<boost::_bi::value<boost::shared_ptr<muduo::net::TcpConnection> > >::operator()<boost::_mfi::mf0<void, muduo::net::TcpConnection>, boost::_bi::list0> (this=0x555555832e70, f=..., a=...) at /usr/include/boost/bind/bind.hpp:259
#10 0x00005555555db2c6 in boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::TcpConnection>, boost::_bi::list1<boost::_bi::value<boost::shared_ptr<muduo::net::TcpConnection> > > >::operator() (this=0x555555832e60) at /usr/include/boost/bind/bind.hpp:1294
#11 0x00005555555da4ac in boost::detail::function::void_function_obj_invoker0<boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::TcpConnection>, boost::_bi::list1<boost::_bi::value<boost::shared_ptr<muduo::net::TcpConnection> > > >, void>::invoke (function_obj_ptr=...) at /usr/include/boost/function/function_template.hpp:159
#12 0x00005555555c7bbb in boost::function0<void>::operator() (this=0x7fffffff8e80) at /usr/include/boost/function/function_template.hpp:759
#13 0x00005555555c62d4 in muduo::net::EventLoop::runInLoop(boost::function<void ()>&&) (this=0x7fffffffd2d0, cb=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:196
#14 0x00005555555ddf03 in muduo::net::TcpServer::newConnection (this=0x7fffffffd380, sockfd=8, peerAddr=...) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpServer.cc:101
#15 0x00005555555e3ed7 in boost::_mfi::mf2<void, muduo::net::TcpServer, int, muduo::net::InetAddress const&>::operator() (this=0x5555558329f0, p=0x7fffffffd380, a1=8, a2=...)
    at /usr/include/boost/bind/mem_fn_template.hpp:280
#16 0x00005555555e361d in boost::_bi::list3<boost::_bi::value<muduo::net::TcpServer*>, boost::arg<1>, boost::arg<2> >::operator()<boost::_mfi::mf2<void, muduo::net::TcpServer, int, muduo::net::InetAddress const&>, boost::_bi::rrlist2<int, muduo::net::InetAddress const&> > (this=0x555555832a00, f=..., a=...) at /usr/include/boost/bind/bind.hpp:398
#17 0x00005555555e2b31 in boost::_bi::bind_t<void, boost::_mfi::mf2<void, muduo::net::TcpServer, int, muduo::net::InetAddress const&>, boost::_bi::list3<boost::_bi::value<muduo::net::TcpServer*>, boost::arg<1>, boost::arg<2> > >::operator()<int, muduo::net::InetAddress const&> (this=0x5555558329f0, a1=@0x7fffffff9fd4: 8, a2=...) at /usr/include/boost/bind/bind.hpp:1318
#18 0x00005555555e1c04 in boost::detail::function::void_function_obj_invoker2<boost::_bi::bind_t<void, boost::_mfi::mf2<void, muduo::net::TcpServer, int, muduo::net::InetAddress const&>, boost::_bi::list3<boost::_bi::value<muduo::net::TcpServer*>, boost::arg<1>, boost::arg<2> > >, void, int, muduo::net::InetAddress const&>::invoke (function_obj_ptr=..., a0=8, a1=...)
    at /usr/include/boost/function/function_template.hpp:159
#19 0x00005555555ec45f in boost::function2<void, int, muduo::net::InetAddress const&>::operator() (this=0x5555558329e8, a0=8, a1=...)
    at /usr/include/boost/function/function_template.hpp:759
#20 0x00005555555ec1e9 in muduo::net::Acceptor::handleRead (this=0x555555832920) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Acceptor.cc:68
#21 0x00005555555e408b in boost::_mfi::mf0<void, muduo::net::Acceptor>::operator() (this=0x555555832970, p=0x555555832920) at /usr/include/boost/bind/mem_fn_template.hpp:49
#22 0x00005555555ec789 in boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> >::operator()<boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::rrlist1<muduo::Timestamp> > (
    this=0x555555832980, f=..., a=...) at /usr/include/boost/bind/bind.hpp:259
#23 0x00005555555ec699 in boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >::operator()<muduo::Timestamp> (this=0x555555832970, a1=...) at /usr/include/boost/bind/bind.hpp:1306
---Type <return> to continue, or q <return> to quit---
#24 0x00005555555ec5ea in boost::detail::function::void_function_obj_invoker1<boost::_bi::bind_t<void, boost::_mfi::mf0<void, muduo::net::Acceptor>, boost::_bi::list1<boost::_bi::value<muduo::net::Acceptor*> > >, void, muduo::Timestamp>::invoke (function_obj_ptr=..., a0=...) at /usr/include/boost/function/function_template.hpp:159
#25 0x00005555555ed69e in boost::function1<void, muduo::Timestamp>::operator() (this=0x555555832968, a0=...) at /usr/include/boost/function/function_template.hpp:759
#26 0x00005555555ed03e in muduo::net::Channel::handleEventWithGuard (this=0x555555832930, receiveTime=...) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:107
#27 0x00005555555ecd32 in muduo::net::Channel::handleEvent (this=0x555555832930, receiveTime=...) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:79
#28 0x00005555555c5ecc in muduo::net::EventLoop::loop (this=0x7fffffffd2d0) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:125
#29 0x00005555555c53b2 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:16

```

#### readv
```
#0  muduo::net::sockets::readv (sockfd=8, iov=0x7ffffffea030, iovcnt=2) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/SocketsOps.cc:167
#1  0x00005555555ec8c2 in muduo::net::Buffer::readFd (this=0x555555832c38, fd=8, savedErrno=0x7fffffffa084)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Buffer.cc:38
#2  0x00005555555d4c81 in muduo::net::TcpConnection::handleRead (this=0x555555832b20, receiveTime=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/TcpConnection.cc:320
#3  0x00005555555dc9a9 in boost::_mfi::mf1<void, muduo::net::TcpConnection, muduo::Timestamp>::operator() (this=0x555555832d30, p=0x555555832b20, a1=...)
    at /usr/include/boost/bind/mem_fn_template.hpp:165
#4  0x00005555555db8a6 in boost::_bi::list2<boost::_bi::value<muduo::net::TcpConnection*>, boost::arg<1> >::operator()<boost::_mfi::mf1<void, muduo::net::TcpConnection, muduo::Timestamp>, boost::_bi::rrlist1<muduo::Timestamp> > (this=0x555555832d40, f=..., a=...) at /usr/include/boost/bind/bind.hpp:319
#5  0x00005555555dab29 in boost::_bi::bind_t<void, boost::_mfi::mf1<void, muduo::net::TcpConnection, muduo::Timestamp>, boost::_bi::list2<boost::_bi::value<muduo::net::TcpConnection*>, boost::arg<1> > >::operator()<muduo::Timestamp> (this=0x555555832d30, a1=...) at /usr/include/boost/bind/bind.hpp:1306
#6  0x00005555555d9e90 in boost::detail::function::void_function_obj_invoker1<boost::_bi::bind_t<void, boost::_mfi::mf1<void, muduo::net::TcpConnection, muduo::Timestamp>, boost::_bi::list2<boost::_bi::value<muduo::net::TcpConnection*>, boost::arg<1> > >, void, muduo::Timestamp>::invoke (function_obj_ptr=..., a0=...)
    at /usr/include/boost/function/function_template.hpp:159
#7  0x00005555555ed69e in boost::function1<void, muduo::Timestamp>::operator() (this=0x555555832d28, a0=...)
    at /usr/include/boost/function/function_template.hpp:759
#8  0x00005555555ed03e in muduo::net::Channel::handleEventWithGuard (this=0x555555832cf0, receiveTime=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:107
#9  0x00005555555ecd1d in muduo::net::Channel::handleEvent (this=0x555555832cf0, receiveTime=...)
    at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/Channel.cc:74
#10 0x00005555555c5ecc in muduo::net::EventLoop::loop (this=0x7fffffffd2d0) at /home/fengpeng/WebServer/muduo/muduo-master/muduo/net/EventLoop.cc:125
#11 0x00005555555c53b2 in main () at /home/fengpeng/WebServer/muduo/muduo-master/examples/simple/echo/main.cc:16
```


TcpServer中有 EventLoop和EventLoopThreadPool