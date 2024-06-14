#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace monsoon {
    // 防止对象被复制
    class Nonecopyable {
    public:
        Nonecopyable() = default;
        ~Nonecopyable() = default;
        Nonecopyable(const Nonecopyable &) = delete;
        Nonecopyable operator=(const Nonecopyable) = delete;
    };
}

#endif