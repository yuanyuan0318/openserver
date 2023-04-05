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
