#pragma once
#include <vector>
#include <mutex>
#include <memory>

template <typename T>
class ObjectPool {
public:
    static ObjectPool& getInstance() {
        static ObjectPool pool;
        return pool;
    }

    // 从池子获取对象
    std::shared_ptr<T> get() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pool_.empty()) {
            auto res = std::move(pool_.back());
            pool_.pop_back();
            return res;
        }
        // 如果池子空了，创建一个新的，并绑定“回收”逻辑
        return std::shared_ptr<T>(new T(), [this](T* p) {
            // 当外部引用计数归零，放回池子
            // 注意：这里需要重新包裹成 shared_ptr 放入池子
            // 工业级实现通常会更复杂，目前可以先这样简化
            std::lock_guard<std::mutex> lk(this->mutex_);
            this->pool_.push_back(std::shared_ptr<T>(p));
        });
    }

private:
    ObjectPool() = default;
    std::vector<std::shared_ptr<T>> pool_;
    std::mutex mutex_;
};