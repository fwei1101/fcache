#pragma once

#include <vector>
#include <memory>
#include <string>

#include "FCachePolicy.h"

struct CachesTestBox
{
    // TestTools
    std::vector<std::unique_ptr<FreddyCache::FCachePolicy<int, std::string>>> caches;
    std::vector<int> hit_counts;
    std::vector<int> get_counts;
    std::vector<std::string> cache_names;
    std::vector<double> average_operation_time;
};

CachesTestBox initCachesTestBox(int capacity, int k, int threshold, int granularity);