#include"bytearray.h"
#include"en_dian.h"
#include"log.h"
#include<iomanip>
#include <fstream>
#include <sstream>
#include<math.h>
#include<algorithm>
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace mo1al
{
ByteArray::Node::Node(size_t s)
    :ptr(new char[s]),next(nullptr),size(s)
{

}
ByteArray::Node::Node()
    : ptr(nullptr),next(nullptr),size(0)
{

}   
ByteArray::Node::~Node()
{
    if(ptr){
        delete[] ptr;
    }
}
ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size),m_capacity(base_size),m_position(0),m_size(0),
        m_root(new Node(base_size)),m_cur(m_root),m_endian(SYLAR_BIG_ENDIAN)
{

}   
ByteArray::~ByteArray()
{
    Node* tmp = m_root;
    while (tmp)
    {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}
bool ByteArray::isLittleEndian()const
{
    return m_endian == SYLAR_LITTLE_ENDIAN;
}
void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = SYLAR_LITTLE_ENDIAN;
    } else {
        m_endian = SYLAR_BIG_ENDIAN;
    }
}
void ByteArray::writeFint8(int8_t value)
{
    write(&value,sizeof(value));
}
void ByteArray::writeFuint8(uint8_t value)
{
    write(&value,sizeof(value));
}
void ByteArray::writeFint16(int16_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}
void ByteArray::writeFuint16(uint16_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}
void ByteArray::writeFint32 (int32_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}
void ByteArray::writeFuint32 (uint32_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}
void ByteArray::writeFint64 (int64_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}
void ByteArray::writeFuint64 (uint64_t value)
{
    if(SYLAR_BYTE_ORDER!=m_endian)
    {
       value =  byteswap(value);
    }
    write(&value,sizeof(value));
}

// ZigZag 编码：将有符号整数映射为无符号整数，便于变长编码
static uint32_t EncodeZigzag32(const int32_t&v)
{
    if(v<0){
        return ((uint32_t)(-v))*2-1;
    }
    else{
        return (uint32_t)v*2;
    }
}
static uint64_t EncodeZigzag64(const int64_t&v)
{
    if(v<0){
        return ((uint64_t)(-v))*2-1;
    }
    else{
        return (uint64_t)v*2;
    }
}
static int32_t DecodeZigzag32(const uint32_t&v)
{
    return (v>>1)^-(v&1);
}
static int64_t DecodeZigzag64(const uint64_t&v)
{
    return (v>>1)^-(v&1);
}

void ByteArray::writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}
void ByteArray::writeUint32(uint32_t value)
{
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value>=0x80){
        tmp[i++] = (value&0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] =value;
    write(tmp,i);
}
void ByteArray::writeInt64(int64_t value)
{
    writeUint64(EncodeZigzag64(value));
}
void ByteArray::writeUint64(uint64_t value)
{
    uint8_t tmp[10];
    uint8_t i = 0;
    while (value>=0x80)
    {
        tmp[i++] = (value&0x7F)|0x80;
        value>>=7;
    }
    tmp[i++] = value;
    write(tmp,i);
}

void ByteArray::writeFloat(float value)
{
    uint32_t v;
    memcpy(&v,&value,sizeof(value));
    writeFuint32(v);
}
void ByteArray::writeDouble(double value)
{
    uint64_t v;
    memcpy(&v,&value,sizeof(value));
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string&value)
{
    writeFuint16(value.size());
    write(value.c_str(),value.size());
}
void ByteArray::writeStringF32(const std::string&value)
{
    writeFuint32(value.size());
    write(value.c_str(),value.size());
}
void ByteArray::writeStringF64(const std::string& value)
{
    writeFuint64(value.size());
    write(value.c_str(),value.size());
}
void ByteArray::writeStringVint(const std::string& value)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}
void ByteArray::writeStringWithoutLength(const std::string& value)
{
    write(value.c_str(), value.size());
}

int8_t   ByteArray::readFint8()
{
    int8_t v;
    read(&v,sizeof(v));
    return v;
}
uint8_t  ByteArray::readFuint8(){
    uint8_t v;
    read(&v,sizeof(v));
    return v;
}
#define XX(type)\
    type v;\
    read(&v,sizeof(v));\
    if(m_endian==SYLAR_BYTE_ORDER){\
        return v;\
    }\
    else{\
        return byteswap(v);\
    }
int16_t  ByteArray::readFint16(){
    XX(int16_t);
}
uint16_t ByteArray::readFuint16()
{
    XX(uint16_t);
}
int32_t  ByteArray::readFint32()
{
    XX(int32_t);
}
uint32_t ByteArray::readFuint32()
{
    XX(uint32_t);
}
int64_t  ByteArray::readFint64()
{
    XX(int64_t);
}
uint64_t ByteArray::readFuint64()
{
    XX(uint64_t);
}
#undef XX
int32_t  ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}
uint32_t ByteArray::readUint32(){
    uint32_t result = 0;
    for(int i=0;i<32;i+=7)
    {
        uint8_t b = readFuint8();
        if(b<0x80){
            result |= ((uint32_t)b)<<i;
            break;
        }
        else{
            result |= ((uint32_t)(b&0x7f))<<i;
        }
    }
    return result;
}
int64_t  ByteArray::readInt64()
{
    return DecodeZigzag64(readUint64());
}
uint64_t ByteArray::readUint64()
{
    uint64_t result = 0;
    for(int i=0;i<64;i+=7){
        uint8_t b = readFuint8();
        if(b<0x80){
            result |= ((uint64_t)b)<<i;
            break;
        }
        else{
            result |= (((uint64_t)b)&(0x7f))<<i; 
        }
    }
    return result;
}

