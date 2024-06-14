#ifndef __MONSOON_FIBER_H__
#define __MONSOON_FIBER_H__

#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <functional>
#include <iostream>
#include <memory>
#include "utils.hpp"

namespace monsoon
{
    // 协程（纤程）
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        /**
         * enable_shared_from_this 是一个模板类，
         * 用于解决在对象自身被 shared_ptr 管理时，
         * 获取一个指向自身的 shared_ptr 的问题。
         * 通常情况下，当一个类需要在某些地方保存自身的 shared_ptr，
         * 而又希望避免出现循环引用导致内存泄漏的情况时，
         * 可以继承自 enable_shared_from_this。
         * 通过调用 shared_from_this() 成员函数，
         * 可以获取一个指向当前对象的 shared_ptr，
         * 而不会导致额外的引用计数增加，从而避免循环引用问题。
         */
    public:
        typedef std::shared_ptr<Fiber> ptr;
        // Fiber状态机
        enum State
        {
            // 就绪态，协程yield后的状态
            READY,
            // 运行态，resume后的状态
            RUNNING,
            // 结束态，回调函数执行完之后的状态
            TERM,
        };

    private:
        // 初始化，构造主协程
        Fiber();

    public:
        // 构造子协程
        Fiber(std::function<void()> cb, size_t stackSz = 0, bool run_in_scheduler = true);
        ~Fiber();
        // 重置协程状态，复用栈空间
        void reset(std::function<void()> cb);
        // 协程切换到运行态
        void resume();
        // 让出执行权
        void yield();

        // 获取协程ID
        uint64_t getId() const { return id_; }
        // 获取协程状态
        State getState() const { return state_; }

        // 设置当前正在运行的协程
        static void SetThis(Fiber *f);

        /**
         * 获取当前线程中的执行线程
         * 如果当前线程没有创建协程，则创建第一个协程，即为主协程
         * 其他协程通过该协程来调度
        */
        static Fiber::ptr GetThis();
        // 协程总数
        static uint64_t TotalFiberNum();
        // 协程回调函数
        static void MainFunc();
        // 获取当前协程ID
        static uint64_t GetCurFiberID();

    private:
        // 协程ID
        uint64_t id_ = 0;
        // 协程栈大小
        uint32_t stackSize_ = 0;
        // 协程状态
        State state_ = READY;
        // 协程上下文
        ucontext_t ctx_;
        // 协程栈地质
        void *stack_ptr = nullptr;
        // 协程回调函数
        std::function<void()> cb_;
        // 是否参与调度器调度
        bool isRunInScheduler_;
    };
}

#endif
