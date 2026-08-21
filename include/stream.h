// stream.h —— 流抽象：定义读写接口，支持定长读写。
#pragma once
#include"bytearray.h"
#include<memory>
namespace mo1al
{
// 流抽象基类：定义读写接口与定长读写
class Stream
{
public:
    typedef std::shared_ptr<Stream>ptr;
    virtual ~Stream(){};
    // 从流中读取数据到 buffer
    virtual int read(void*buffer,size_t length)=0;
    // 将数据读入到 ByteArray
    virtual int read(ByteArray::ptr ba,size_t length)=0;

    // 定长读取：循环读取直到读满 length 字节
    virtual int readFixSize(void* buffer, size_t length);
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    // 向流中写入数据
    virtual int write(const void*buffer,size_t length)=0;
    // 将 ByteArray 中的数据写入流
    virtual int write(ByteArray::ptr ba,size_t length)=0;

    // 定长写入：循环写入直到写完 length 字节
    virtual int writeFixSize(const void* buffer, size_t length);
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close()=0;
};
}