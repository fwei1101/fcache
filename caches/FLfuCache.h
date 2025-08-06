#pragma once

#include "FCachePolicy.h"

#include <memory>
#include <unordered_map>
#include <map>
#include <mutex>
#include <iostream>

namespace FreddyCache
{

template <typename Key, typename Value>
class LfuNode : public Node<Key, Value>
{
private:
    size_t accessCount_;
public:
    std::weak_ptr<LfuNode<Key, Value>> prev_;
    std::shared_ptr<LfuNode<Key, Value>> next_;

public:
    LfuNode(Key key, Value value)
        : Node<Key, Value>(key, value)
        , accessCount_(1)
    {}

    size_t getAccessCount() const
    {
        return accessCount_;
    }

    void setAccessCount(size_t count)
    {
        accessCount_ = count;
    }

};

template <typename Key, typename Value>
class LfuNodeList
{
private:
    using Node = LfuNode<Key, Value>;
    using NodePtr = std::shared_ptr<Node>;

    NodePtr dummyHead_;
    NodePtr dummyTail_;
    size_t  size_;

public:
    LfuNodeList()
        : size_(0)
    {
        initializeList();
    }

    ~LfuNodeList() = default;

    bool isEmpty() const
    {
        return dummyHead_->next_ == dummyTail_;
    }

    void insert(NodePtr node)
    {
        node->next_ = dummyTail_;
        node->prev_ = dummyTail_->prev_;
        dummyTail_->prev_.lock()->next_ = node;
        dummyTail_->prev_ = node;
        ++size_;
    }

    void remove(NodePtr node)
    {
        if (!node->prev_.expired() && node->next_)
        {
            auto prev = node->prev_.lock();
            prev->next_ = node->next_;
            node->next_->prev_ = prev;
            node->next_ = nullptr;
            --size_;
        }
    }

    NodePtr getLeastRecent() const
    {
        return dummyHead_->next_;
    }

    size_t getSize() const
    {
        return size_;
    }

private:
    void initializeList()
    {
        dummyHead_ = std::make_shared<Node>(Key(), Value());
        dummyTail_ = std::make_shared<Node>(Key(), Value());
        dummyHead_->next_ = dummyTail_;
        dummyTail_->prev_ = dummyHead_;
    }
};

template <typename Key, typename Value>
class FLfuCache : public FCachePolicy<Key, Value>
{
private:
    using Node = LfuNode<Key, Value>;
    using NodePtr = std::shared_ptr<Node>;
    using NodeList = LfuNodeList<Key, Value>;
    using NodeListPtr = std::shared_ptr<NodeList>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
    using NodeListMap = std::map<size_t, NodeListPtr>;

    size_t capacity_;
    size_t revolvingThreshold_;
    size_t granularity_;
    NodeMap nodeMap_;
    NodeListMap nodeListMap_;
    std::mutex mutex_;

public:
    FLfuCache(size_t capacity, size_t revolvingThreshold, size_t granularity)
        : capacity_(capacity)
        , granularity_(granularity)
        , revolvingThreshold_(revolvingThreshold)
    {}

    ~FLfuCache() override = default;

    void put(Key key, Value value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        revolveIfNeeded();
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            it->second->setValue(value);
            incrementAccessCount(it->second);
            return;
        }

        if (nodeMap_.size() == capacity_)
        {
            evictLeastFrequent();
        }
        addNewNode(key, value);
    }

    bool get(Key key, Value & value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        revolveIfNeeded();
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            value = it->second->getValue();
            incrementAccessCount(it->second);
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

private:
    void incrementAccessCount(NodePtr node)
    {
        size_t accessCount = node->getAccessCount();
        size_t oldLevel = accessCount / granularity_;
        if (accessCount < revolvingThreshold_)
        {
            accessCount++;
            node->setAccessCount(accessCount);
        }
        size_t newLevel = accessCount / granularity_;

        NodeListPtr oldNodeList = nodeListMap_[oldLevel];
        oldNodeList->remove(node);
        if (oldLevel != newLevel)
        {
            if (oldNodeList->isEmpty())
            {
                nodeListMap_.erase(oldLevel);
            }

            if (nodeListMap_.find(newLevel) == nodeListMap_.end())
            {
                nodeListMap_.emplace(newLevel, std::make_shared<NodeList>());
            }
        }
        nodeListMap_[newLevel]->insert(node);

    }

    void evictLeastFrequent()
    {
        NodeListPtr nodeList = nodeListMap_.begin()->second;
        NodePtr node = nodeList->getLeastRecent();
        nodeList->remove(node);
        nodeMap_.erase(node->getKey());
        if (nodeList->isEmpty())
        {
            nodeListMap_.erase(nodeListMap_.begin());
        }
    }

    void addNewNode(const Key & key, const Value & value)
    {
        NodePtr node = std::make_shared<Node>(key, value);
        size_t accessCount = node->getAccessCount();
        size_t level = accessCount / granularity_;
        if (nodeListMap_.find(level) == nodeListMap_.end())
        {
            nodeListMap_.emplace(level, std::make_shared<NodeList>());
        }
        nodeListMap_[level]->insert(node);
        nodeMap_[key] = node;
    }

    void revolveIfNeeded()
    {
        size_t thresholdingLevel = revolvingThreshold_ / granularity_;
        auto it = nodeListMap_.find(thresholdingLevel);
        if (it != nodeListMap_.end() && it->second->getSize() > capacity_ / 2)
        {
            nodeListMap_.clear();
            size_t accessCount = 1;
            size_t revolvedLevel = accessCount / granularity_;
            nodeListMap_.emplace(revolvedLevel, std::make_shared<NodeList>());
            for (auto & it : nodeMap_)
            {
                NodePtr node = it.second;
                node->setAccessCount(accessCount);
                nodeListMap_[revolvedLevel]->insert(node);
            }
        }
    }

};

}