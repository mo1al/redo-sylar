// socket_stream.h —— Socket 流：将 Socket 包装为 Stream，提供流式读写。
#pragma once
#include"stream.h"
#include"socket.h"
namespace mo1al
{
// Socket 流：把 Socket 封装为 Stream 接口
class SocketStream:public Stream
{
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock,bool owner=true);
    ~SocketStream();

    virtual int read(void*buffer,size_t length)override;
    virtual int read(ByteArray::ptr ba,size_t length)override;

    virtual int write(const void*buffer,size_t length)override;
    virtual int write(ByteArray::ptr ba,size_t length)override;
    virtual void close()override;
    Socket::ptr getSocket()const {return m_socket;}
    bool isConnected()const {return m_socket&&m_socket->isConnected();}
private:
    Socket::ptr m_socket;
    bool m_owner;
};
}