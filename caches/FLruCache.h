#pragma once

#include "FCachePolicy.h"

#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <cmath>

namespace FreddyCache
{

// 前向声明
template <typename Key, typename Value> class FLruCache;

template <typename Key, typename Value>
class LruNode
{
private:
    Key     key_;
    Value   value_;
    size_t  accessCount_;
    std::weak_ptr<LruNode<Key, Value>> prev_;
    std::shared_ptr<LruNode<Key, Value>> next_;
public:
    LruNode(Key key, Value value)
        : key_(key)
        , value_(value)
        , accessCount_(1)
    {}

    // 提供必要的访问器
    Key     getKey() const { return key_; }
    Value   getValue() const { return value_; }
    void    setValue(const Value& value) { value_ = value; }
    size_t  getAccessCount() const { return accessCount_; }
    void    incrementAccessCount() { ++accessCount_; }

    friend class FLruCache<Key, Value>;
};

template <typename Key, typename Value>
class FLruCache : public FCachePolicy<Key, Value>
{
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
private:
    int     capacity_;
    NodeMap nodeMap_;
    std::mutex mutex_;
    NodePtr dummyHead_;
    NodePtr dummyTail_;
public:
    FLruCache(int capacity)
        : capacity_(capacity)
    {
        initializeList();
    }

    ~FLruCache() override = default;

    void put(Key key, Value value) override
    {
        if (capacity_ <= 0)
            return;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            updateExistingNode(it->second, value);
            return;
        }

        addNewNode(key, value);
    }

    bool get(Key key, Value & value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            moveToMostRecent(it->second);
            value = it->second->value_;
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            removeNode(it->second);
            nodeMap_.erase(it);
        }
    }

private:
    void initializeList()
    {
        dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyHead_->next_ = dummyTail_;
        dummyTail_->prev_ = dummyHead_;
    }

    void insertNode(NodePtr node)
    {
        node->next_ = dummyTail_;
        node->prev_ = dummyTail_->prev_;
        dummyTail_->prev_.lock()->next_ = node; // 使用 lock() 将 weak_ptr 升为 shared_ptr 以便访问
        dummyTail_->prev_ = node;
    }

    void removeNode(NodePtr node)
    {
        if (!node->prev_.expired() && node->next_)
        {
            auto prev = node->prev_.lock();
            prev->next_ = node->next_;
            node->next_->prev_ = prev;
            node->next_ = nullptr; // 清空 next_ 指针，彻底断开节点与链表的连接
        }
    }

    void moveToMostRecent(NodePtr node)
    {
        removeNode(node);
        insertNode(node);
    }

    void evictLeastRecent()
    {
        NodePtr leastRecent = dummyHead_->next_;
        removeNode(leastRecent);
        nodeMap_.erase(leastRecent->getKey());
    }

    void addNewNode(const Key & key, const Value & value)
    {
        if (nodeMap_.size() >= capacity_)
            evictLeastRecent();

        NodePtr newNode = std::make_shared<LruNodeType>(key, value);
        insertNode(newNode);
        nodeMap_[key] = newNode;
    }

    void updateExistingNode(NodePtr node, const Value & value)
    {
        node->setValue(value);
        moveToMostRecent(node);
    }
};

// LRU-k
template <typename Key, typename Value>
class FLruKCache : public FLruCache<Key, Value>
{
private:
    int     k_;
    std::unique_ptr<FLruCache<Key, size_t>> accessCountList_; // 累计访问计数器
    std::unordered_map<Key, Value> pendingValueMap_; // 待定区数据存放区
private:
    // 禁用基类方法操作主缓存数据
    using FLruCache<Key, Value>::get;
    using FLruCache<Key, Value>::put;
public:
    FLruKCache(int capacity, int accessCountCapacity, int k)
        : FLruCache<Key, Value>(capacity)
        , accessCountList_(std::make_unique<FLruCache<Key, size_t>>(accessCountCapacity))
        , k_(k)
    {}

    bool get(Key key, Value & value)
    {
        // 首先尝试从主缓存获取数据
        bool inMainCache = FLruCache<Key, Value>::get(key, value);
        if (inMainCache)
            return true;

        // 尝试从等待区获取数据
        size_t accessCount;
        bool inPendingList = accessCountList_->get(key, accessCount);
        if (inPendingList)
        {
            accessCount++;
            auto it = pendingValueMap_.find(key);
            value = it->second;
            if (accessCount >= k_)
            {
                // 将数据从等待区移动到主缓存
                moveFromPendingToMainCache(key, value);
            }
            return true;
        }

        // 缓存中不存在该数据
        pendingValueMap_.erase(key);
        return false;
    }

    void put(Key key, Value value)
    {
        // 首先尝试将数据放入主缓存
        bool inMainCache = FLruCache<Key, Value>::get(key, value);
        if (inMainCache)
        {
            FLruCache<Key, Value>::put(key, value);
            return;
        }

        // 操作等待区
        size_t accessCount = accessCountList_->get(key);
        accessCount++;
        // 该数据已达标
        if (accessCount >= k_)
        {
            // 将数据从等待区移动到主缓存
            moveFromPendingToMainCache(key, value);
            return;
        }
        // 数据未达标
        accessCountList_->put(key, accessCount);
        pendingValueMap_[key] = value;

    }

private:
    void moveFromPendingToMainCache(const Key & key, const Value & value)
    {
        pendingValueMap_.erase(key);
        accessCountList_->remove(key);
        FLruCache<Key, Value>::put(key, value);
    }
};

// Hash-LRU
template<typename Key, typename Value>
class FHashLruCache : public FCachePolicy<Key, Value>
{
private:
    size_t  capacity_;
    int     sliceNum_;
    std::vector<std::unique_ptr<FLruCache<Key, Value>>> lruSliceCaches_;
public:
    FHashLruCache(size_t capacity, int sliceNum)
        : capacity_(capacity)
        , sliceNum_(sliceNum)
        {
            size_t sliceCapacity = std::ceil(capacity / static_cast<double>(sliceNum_));
            for (int i = 0; i < sliceNum_; ++i)
            {
                lruSliceCaches_.emplace_back(std::make_unique<FLruCache<Key, Value>>(sliceCapacity));
            }
        }

    bool get(Key key, Value & value) override
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lruSliceCaches_[sliceIndex]->get(key, value);
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void put(Key key, Value value) override
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lruSliceCaches_[sliceIndex]->put(key, value);
    }

private:
        size_t Hash(Key key)
        {
            std::hash<Key> hashFunc;
            return hashFunc(key);
        }
};

} // namespace FreddyCache