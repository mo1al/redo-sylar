#include"socket_stream.h"
namespace mo1al
{
SocketStream::SocketStream(Socket::ptr sock,bool owner)
    : m_socket(sock),m_owner(owner)
{

}
SocketStream::~SocketStream()
{
    if(m_owner&&m_socket){
        m_socket->close();
    }
}
int SocketStream::read(void*buffer,size_t length)
{
    if(!m_socket->isConnected()){
        return -1;
    }
    return m_socket->recv(buffer,length);
}
int SocketStream::read(ByteArray::ptr ba,size_t length)
{
    if(!m_socket->isConnected()){
        return -1;
    }
    std::vector<iovec>buffer;
    // 获取可写缓冲区，通过 scatter/gather 一次性接收多段数据
    ba->getWriteBuffers(buffer,length);
    int rt = m_socket->recv(&buffer[0],buffer.size());
    if(rt>0){
        ba->setPosition(ba->getPosition()+rt);
    }
    return rt;
}
int SocketStream::write(const void*buffer,size_t length)
{
    if(!m_socket->isConnected()){
        return -1;
    }
    return m_socket->send(buffer,length);
}
int SocketStream::write(ByteArray::ptr ba,size_t length)
{
    if(!m_socket->isConnected()){
        return -1;
    }
    std::vector<iovec>buffer;
    // 获取可读缓冲区，通过 scatter/gather 一次性发送多段数据
    ba->getReadBuffers(buffer,length);
    int rt = m_socket->send(&buffer[0],buffer.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}
void SocketStream::close()
{
    if(m_socket){
        m_socket->close();
    }
}
}