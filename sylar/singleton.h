/**
 * @file singleton.h
 * @author {lunisolar} ({1739930489@qq..com})
 * @brief 单例模式封装
 * @version 0.1
 * @date 2025-10-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace mysylar
{
namespace
{
template <class T, class X, int N>
T& GetInstanceX()
{
    static T v;
    return v;
}
template <class T, class X, int N>
std::shared_ptr<T> GetInstancePtr()
{
    static std::shared_ptr<T> v(new T);
    return v;
}
} // namespace
/**
 * @brief 单例模式封装类
 *@details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template <class T, class X = void, int N = 0>
class Singleton
{
  public:
    /**
     * @brief 返回单例裸指针
     *
     */
    static T* GetInstance()
    {
        static T v;
        return &v;
    }
};
/**
 * @brief 单例模式智能指针封装类
 *
 */
template <class T, class X = void, int N = 0>
class SingletonPtr
{
  public:
    /**
     * @brief 返回单例智能指针
     *
     */
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
} // namespace mysylar

#endif //__SYLAR_SINGLETON_H__