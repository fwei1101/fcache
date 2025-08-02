#include <chrono>
#include <string>
#include <vector>

class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
public:
    Timer()
        : start_(std::chrono::high_resolution_clock::now())
    {}

    double elapsed()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - start_).count();
    }
};

void printResults(
    const std::string & testName,
    int capacity,
    const std::vector<std::string> & cache_names,
    const std::vector<int> & get_counts,
    const std::vector<int> & hit_counts,
    const std::vector<double> & average_operate_time
);