float    ByteArray::readFloat()
{
    uint32_t v = readFuint32();
    float value;
    memcpy(&value,&v,sizeof(v));
    return value;
}
double   ByteArray::readDouble()
{
    uint64_t v = readFuint64();
    double value;
    memcpy(&value,&v,sizeof(v));
    return value;

}

// 先读出长度，再读出内容
std::string ByteArray::readStringF16()
{
    uint16_t len = readFuint16();
    std::string buf;
    buf.resize(len);
    read(&buf[0],len);
    return buf;
}
std::string ByteArray::readStringF32()
{
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringF64()
{
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringVint()
{
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear()
{
    // 重置状态，仅保留根节点
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node*tmp = m_root->next;
    while(tmp){
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

// 将数据写入节点链
void ByteArray::write(const void* buf, size_t size)
{   
    if(size==0)return;
    addCapacity(size);
    
    size_t npos = m_position%m_baseSize;
    size_t ncap = m_cur->size-npos;
    size_t bpos = 0;
    while (size>0)
    {
        if(ncap>=size){
            memcpy(m_cur->ptr+npos,(const char*)buf+bpos,size);
            if(m_cur->size==(npos+size)){
                m_cur = m_cur->next;
            }
            m_position += size;
            size=0;
        }
        else{
            memcpy(m_cur->ptr+npos,(const char*)buf+bpos,ncap);
            bpos += ncap;
            m_position += ncap;
            size-=ncap;
            npos = 0;
            m_cur = m_cur->next;
            ncap = m_cur->size;
        }
    }
    if(m_position>m_size){
        m_size = m_position;
    }
}
void ByteArray::read(void* buf, size_t size)
{   
    if(size>getReadSize()){
        throw std::out_of_range("not read len enough");
    }
    size_t npos = m_position%m_baseSize;
    size_t ncap = m_cur->size-npos;
    size_t bpos = 0;
    while(size>0)
    {
        if(ncap>=size){
            // 使用 char* 保证按字节步进
            memcpy((char*)buf+bpos,m_cur->ptr+npos,size);
            if(m_cur->size==(size+npos)){
                m_cur = m_cur->next;
            }
            m_position += size;
            size = 0;
        }
        else{
            memcpy((char*)buf+bpos,m_cur->ptr+npos,ncap);
            m_position += ncap;
            npos = 0;
            bpos += ncap;
            size -= ncap;
            ncap = m_cur->size;
            m_cur = m_cur->next;
        }
    }
    // 已由 getReadSize 保证剩余可读长度足够
}

void ByteArray::read(void*buf,size_t size,size_t position)const
{
    if(size>(m_size-position)){
        throw std::out_of_range("not read len enough");   
    }

    size_t npos = position % m_baseSize;  
    size_t ncap = m_cur->size - npos; 
    size_t bpos = 0; 
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) { 
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap; // 累计已拷贝字节数
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}
void ByteArray::setPosition(size_t v)
{
    if(v>m_capacity){
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_size<m_position){
        m_size = m_position;
    }
    // 重新定位当前节点指针
    m_cur = m_root;
    while(v>m_cur->size){
        m_cur = m_cur->next;
        v-=m_cur->size;
    }
    // 恰好在节点末尾则指向下一节点
    if(v==m_cur->size){
        m_cur = m_cur->next;
    }
}

bool ByteArray::writeToFile(const std::string&name)const
{
    std::ofstream ofs;
    ofs.open(name,std::ios::trunc|std::ios::binary);
    if(!ofs){
        SYLAR_LOG_ERROR(g_logger)<<"writeToFile name="<<name<<" error,errno = "<<errno
            <<"errstr = "<<strerror(errno);
        return false;
    }
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node*cur = m_cur;
    while(read_size>0)
    {
        int diff = pos%m_baseSize;
        int64_t len = std::min((int64_t)m_baseSize-diff,read_size); 
        ofs.write(cur->ptr+diff,len);
        pos += len;
        read_size -= len;
        cur = cur->next;
    }
    return true;
}
bool ByteArray::readFromFile(const std::string&name)
{
    std::ifstream ifs;
    ifs.open(name,std::ios::binary);
    if(!ifs){
        SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    std::shared_ptr<char>buff(new char[m_baseSize],[](char*ptr){
        delete[] ptr;
    });
    while(!ifs.eof()){
        ifs.read(buff.get(),m_baseSize);
        write(buff.get(),ifs.gcount()); // gcount 为本次实际读取长度
    }
    return true;
}
void ByteArray::addCapacity(size_t size)
{
    if(size==0)return ;
    size_t old_cap = getCapacity();
    if(old_cap>=size){
        return ;
    }
    size -= old_cap;
    size_t count = ceil(1.0*size/m_baseSize);
    Node* tmp = m_root;
    while(tmp->next){
        tmp = tmp->next;
    }

    Node* first = NULL; // 新链表的头节点
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    // 若扩容前已无剩余容量（old_cap == 0），说明 m_cur 已指向满节点末尾，
    // 需将其指向新扩容的第一个节点
    if(old_cap == 0) {
        m_cur = first;
    }
}

// 将 [m_position, m_size) 区间的数据转为字符串
std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

// 将 [m_position, m_size) 区间的数据转为十六进制字符串（格式 FF FF FF）
std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

// 未指定位置时使用当前游标
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers
                                ,uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}