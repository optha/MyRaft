#ifndef __MONSOON_SINGLETON_H__
#define __MONSOON_SINGLETON_H__

#include <memory>

namespace monsoon {
    namespace {
        // 返回一个类型为 T 的静态变量 v 的引用
        template <class T, class X, int N>
        T &GetInstanceX() {
            static T v;
            return v;
        }

        // 返回一个类型为 T 的静态智能指针 v 的引用
        template <class T, class X, int N>
        std::shared_ptr<T> GetInstancePtr()
        {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    }

    /**
     * @brief 单例模式封装类
     * @details T 要创建单例的类型
     *          X 用于创建多个实例的标签
     *          N 用于创建同一个标签的多个实例的索引
     */
    template <class T, class X = void, int N = 0>
    class Singleton
    {
    public:
        // 返回单例对象的裸指针
        static T *GetInstance()
        {
            static T v;
            return &v;
        }
    };

    /**
     * @brief 单例模式封装类
     * @details T 要创建单例的类型
     *          X 用于创建多个实例的标签
     *          N 用于创建同一个标签的多个实例的索引
     */
    template <class T, class X = void, int N = 0>
    class SingletonPtr
    {
    public:
        // 返回单例对象的智能指针
        static std::shared_ptr<T> GetInstance()
        {
            static std::shared_ptr<T> v(new T);
            return v;
            // return GetInstancePtr<T, X, N>();
        }
    };
}
#endif