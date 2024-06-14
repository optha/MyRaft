#ifndef CONFIG_H
#define CONFIG_H

const bool Debug = true;
// 系数
const int debugMul = 1;
// 心跳时间一般要比选举时间少一个数量级
// 如果反之，则可能会在选举时间到达时还没有选出领导者
const int HeartBeatTimeout = 25 * debugMul;

const int ApplyInterval = 10 * debugMul;

const int minRandomizedElectionTime = 300 * debugMul;
const int maxRandomizedElectionTime = 500 * debugMul;

const int CONSENSUS_TIMEOUT = 500 * debugMul;

// 协程库中线程池大小
const int FIBER_THREAD_NUM = 1;
// 是否使用caller_threaִd执行调度任务
const bool FIBER_USE_CALLER_THREAD = false;

#endif