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

} // namespace FreddyCache