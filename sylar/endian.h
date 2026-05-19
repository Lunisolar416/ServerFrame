#ifndef __SYLAR_ENDIAN_H__
#define __SYLAR_ENDIAN_H__

#define SYLAR_LITTER_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2
#include <byteswap.h>
#include <cstdint>
#include <type_traits>
namespace mysylar
{

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T value)
{
    return (T) bswap_64((uint64_t) value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T value)
{
    return (T) bswap_32((uint32_t) value);
}
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T value)
{
    return (T) bswap_16((uint16_t) value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_LITTER_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTER_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
template <class T>
T byteswapOnLittleEndian(T t)
{
    return t;
}
template <class T>
T byteswapOnBigEndian(T t)
{
    return byteswap(t);
}
#else
template <class T>
T byteswapOnLittleEndian(T t)
{
    return byteswap(t);
}
template <class T>
T byteswapOnBigEndian(T t)
{
    return t;
}
#endif
} // namespace mysylar

#endif //__SYLAR_ENDIAN_H__