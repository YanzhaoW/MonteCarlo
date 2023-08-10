#pragma once

#include "DistributionGen.hpp"
#include "MultiNomial.hpp"
#include "UniformInserter.hpp"
#include "traits.hpp"
#include <fmt/core.h>
#include <future>
#include <vector>

template <typename T>
concept Loopable = requires(T loopOp) {
    loopOp.Init();
    loopOp.Finish();
};

static std::mutex PRINT_MUTEX; // NOLINT
inline void Print(std::string_view str)
{
    auto lock = std::scoped_lock{ PRINT_MUTEX };
    std::cout << str << "\n";
}

static auto Divide_into(unsigned int totalSize, unsigned int num_of_threads)
{
    assert(totalSize >= num_of_threads);
    const auto divide = totalSize / num_of_threads;
    auto vec = std::vector<unsigned int>(num_of_threads, divide);

    auto res = totalSize - divide * num_of_threads;
    for (size_t index{}; index < res; ++index)
    {
        ++vec[index];
    }

    return vec;
}

template <int binSize>
    requires OddLargeNumber<binSize>
class FineTimeMC
{
  public:
    FineTimeMC() = default;
    explicit FineTimeMC(unsigned int num)
        : totalSamples_{ num }
    {
        multinomial_.SetEntrySize(num);
    }

    void SetLoopSize(unsigned int loopSize)
    {
        loopSize_ = loopSize;
        multinomial_.SetLoopSize(loopSize);
    }

    void SetThreadsNum(unsigned int num)
    {
        threads_num_ = num;
    }

    auto GetOperator() -> auto&
    {
        return uniformInserter_;
    }

    [[nodiscard]] auto RunWithFixDistribution(const auto& midProb)
    {
        std::vector<double> distribution;
        auto inserter = UniformInserter{ totalSamples_, "fix distribution" };
        const auto probs = dis_generator_(distribution, midProb);
        multinomial_.Loop_on(distribution, inserter);
        return std::make_tuple(std::move(inserter), GetCenterBoundary(distribution, totalSamples_));
    }

    void Run_all_probs(auto& writer)
    {
        if (sample_size_ == 0)
        {
            throw std::logic_error("Epoch size is 0!");
        }

        auto threads = Divide_into(sample_size_, threads_num_);

        for (const auto& thread_loopN : threads)
        {
            auto future = std::async(
                std::launch::async,
                [this, &writer](unsigned int loopN)
                {
                    // std::cout << loopN << "\n";
                    std::string histname = fmt::format("hist_{}", threads_count_++);
                    auto inserter = UniformInserter{ totalSamples_, histname };
                    Epoch(loopN, inserter, writer);
                    Add_inserter(std::move(inserter));
                },
                thread_loopN);
            thread_results_.emplace_back(std::move(future));
        }

        for (auto& result : thread_results_)
        {
            result.get();
        }
    }

    void Epoch(int loopN, auto& inserter, auto& writer)
    {
        thread_local std::vector<double> distribution;
        for (size_t counter{}; counter < loopN; ++counter)
        {
            inserter.Init();
            const auto [pre, mid, post] = dis_generator_(distribution);
            multinomial_.Loop_on(distribution, inserter);

            Result epoch_result;
            epoch_result.stat = inserter.Finish();
            epoch_result.mid_prob = mid;
            epoch_result.pre_prob = pre;
            epoch_result.post_prob = post;

            RecordResult(epoch_result, writer);
        }
    }

    void SetEpochSize(unsigned int size)
    {
        sample_size_ = size;
    }

  private:
    unsigned int totalSamples_ = 0;
    unsigned int loopSize_ = 0;
    unsigned int sample_size_ = 1;
    unsigned int threads_num_ = 1;
    std::atomic<int> threads_count_ = 0;
    DisGenerator<binSize> dis_generator_;
    MultiNomial multinomial_{};
    std::vector<UniformInserter> uniformInserter_{};
    std::vector<std::future<void>> thread_results_;
    std::mutex mu_recorder_;

    void RecordResult(auto& result, auto& writer)
    {
        auto lock = std::scoped_lock{ mu_recorder_ };
        writer.add_row(result.pre_prob, result.mid_prob, result.post_prob, result.stat.mean, result.stat.err);
    }

    void Add_inserter(UniformInserter inserter)
    {
        auto lock = std::scoped_lock{ mu_recorder_ };
        uniformInserter_.emplace_back(std::move(inserter));
    }
};