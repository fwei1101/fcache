#pragma once

namespace FreddyCache
{

template <typename Key, typename Value>
class FCachePolicy
{
public:
    virtual ~FCachePolicy() {};

    // 添加缓存接口
    virtual void put(Key key, Value value) = 0;

    // 两种获取缓存接口
    virtual bool get(Key key, Value & value) = 0;
    virtual Value get(Key key) = 0;
};

template <typename Key, typename Value>
class Node
{
protected:
    Key     key_;
    Value   value_;
public:
    Node(Key key, Value value)
        : key_(key)
        , value_(value)
    {}

    // 提供必要的访问器
    Key     getKey() const { return key_; }
    Value   getValue() const { return value_; }
    void    setValue(const Value& value) { value_ = value; }

};

} // namespace FreddyCache