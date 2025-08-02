#include "cachesTestBox.h"
#include "FLruCache.h"

CachesTestBox initCachesTestBox(int capacity, int k, int slices)
{
    auto lru = std::make_unique<FreddyCache::FLruCache<int, std::string>>(capacity);
    auto lruk = std::make_unique<FreddyCache::FLruKCache<int, std::string>>(capacity, capacity, k);
    auto lruhash = std::make_unique<FreddyCache::FHashLruCache<int, std::string>>(capacity, slices);

    CachesTestBox c;
    c.caches.clear(),
    c.caches.emplace_back(std::move(lru));
    c.caches.emplace_back(std::move(lruk));
    c.caches.emplace_back(std::move(lruhash));

    auto cacheNums = c.caches.size();
    c.hit_counts = std::vector<int>(cacheNums, 0);
    c.get_counts = std::vector<int>(cacheNums, 0);
    c.cache_names = {
        "LRU",
        "LRU-K" + std::to_string(k),
        "LRU-H" + std::to_string(slices)
    };
    c.average_operation_time = std::vector<double>(cacheNums, 0);

    return c;
}