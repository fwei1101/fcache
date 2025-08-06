#include <iostream>
#include <random>
#include <vector>
#include <string>

#include "cachesTestBox.h"
#include "testUtils.h"

void testLoopPattern()
{
    std::cout << "\n=== 测试场景: 循环扫描测试 ===" << std::endl;

    const int CAPACITY = 20;
    const int K = 3;
    const int THRESHOLD = 100;
    const int GRANULARITY = 10;
    const int OPERATIONS = 200000;
    const int LOOP_SIZE = 500;

    // 装载缓存模型
    auto ctb = initCachesTestBox(CAPACITY, K, THRESHOLD, GRANULARITY);
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
        for (int k = 0; k < LOOP_SIZE / 5; ++k)
        {
            std::string v = "value" + std::to_string(k);
            caches[i]->put(k, v);
        }

        // 设置循环扫描当前位置
        int current_pos = 0;

        // 交替 put 与 get
        for (int op = 0; op < OPERATIONS; ++op) {
            // 设置20%写概率
            bool isPut = (gen() % 100 < 20);

            // 生成 key
            int k;
            if (op % 100 < 60) // 60% 顺序扫描概率
            {
                k = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            }
            else if (op % 100 < 90) // 30% 随机跳跃概率
            {
                k = gen() % LOOP_SIZE;
            } else // 10% 外围访问概率
            {
                k = LOOP_SIZE + (gen() % LOOP_SIZE);
            }

            // 生成操作
            if (isPut)
            {
                std::string v = "loop" + std::to_string(k) + "_v" + std::to_string(op % 100);
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
    printResults("循环扫描测试", CAPACITY, cache_names, get_counts, hit_counts, average_operation_time);
}