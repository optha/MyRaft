#ifndef UTIL_H
#define UTIL_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <sstream>
#include <functional>
#include <random>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "config.h"

/**
 * 用于实现 延迟执行某个函数 的功能
 * 将某个函数的执行推迟到将来的某个时间点，通常是在当前作用域结束时执行。
 * 这种技术常用于资源管理、清理操作或者需要延迟执行的逻辑。
 * 通过延迟函数的方式，可以确保在特定的时机执行必要的清理或操作，
 * 提高代码的可维护性和可读性，同时避免忘记执行某些必要的操作。
*/
template <class F>
class DeferClass {
public:
    /** 接受右值引用 —— 临时对象或将要被移动的对象
     * 通过forward<F>(f)将f转发给成员变量m_func
     * 从而实现对传入函数的移动语义，避免不必要的拷贝，
     * 确保在对象销毁时能正确执行延迟函数
    */
    DeferClass(F&& f) : m_func(std::forward<F>(f)) {}

    // 接受常量引用
    DeferClass(const F& f) : m_func(f) {}
    ~DeferClass() { m_func(); }

    // 禁用拷贝构造函数和拷贝复制运算符
    // 确保对象不会被复制，从而避免多次执行延迟函数
    DeferClass(const DeferClass &e) = delete;
    DeferClass &operator=(const DeferClass &e) = delete;

private:
    // 保存要延迟执行的函数对象
    F m_func;
};

/**
 * 这段代码是使用宏定义来实现延迟执行的功能。
 * 首先，通过宏定义_CONCAT(a, b)将两个参数a和b连接在一起，
 * 然后定义了宏_MAKE_DEFER_(line)，
 * 在这个宏中创建了一个DeferClass类型的对象，
 * 对象名是通过_CONCAT宏生成的，
 * 对象的初始化采用了lambda表达式，实现了延迟执行的效果。
*/
#define _CONCAT(a, b) a##b
#define _MAKE_DEFER_(line) DeferClass _CONCAT (defer_placeholder, line) = [&]()

/** 接着，使用#undef DEFER取消之前对DEFER宏的定义，
 * 然后重新定义DEFER宏为_MAKE_DEFER_(__LINE__)，
 * 其中__LINE__是一个预定义的宏，表示当前行号，
 * 这样每次使用DEFER宏时都会生成一个唯一的对象名，实现了延迟执行的功能。
*/
#undef DEFER
#define DEFER _MAKE_DEFER_(__LINE__)

void DPrintf(const char *format, ...);
void myAssert(bool condition, std::string message = "Assertion failed!");

// 整体实现了将多个参数按照指定格式组合成一个字符串的功能。
template <typename... Args>
std::string format(const char* format_str, Args... args) {
    std::stringstream ss;
    // 将参数args按顺序写入到ss中
    int _[] = {((ss << args), 0)...};
    (void)_;    // 忽略表达式(ss << args)的返回值
    // 最后将ss中的内容转换为std::string并返回
    return ss.str();
}

// 获取当前时间
std::chrono::_V2::system_clock::time_point now();
// 获取随机选举超时时间
std::chrono::milliseconds getRandomizedElectionTimeout();

void sleepNMilliseconds(int N);

// 异步写日志的日志队列
// lock_guard 和 unique_lock 的区别
template <typename T>
class LockQueue {
public:
    // 线程写日志
    void Push(const T& data) {
        // lock_guard管理互斥锁的加锁和解锁操作。
        // 它的主要作用是在构造函数中获取一个互斥锁，然后在析构函数中自动释放该锁，以确保在锁保护区域的结束时正确解锁。
        // RAII的思想保证锁正确释放
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_convariable.notify_one();
    }

    // 线程读日志
    T Pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty()) {
            // 日志队列为空，线程阻塞
            m_convariable.wait(lock);
        }
        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

    // 添加一个超时时间参数，默认为 50 毫秒
    bool timeOutPop(int timeout, T* ResData) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 获取当前时间，并计算超时时间
        auto now = std::chrono::system_clock::now();
        auto timeout_time = now + std::chrono::milliseconds(timeout);

        // 超时之前不断检查队列是否为空
        while (m_queue.empty()) {
            // 如果超时，返回一个空对象
            if (m_convariable.wait_until(lock, timeout_time) == std::cv_status::timeout)
                return false;
            else continue;
        }

        T data = m_queue.front();
        m_queue.pop();
        *ResData = data;
        return true;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_convariable;
};

class OperaionFromRaft {
public:
    // "Get" "Put" "Append"
    std::string Operation;
    std::string Key;
    std::string Value;
    std::string ClientId;
    // 客户端号码请求的Request的序列号，保证线程一致性
    int RequestId;

public:
    // 将当前对象序列化为一个字符串形式的表示
    std::string asString() const {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        oa << *this;
        return ss.str();
    }

    bool parseFromString(std::string str) {
        std::stringstream iss(str);
        boost::archive::text_iarchive ia(iss);
        ia >> *this;
        return true;
    }

    friend std::ostream &operator<<(std::ostream &os, const OperaionFromRaft &obj) {
        os << "[MyClass: Operation{" + obj.Operation + "}, Key{" + obj.Key + "}, Value{" + obj.Value + "}, ClientId{" + obj.ClientId + "}, RequestId{" + std::to_string(obj.RequestId) + "}]";
        return os;
    }

private:
    // 使用Boost库进行序列化以访问当前类的私有成员
    // 这样可以确保Boost序列化库能够正确地对当前类进行序列化和反序列化操作。
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar &Operation;
        ar &Key;
        ar &Value;
        ar &ClientId;
        ar &RequestId;
    }
};

const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";
const std::string ErrWrongLeader = "ErrWrongLeader";

bool isReleasePort(unsigned short usPort);
bool getReleasePort(short &port);

#endif