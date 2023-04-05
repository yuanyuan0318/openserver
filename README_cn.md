# OpenServer
OpenServer是一款超轻量、超迷你、Actor模式、组件设计的高性能、高并发的跨全平台服务器框架。

OpenServer主要使用OpenSocket和OpenThread等开源项目实现。OpenSocket是高性能复用IO库，OpenThread可以轻松实现Actor模式。
组件设计模式，把业务分解封装成组件，再由不同的组件组装出不同的Actor，从而实现业务逻辑。

Actor模式和组件设计，可以简化业务逻辑，易于单元测试，也更容易维护和寻找BUG。

配合OpenJson使用，可以把相同的业务封装成组件，然后用配置文件json去控制组装和启动相关服务，大幅软件开发效率。


**OpenLinyou致力于C++跨平台高并发高性能服务器框架开发，全平台设计，支持windows、linux、mac、安卓和iOS等平台，可以充分利用各平台的优势和工具，在VS或者XCode上开发写代码，做到一份代码跑全部平台。**
OpenLinyou：https://www.openlinyou.com
OpenServer:https://github.com/openlinyou/openserver
https://gitee.com/linyouhappy/openserver

## 跨平台支持
Linux和安卓使用epoll，Windows使用IOCP(wepoll)，iOS和Mac使用kqueue，其他系统使用select。

## 编译和执行
请安装cmake工具，用cmake可以构建出VS或者XCode工程，就可以在vs或者xcode上编译运行。
源代码：https://github.com/openlinyou/openserver
https://gitee.com/linyouhappy/openserver
```
#克隆项目
git clone https://github.com/openlinyou/openserver
cd ./openserver
#创建build工程目录
mkdir build
cd build
cmake ..
#如果是win32，在该目录出现openserver.sln，点击它就可以启动vs写代码调试
make
./helloworld
```

## 技术特点
OpenServer的技术特点：
1. 跨全平台设计，此服务器框架可以运行在安卓和iOS上。
2. Linux和安卓使用epoll，Windows使用IOCP(wepoll)，iOS和Mac使用kqueue，其他系统使用select。
3. 支持IPv6，小巧迷你，采用Actor模式和组件设计，通过组件去组装业务。
4. Actor模式和组件设计，可以非常容易实现高并发和分布式。也可以通过配置文件去定制业务和启动服务。
5. 一条线程一个actor，一个actor由多个组件组装。用玩积木的方式去做服务器开发。

## 1.测试例子
这个是一个实现UDP通信的例子。
一条线程就是一个actor，把它成为服务者（server）。所有的actor都一样，不同的是放了不同的组件。

首先实现两个组件ComUDPClient和ComUDPServer，它们都继承OpenComSocket，从而可以接收UDP网络消息。
通过OpenServer::RegisterCom，把这两个组件注册到Server上。它的作用很简单，就是创建一个组件对象，并给它起个名字。
这个组件对象，有一个New函数，从而实现通过名字就可以创建对应的对象。

接下来就是组装Server。OpenServer::StartServer这个接口实现创建Server，给这个server取个名字，然后指定上述的组件名称。就可以把这个server创建出来。尽管OpenServer::StartServer会返回这个Server对象指针，但是不要轻易修改它的属性数据，因为它的属性数据由它所在的线程管理。

本测试例子创建了一个UDPServer服务器，创建了两个UDPClient，它们的名字必须要不一致。两个客户端通过UDP向服务器定时发送心跳。

