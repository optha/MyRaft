# MyRaft
学习Raft、RPC、跳表、协程相关demo

## Code construction
-scr
common<br>
    util - 延迟类、读写日志队列<br>
    config - 协程部分参数、时间参数<br>
  fiber<br>
    utils - 时间相关、解码、堆栈信息<br>
    fiber - 协程相关，创建、重置、挂起、恢复（resume）<br>
    thread - 线程创建、销毁<br>
    mutex - 读写锁<br>
    hook - hook系统函数<br>
    scheduler - 调度器，控制线程，线程再控制协程<br>
    singleton - 单例模式，确保一个类只有一个实例，并提供一个全局访问点来获取该实例。获取对应类型的单例对象的指针或智能指针。<br>

