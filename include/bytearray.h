// bytearray.h —— 字节流缓冲区：支持定长/变长编码的二进制读写。
#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <string.h>

namespace mo1al
{
// 字节流缓冲区：基于链表节点存储，支持定长/变长编码读写
class ByteArray
{
public:
    typedef std::shared_ptr<ByteArray>ptr;
    // 存储节点：指向一块连续内存
    struct Node
    {
        Node(size_t s); // 构造指定大小的内存块
        Node();
        ~Node(); // 释放内存
        char*ptr;
        Node*next;
        size_t size;
    };

    ByteArray(size_t base_size=4096);
    ~ByteArray();
    // 定长编码（Fix）：按固定字节数存储，需考虑字节序
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32 (int32_t value);
    void writeFuint32 (uint32_t value);
    void writeFint64 (int64_t value);
    void writeFuint64 (uint64_t value);
    // 变长编码（Var）：ZigZag + 7bit 编码，节省空间
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);

    void writeStringF16(const std::string&value);
    void writeStringF32(const std::string&value);
    void writeStringF64(const std::string& value);
    void writeStringVint(const std::string& value);
    void writeStringWithoutLength(const std::string& value);

    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float    readFloat();
    double   readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    // 只读函数：不修改内部游标
    void read(void*buf,size_t size,size_t position)const;

    size_t getPosition()const {return m_position;}
    void setPosition(size_t v) ;

    bool writeToFile(const std::string&name)const;
    bool readFromFile(const std::string&name);

    size_t getBaseSize()const {return m_baseSize;}
    size_t getReadSize()const {return m_size-m_position;}

    bool isLittleEndian()const;
    void setIsLittleEndian(bool val);

    std::string toString()const;
    std::string toHexString()const;

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>&buffers,uint64_t len,uint64_t position)const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    size_t getSize()const {return m_size;}
private:
    void addCapacity(size_t size);
    size_t getCapacity()const {return m_capacity-m_position;}
private:
    size_t m_baseSize;
    size_t m_position; // 读/写游标位置
    size_t m_capacity; // 总容量
    size_t m_size; // 已写入数据大小
    int8_t m_endian;
    Node* m_root;
    Node* m_cur;
};
}