#include <iostream>
#include <random>
#include <vector>
#include <string>

#include "cachesTestBox.h"
#include "testUtils.h"

void testHotDataAccess()
{
    std::cout << "\n=== 测试场景: 热点数据访问测试 ===" << std::endl;

    const int CAPACITY = 20;
    const int K = 3;
    const int SLICES = 4;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    // 装载缓存模型
    auto ctb = initCachesTestBox(CAPACITY, K, SLICES);
    auto & caches = ctb.caches;
    auto & hit_counts = ctb.hit_counts;
    auto & get_counts = ctb.get_counts;
    auto & cache_names = ctb.cache_names;
    auto & average_operation_time = ctb.average_operation_time;

    // 初始化随机数种子
    std::random_device rd;
    std::mt19937 gen(rd());

    // 测试缓存
    for (size_t i = 0; i < caches.size(); ++i)
    {
        // 预热
        for (int k = 0; k < HOT_KEYS; ++k)
        {
            std::string v = "value" + std::to_string(k);
            caches[i]->put(k, v);
        }

        // 交替 put 与 get
        for (int op = 0; op < OPERATIONS; ++op) {
            // 大多数缓存系统中读多于写，故设置30%写概率
            bool isPut = (gen() % 100 < 30);
            // 设置70%概率访问热点数据
            bool isHot = (gen() % 100 < 70);

            // 生成 key
            int k = isHot ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);

            // 生成操作
            if (isPut)
            {
                std::string v = "value" + std::to_string(k) + "_v" + std::to_string(op % 100);
                Timer t;
                caches[i] -> put(k, v);
                double elapsed = t.elapsed();
                average_operation_time[i] += elapsed;
            }
            else
            {
                std::string res;
                get_counts[i]++;
                Timer t;
                bool isHit = caches[i]->get(k, res);
                double elapsed = t.elapsed();
                average_operation_time[i] += elapsed;
                if (isHit)
                    hit_counts[i]++;
            }

        }

        average_operation_time[i] /= OPERATIONS;
    }

    // 输出结果
    printResults("热点数据访问测试", CAPACITY, cache_names, get_counts, hit_counts, average_operation_time);
}