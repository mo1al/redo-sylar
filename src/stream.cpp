#include"stream.h"
namespace mo1al
{
int Stream::readFixSize(void* buffer, size_t length)
{
    size_t offset = 0;
    int64_t left = length;
    while(left > 0) {
        int64_t len = read((char*)buffer + offset, left);
        if(len <= 0) { // 读到结尾或出错
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}
int Stream::readFixSize(ByteArray::ptr ba, size_t length)
{
    int64_t left = length;
    while(left > 0) {
        int64_t len = read(ba, left);
        if(len <= 0) { // 读到结尾或出错
            return len;
        }
        left -= len;
    }
    return length;
}
int Stream::writeFixSize(const void* buffer, size_t length)
{
    size_t offset = 0;
    int64_t left = length;
    while(left > 0) {
        int64_t len = write((const char*)buffer + offset, left);
        if(len <= 0) { // 写满或出错
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}
int Stream::writeFixSize(ByteArray::ptr ba, size_t length)
{
    int64_t left = length;
    while(left>0)
    {
        int64_t len = write(ba,left);
        if(len<=0){ // 写满或出错
            return len;
        }
        left -= len;
    }
    return length;
}
}