```C++
#include <assert.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "openbuffer.h"
#include "openserver.h"
using namespace open;

////////////ComUDPClientMsg//////////////////////
struct ComUDPClientMsg : public OpenMsgProtoMsg
{
    int port_;
    std::string ip_;
    ComUDPClientMsg() :OpenMsgProtoMsg(), port_(0) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return ComUDPClientMsg::MsgId(); }
};

////////////ComUDPServerMsg//////////////////////
struct ComUDPServerMsg : public OpenMsgProtoMsg
{
    int port_;
    std::string ip_;
    ComUDPServerMsg() :OpenMsgProtoMsg(), port_(0) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return ComUDPServerMsg::MsgId(); }
};

////////////ComUDPClient//////////////////////
class ComUDPClient : public OpenComSocket
{
    int fd_;
    int port_;
    std::string ip_;
    OpenBuffer buffer_;
    OpenSlice slice_;
public:
    ComUDPClient() :OpenComSocket(), fd_(-1){}
    virtual ~ComUDPClient() {}
    virtual inline ComUDPClient* newCom() { return new ComUDPClient; }
private:
    virtual bool start(OpenServer* worker)
    {
        OpenComSocket::start(worker);
        //timer
        startInterval(3000);
        return true;
    }
    void sendData(OpenBuffer& buffer)
    {
        int ret = OpenSocket::Instance().send(fd_, buffer.data(), (int)buffer.size());
        if (ret != 0)
        {
            printf("[ComUDPClient][%s:%d]sendData ret = %d\n", ip_.c_str(), port_, ret);
            assert(false);
        }
    }
    void sendHeartBeat()
    {
        buffer_.clear();
        uint32_t uid = 1 + pid_;
        buffer_.pushUInt32(uid);
        uint32_t id = 1;
        buffer_.pushUInt32(id);
        std::string data = "ping";
        buffer_.pushUInt32((uint32_t)data.size());
        buffer_.pushBack(data.data(), data.size());
        sendData(buffer_);
    }
    virtual void onOpenMsgProto(OpenMsgProto& proto)
    {
        if (!proto.msg_) return;
        if (ComUDPClientMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<ComUDPClientMsg> protoMsg = std::dynamic_pointer_cast<ComUDPClientMsg>(proto.msg_);
            if (!protoMsg)
            {
                assert(false); return;
            }
            port_ = protoMsg->port_;
            ip_ = protoMsg->ip_;
            fd_ = OpenSocket::Instance().udp(pid_, 0, 0);
            OpenSocket::Instance().udpConnect(fd_, protoMsg->ip_.data(), protoMsg->port_);
            OpenSocket::Instance().start(pid_, fd_);
        }
    }
    virtual void onSocketUdp(const OpenSocketMsg& msg) 
    {
        std::string ip; int port = 0;
        const char* address = msg.option_;
        if (OpenSocket::UDPAddress(address, ip, port) != 0)
        {
            printf("[ComUDPClient][%s:%d]onSocketUdp UDPAddress error.\n", ip.c_str(), port);
            assert(false);
            return;
        }
        slice_.setData((unsigned char*)msg.data(), msg.size());
        uint32_t uid = 0;
        uint32_t id = 0;
        uint32_t len = 0;
        slice_.popUInt32(uid);
        slice_.popUInt32(id);
        slice_.popUInt32(len);
        std::vector<char> vect;
        vect.resize(len + 1, 0);
        slice_.popFront(vect.data(), len);
        printf("[ComUDPClient][%s]onSocketUdp uid:%d, id:%d, data:%s\n", pname_.data(), uid, id, vect.data());

    }
    virtual void onSocketClose(const OpenSocketMsg& msg) {}
    virtual void onSocketOpen(const OpenSocketMsg& msg) 
    {
        sendHeartBeat();
    }
    virtual void onSocketError(const OpenSocketMsg& msg) {}
    virtual void onSocketWarning(const OpenSocketMsg& msg) {}
    virtual void onOpenTimerProto(const OpenTimerProto& proto)
    {
        if (fd_ >= 0)
        {
            sendHeartBeat();
        }
        //timer
        startInterval(3000);
    }
};

////////////ComUDPServer//////////////////////
class ComUDPServer : public OpenComSocket
{
    int fd_;
    int port_;
    std::string ip_;
    OpenBuffer buffer_;
    OpenSlice slice_;
public:
    ComUDPServer():OpenComSocket(), fd_(-1){}
    virtual ~ComUDPServer() {}
    virtual inline ComUDPServer* newCom() { return new ComUDPServer; }
private:
    virtual bool start(OpenServer* worker)
    {
        OpenComSocket::start(worker);
        return true;
    }
    void sendHeartBeat(const char* address, uint32_t uid)
    {
        buffer_.clear();
        buffer_.pushUInt32(uid);
        uint32_t id = 1;
        buffer_.pushUInt32(id);
        std::string data = "pong";
        buffer_.pushUInt32((uint32_t)data.size());
        buffer_.pushBack(data.data(), data.size());
        sendData(address, buffer_);
    }
    void sendData(const char* address, OpenBuffer& buffer)
    {
        int ret = OpenSocket::Instance().udpSend(fd_, address, buffer.data(), (int)buffer.size());
        if (ret != 0)
        {
            printf("[ComUDPServer][%s:%d]sendData ret = %d\n", ip_.c_str(), port_, ret);
            assert(false);
        }
    }
    virtual void onOpenMsgProto(OpenMsgProto& proto)
    {
        if (!proto.msg_) return;
        if (ComUDPServerMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<ComUDPServerMsg> protoMsg = std::dynamic_pointer_cast<ComUDPServerMsg>(proto.msg_);
            if (!protoMsg)
            {
                assert(false); return;
            }
            fd_ = OpenSocket::Instance().udp((uintptr_t)pid_, protoMsg->ip_.data(), protoMsg->port_);
            if (fd_ < 0)
            {
                printf("[ComUDPServer]listen failed. addr:%s:%d, fd_=%d\n", protoMsg->ip_.data(), protoMsg->port_, fd_);
                assert(false);
                return;
            }
            OpenSocket::Instance().start((uintptr_t)pid_, fd_);
            printf("[ComUDPServer]listen success. addr:%s:%d, fd_=%d\n", protoMsg->ip_.data(), protoMsg->port_, fd_);
        }
    }
    virtual void onSocketUdp(const OpenSocketMsg& msg)
    {
        std::string ip; int port = 0;
        const char* address = msg.option_;
        if (OpenSocket::UDPAddress(address, ip, port) != 0)
        {
            printf("[ComUDPServer][%s:%d]onSocketUdp UDPAddress error.\n", ip.c_str(), port);
            assert(false);
            return;
        }
        slice_.setData((unsigned char*)msg.data(), msg.size());
        uint32_t uid = 0;
        uint32_t id = 0;
        uint32_t len = 0;
        slice_.popUInt32(uid);
        slice_.popUInt32(id);
        slice_.popUInt32(len);
        std::vector<char> vect;
        vect.resize(len + 1, 0);
        slice_.popFront(vect.data(), len);
        printf("[ComUDPServer][%s]onSocketUdp[%s:%d] uid:%d, id:%d, data:%s\n", pname_.data(), ip.data(), id, uid, id, vect.data());

        sendHeartBeat(address, uid);
    }
    virtual void onSocketClose(const OpenSocketMsg& msg) {}
    virtual void onSocketOpen(const OpenSocketMsg& msg) {}
    virtual void onSocketError(const OpenSocketMsg& msg) {}
    virtual void onSocketWarning(const OpenSocketMsg& msg) {}
};

int main()
{
    OpenApp::Instance().start();
    OpenTimer::Run();
    //register component
    OpenServer::RegisterCom<ComUDPClient>("ComUDPClient");
    OpenServer::RegisterCom<ComUDPServer>("ComUDPServer");

    //create server
    OpenServer* server = OpenServer::StartServer("UDPServer", { "ComUDPServer" });
    std::vector<OpenServer*> clients = {
        OpenServer::StartServer("UDPClient1", { "ComUDPClient" }),
        OpenServer::StartServer("UDPClient2", { "ComUDPClient" })
    };
    const std::string testListenIp = "0.0.0.0";
    const std::string testServerIp = "127.0.0.1";
    const int testServerPort_ = 9999;
    //start server
    {
        auto msg = std::shared_ptr<ComUDPServerMsg>(new ComUDPServerMsg);
        msg->ip_ = testListenIp;
        msg->port_ = testServerPort_;
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = msg;
        bool ret = OpenThread::Send(server->pid(), proto);
        assert(ret);
    }
    //wait server start.
    OpenThread::Sleep(1000);
    //start client
    {
        auto msg = std::shared_ptr<ComUDPClientMsg>(new ComUDPClientMsg);
        msg->ip_ = testServerIp;
        msg->port_ = testServerPort_;
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = msg;
        for (size_t i = 0; i < clients.size(); i++)
        {
            bool ret = OpenThread::Send(clients[i]->pid(), proto);
            assert(ret);
        }
    }
    //Do not delete data.
    clients.clear();

    OpenApp::Instance().wait();
    printf("Pause\n");
    return getchar();
}

```

## 全部源文件
+ src/socket_os.h
+ src/socket_os.c
+ src/opensocket.h
+ src/opensocket.cpp
+ src/wepoll.h(only win32)
+ src/wepoll.c(only win32)
+ src/openthread.h
+ src/openthread.cpp
+ src/openserver.h
+ src/openserver.cpp
+ src/opentime.h
+ src/opentime.cpp
+ src/openbuffer.h
+ src/openbuffer.cpp
+ src/openjson.h
+ src/openjson.cpp
+ src/opencsv.h
+ src/opencsv.cpp
+ src/openfsm.h
+ src/openfsm.cpp
   