#include "cachesTestBox.h"
#include "FLruCache.h"
#include "FLfuCache.h"

CachesTestBox initCachesTestBox(int capacity, int k, int threshold, int granularity)
{
    auto lru = std::make_unique<FreddyCache::FLruCache<int, std::string>>(capacity);
    auto lruk = std::make_unique<FreddyCache::FLruKCache<int, std::string>>(capacity, capacity, k);
    auto lfu = std::make_unique<FreddyCache::FLfuCache<int, std::string>>(capacity, threshold, granularity);

    CachesTestBox c;
    c.caches.clear(),
    c.caches.emplace_back(std::move(lru));
    c.caches.emplace_back(std::move(lruk));
    c.caches.emplace_back(std::move(lfu));

    auto cacheNums = c.caches.size();
    c.hit_counts = std::vector<int>(cacheNums, 0);
    c.get_counts = std::vector<int>(cacheNums, 0);
    c.cache_names = {
        "LRU",
        "LRU-K" + std::to_string(k),
        "LFU"
    };
    c.average_operation_time = std::vector<double>(cacheNums, 0);

    return c;
}