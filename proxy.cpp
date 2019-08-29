#include "argh/argh.h"
#include <thread>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <memory>
#include <boost/lockfree/queue.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <cstdio>
#include <sys/time.h>
#include <sys/resource.h>


class Object {
    size_t size;
    char* data;
public:
    Object(size_t _size) : size(_size), data(new char[size]) {}
    ~Object() {delete[] data;}
};


// from: https://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c
struct statm_t {
    unsigned long size, resident, share, text, lib, data, dt;
};

void read_off_memory_status(statm_t& result) {
  const char* statm_path = "/proc/self/statm";

  FILE* f = fopen(statm_path, "r");
  if (!f) {
    perror(statm_path);
    return;
  }
  if (7 != fscanf(f, "%ld %ld %ld %ld %ld %ld %ld",
    &result.size, &result.resident, &result.share, &result.text, &result.lib, &result.data, &result.dt)) {
    perror(statm_path);
  }
  fclose(f);
}



using ObjectQueue = boost::lockfree::queue<Object*, boost::lockfree::capacity<65530>>;
// needed, so that a q (which is non copyable) could be set inside a container
using ObjectQueuePtr = std::unique_ptr<ObjectQueue>;

class Accumulator {
    std::mutex accumulate_lock;
    unsigned total_objects_produced = 0;
    unsigned total_objects_consumed = 0;
    unsigned total_objects_failed = 0;

public:

    void producer_ended(unsigned total_objects, unsigned failed_objects) {
        std::unique_lock<std::mutex> l(accumulate_lock);
        total_objects_produced += total_objects;
        total_objects_failed += failed_objects;
    }

    void consumer_ended(unsigned total_objects) {
        std::unique_lock<std::mutex> l(accumulate_lock);
        total_objects_consumed += total_objects;
    }

    void all_ended() {
        if (total_objects_produced != total_objects_consumed) {
            std::cout << "total objects produced: " << total_objects_produced << std::endl;
            std::cout << "total objects consumed: " << total_objects_consumed << std::endl;
        }
        const auto failed_objects_pecent = 100.0*total_objects_failed/total_objects_produced;
        if (failed_objects_pecent >= 1.0) {
            std::cout << "failed to push " << total_objects_failed << " objects (" << 100.0*total_objects_failed/total_objects_produced << "%)" << std::endl;
        }
    }
};

