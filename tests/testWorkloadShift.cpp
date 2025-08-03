#include <iostream>
#include <random>
#include <vector>
#include <string>

#include "cachesTestBox.h"
#include "testUtils.h"

void testWorkloadShift()
{
    std::cout << "\n=== 测试场景: 负载剧变测试 ===" << std::endl;

    const int CAPACITY = 20;
    const int K = 3;
    const int THRESHOLD = 10000;
    const int OPERATIONS = 200000;
    const int PAHSE_LENGTH = OPERATIONS / 5;

    // 装载缓存模型
    auto ctb = initCachesTestBox(CAPACITY, K, THRESHOLD);
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
        for (int k = 0; k < 30; ++k)
        {
            std::string v = "init" + std::to_string(k);
            caches[i]->put(k, v);
        }

        // 交替 put 与 get
        for (int op = 0; op < OPERATIONS; ++op) {
            // 计算当前阶段
            int phase = op / PAHSE_LENGTH;

            // 设置各阶段写概率与 key 分布
            int putProbability, k;
            switch (phase)
            {
                case 0:
                    putProbability = 15;
                    k = gen() % 5;
                    break; // 阶段1：热点访问，15%写概率, 热点范围 5
                case 1:
                    putProbability = 30;
                    k = gen() % 400;
                    break; // 阶段2： 大范围随机，30%写概率，热点范围 400
                case 2:
                    putProbability = 10;
                    k = (op - phase * PAHSE_LENGTH) % 100;
                    break; // 阶段3： 顺序扫描，10%写概率，100 个键
                case 3:
                {
                    putProbability = 25;
                    int locality = (op / 800) % 5;
                    k = locality * 15 + (gen() % 15);
                    break; // 阶段4： 局部随机，25%写概率，产生 5 个局部区域，每个区域 15 个键
                }
                case 4:
                {
                    putProbability = 20;
                    int r = gen() % 100;
                    if (r < 40)
                        k = gen() % 5;
                    else if (r < 70)
                        k = 5 + (gen() % 45);
                    else
                        k = 50 + (gen() % 350);
                    break; // 阶段5： 混合访问，20%写概率，概率决定热点范围
                }
            }
            bool isPut = (gen() % 100 < putProbability);

            // 生成操作
            if (isPut)
            {
                std::string v = "value" + std::to_string(k) + "_p" + std::to_string(phase);
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
    printResults("负载剧变测试", CAPACITY, cache_names, get_counts, hit_counts, average_operation_time);
}