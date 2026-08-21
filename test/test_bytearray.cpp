#include"sylar.h"
#include"bytearray.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_byte()
{
// #define XX(type,len,write_fun,read_fun,basesize)\
// {\
//     std::vector<type>vec;\
//     for(size_t i=0;i<len;i++){\
//         vec.push_back(i);\
//     }\
//     mo1al::ByteArray::ptr byte(new mo1al::ByteArray(basesize));\
//     for(auto&v:vec){\
//         byte->write_fun(v);\
//     }\
//     byte->setPosition(0);\
//     for(size_t i=0;i<vec.size();i++){\
//         type v= byte->read_fun();\
//         SYLAR_ASSERT(v==vec[i]);\
//     }\
//     SYLAR_ASSERT(byte->getReadSize()==0);\
//     SYLAR_LOG_INFO(g_logger)<<#write_fun " / " #read_fun \
//         " (" #type ") len=" << len\
//         << " basesize = "<<basesize\
//         << " size = "<<byte->getSize();\
// }
//     XX(int8_t,  100, writeFint8, readFint8, 1);
//     XX(uint8_t, 100, writeFuint8, readFuint8, 1);
//     XX(int16_t,  100, writeFint16,  readFint16, 1);
//     XX(uint16_t, 100, writeFuint16, readFuint16, 1);
//     XX(int32_t,  100, writeFint32,  readFint32, 1);
//     XX(uint32_t, 100, writeFuint32, readFuint32, 1);
//     XX(int64_t,  100, writeFint64,  readFint64, 1);
//     XX(uint64_t, 100, writeFuint64, readFuint64, 1);

//     XX(int32_t,  100, writeInt32,  readInt32, 1);
//     XX(uint32_t, 100, writeUint32, readUint32, 1);
//     XX(int64_t,  100, writeInt64,  readInt64, 1);
//     XX(uint64_t, 100, writeUint64, readUint64, 1);
// #undef XX


#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    mo1al::ByteArray::ptr ba(new mo1al::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        SYLAR_ASSERT(v == vec[i]); \
    } \
    SYLAR_ASSERT(ba->getReadSize() == 0); \
    SYLAR_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    SYLAR_ASSERT(ba->writeToFile("./" #type "_" #len "-" #read_fun ".dat")); \
    mo1al::ByteArray::ptr ba2(new mo1al::ByteArray(base_len * 2)); \
    SYLAR_ASSERT(ba2->readFromFile("./" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    SYLAR_ASSERT(ba->toString() == ba2->toString()); \
    SYLAR_ASSERT(ba->getPosition() == 0); \
    SYLAR_ASSERT(ba2->getPosition() == 0); \
}
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX
} 

int main()
{
    test_byte();
    return 0;
}