int main(int, char** argv) {
    // input parameters
    argh::parser cmdl(argv);

    if (cmdl["h"]) {
        std::cout << "usage: " << cmdl[0] << "[--iterations=<number>|--threads=<number>|--min-size=<bytes>|--max-size=<bytes>|--overall-iterations=<number>]" << std::endl << std::endl;
        std::cout << "\titerations: the number of objects each producer is going to create [default 1M]" << std::endl;
        std::cout << "\tthreads: the number of consumer threads and number of producer threads. zero means a single thread [default 0]" << std::endl;
        std::cout << "\tmin-size: the minimum size in bytes to allocate per object [default to max-size if set, or to 1K if not]" << std::endl;
        std::cout << "\tmax-size: the maximum size in bytes to allocate per object [default to min-size if set, or to 1K if not]" << std::endl;
        std::cout << "\toverall-iterations: number of times the test is executed [default 1]" << std::endl;
        return 0;
    }

    static const unsigned DEFAULT_ITERATIONS = 1000000;
    unsigned number_of_iterations;
    cmdl("iterations", 0) >> number_of_iterations;
    if (number_of_iterations == 0) {
        number_of_iterations = DEFAULT_ITERATIONS;
    }
    unsigned number_of_threads;
    cmdl("threads", 0) >> number_of_threads;

    static const unsigned DEFAULT_OBJECT_SIZE = 1024;
    unsigned min_object_size;
    cmdl("min-size", 0) >> min_object_size;
    unsigned max_object_size;
    cmdl("max-size", 0) >> max_object_size;
    if (min_object_size == 0) {
        min_object_size = max_object_size;
    }
    if (max_object_size == 0) {
        max_object_size = min_object_size;
    }
    if (min_object_size > max_object_size) {
        std::cerr << "max object size must be greater than or equal to min object size" << std::endl;
        return -1;
    }
    if (min_object_size == 0 && max_object_size == 0) {
        min_object_size = DEFAULT_OBJECT_SIZE;
        max_object_size = DEFAULT_OBJECT_SIZE;
    }
    static const unsigned DEFAULT_OVERALL_ITERATIONS = 1;
    unsigned overall_iterations;
    cmdl("overall-iterations", 0) >> overall_iterations;
    if (overall_iterations == 0) {
        overall_iterations = DEFAULT_OVERALL_ITERATIONS;
    }

    std::vector<ObjectQueuePtr> queues;

    // creating all queues
    for (auto i = 0U; i < number_of_threads; ++i) {
        // create a lockfree queue per consumers
        queues.emplace_back(new ObjectQueue());
    }
    // create a single queue for the case that threads=0
    auto single_q = ObjectQueuePtr(new ObjectQueue()); 
   
    const auto number_of_ops = number_of_iterations*std::max(number_of_threads,1U);

    std::cout.imbue(std::locale(""));
    std::cout << std::setprecision(2);
    std::cout << std::fixed;

    for (auto oi = 0U; oi < overall_iterations; ++oi) {
        std::cout << "iteration: " << oi << " started ====================" << std::endl;
        std::atomic<bool> done{false};
        Accumulator acc;
        statm_t mem_start;
        read_off_memory_status(mem_start);

        rusage usage_start;
        auto start_user_time_ms = 0.0;
        auto start_system_time_ms = 0.0;
        if (getrusage(RUSAGE_SELF, &usage_start) == 0) {
            start_user_time_ms = usage_start.ru_utime.tv_sec*1000.0 + usage_start.ru_utime.tv_usec/1000.0;
            start_system_time_ms = usage_start.ru_stime.tv_sec*1000.0 + usage_start.ru_stime.tv_usec/1000.0;
        }
        const auto t_start = std::chrono::high_resolution_clock::now();

        if (number_of_threads == 0) {
            // special case where the same thread is consumer and producer
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(min_object_size, max_object_size);
            unsigned failed_to_push = 0;
            unsigned obj_count = 0;
            auto q = single_q.get();
            for (auto i = 0U; i != number_of_iterations; ++i) {
                // try to push until the queue has no space
                Object* obj = nullptr;
                while (!q->push(obj = new Object(dis(gen)))) {
                    // if queue is full, we empty it
                    q->consume_all([&obj_count](auto obj) {
                                delete obj;
                                ++obj_count;
                            });
                }
                // empty the last insertion to the queue
                q->consume_all([&obj_count](auto obj) {
                            delete obj;
                            ++obj_count;
                        });
            }
            acc.consumer_ended(obj_count);
            acc.producer_ended(number_of_iterations, failed_to_push);
        }

        // starting consumer threads
        std::vector<std::thread> consumers;
        for (auto i = 0U; i < number_of_threads; ++i) {
            auto q = queues[i].get();
            consumers.push_back(std::thread([&done, &acc, q]() {
                unsigned obj_count = 0;
                while (!done) {
                    // if we are not done, we keep trying even if queue is empty
                    q->consume_all([&obj_count](auto obj) {
                                delete obj;
                                ++obj_count;
                            });
                }
                q->consume_all([&obj_count](auto obj) {
                            delete obj;
                            ++obj_count;
                        });
                acc.consumer_ended(obj_count);
            }));
        }
        // starting producer threads
        std::vector<std::thread> producers;
        for (auto i = 0U; i < number_of_threads; ++i) {
            auto q = queues[i].get();
            producers.push_back(std::thread([q, &acc, number_of_iterations, min_object_size, max_object_size]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(min_object_size, max_object_size);
                unsigned failed_to_push = 0;
                for (auto i = 0U; i != number_of_iterations; ++i) {
                    // try to push until the queue has no space
                    Object* obj = nullptr;
                    while (!q->push(obj = new Object(dis(gen)))) {
                        delete obj;
                        ++failed_to_push;
                        // dont spin when the queue is full
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
                acc.producer_ended(number_of_iterations, failed_to_push);
            }));
        }

        // wait for producers to finish
        std::for_each(producers.begin(), producers.end(), [](std::thread &t) {t.join();});
        done = true;
        std::for_each(consumers.begin(), consumers.end(), [](std::thread &t) {t.join();});

        const auto t_end = std::chrono::high_resolution_clock::now();
        const double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        rusage usage_end;
        if (getrusage(RUSAGE_SELF, &usage_end) == 0) {
            std::cout << "maximum resident memory used so far: " << usage_end.ru_maxrss << " KB" << std::endl;
            const auto end_user_time_ms = usage_end.ru_utime.tv_sec*1000.0 + usage_end.ru_utime.tv_usec/1000.0;
            const auto end_system_time_ms = usage_end.ru_stime.tv_sec*1000.0 + usage_end.ru_stime.tv_usec/1000.0;
            const auto diff_user_time_ms = end_user_time_ms - start_user_time_ms;
            const auto diff_system_time_ms = end_system_time_ms - start_system_time_ms;
            std::cout << "user CPU time: " << diff_user_time_ms << " ms" << std::endl;
            std::cout << "system CPU time: " << diff_system_time_ms << " ms" << std::endl;
            const auto cpu_usage = 100.0*(diff_user_time_ms + diff_system_time_ms)/elapsed_time_ms;
            std::cout << "CPU usage: " << cpu_usage << " %" << std::endl;
            std::cout << "CPU cost per op:" << cpu_usage/number_of_ops << std::endl;
        }
        std::cout << "elapsed time: " << elapsed_time_ms << " ms" << std::endl;
        std::cout << "ops per second: " << number_of_ops/elapsed_time_ms << std::endl;
        statm_t mem_end;
        read_off_memory_status(mem_end);
        if (mem_end.resident > mem_start.resident) {
            std::cout << "resident memory 'leaked' in this iteration: " << (mem_end.resident - mem_start.resident)/1000.0 << " KB" << std::endl;
        }
        acc.all_ended();
        

        std::cout << "iteration: " << oi << " ended ====================" << std::endl;
    }

    return 0;
}

