// en_dian.h —— 字节序处理：大小端检测与字节序转换工具。
#pragma once
#include<endian.h>
#include<byteswap.h>
#include <type_traits>
#include<stdint.h>
#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2
namespace mo1al
{
// 对 64/32/16 位整数做字节序翻转（按类型大小分发到对应的 bswap 指令）
template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint64_t),T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);
    }
template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint32_t),T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }
template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint16_t),T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);
    }

// 依据系统字节序定义 SYLAR_BYTE_ORDER
#if BYTE_ORDER==BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define  SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
// 当前系统为大端序
// 仅在小端机器上翻转字节序（大端机器原样返回），用于主机序与网络序之间的转换
template<class T>
T byteswapOnLittleEndian(T t)
{
    return t;
}
// 仅在大端机器上翻转字节序
template<class T>
T byteswapOnBigEndian(T t)
{
    return byteswap(t);
}
#else
// 当前系统为小端序
// 仅在小端机器上翻转字节序（小端机器原样返回），用于主机序与网络序之间的转换
template<class T>
T byteswapOnLittleEndian(T t)
{
    return byteswap(t);
}
// 仅在大端机器上翻转字节序
template<class T>
T byteswapOnBigEndian(T t)
{
    return t;
}

#endif
}