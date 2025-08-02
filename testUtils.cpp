#include <iostream>
#include <iomanip>

#include "testUtils.h"

void printResults(
    const std::string & testName,
    int capacity,
    const std::vector<std::string> & cache_names,
    const std::vector<int> & get_counts,
    const std::vector<int> & hit_counts,
    const std::vector<double> & average_operation_time
)
{
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;

    for (size_t i = 0; i < hit_counts.size(); ++i)
    {
        double hitRate = 100.0 * hit_counts[i] / get_counts[i];
        std::cout   << cache_names[i]
                    << "\t- 命中率: "
                    << std::fixed << std::setprecision(2) << hitRate << "% "
                    << "(" << hit_counts[i] << "/" << get_counts[i] << ")"
                    << "\t- 平均操作时: "
                    << std::fixed << std::setprecision(2) << average_operation_time[i] << " μs"
                    << std::endl;
    }
}