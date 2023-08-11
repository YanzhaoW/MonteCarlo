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
    }

    void SetLoopSize(unsigned int loopSize)
    {
        loopSize_ = loopSize;
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
        auto future = std::async(std::launch::async,
                                 [this, midProb]()
                                 {
                                     auto rnd = ROOT::Math::GSLRandomEngine{};
                                     rnd.Initialize();
                                     rnd.SetSeed(SEED_NUM);
                                     std::cout << "max: " << rnd.MaxInt() << "\n";
                                     std::vector<double> distribution;
                                     auto inserter = UniformInserter{ &rnd, totalSamples_, "fix distribution" };
                                     auto multinomial = MultiNomial(&rnd);
                                     multinomial.SetEntrySize(totalSamples_);
                                     multinomial.SetLoopSize(loopSize_);
                                     const auto probs = dis_generator_(distribution, midProb);
                                     multinomial.Loop_on(distribution, inserter);
                                     auto [pre, post] = GetCenterBoundary(distribution, totalSamples_);
                                     inserter.DrawAll({ pre, post });
                                 });
        return future;
    }

    [[nodiscard]] auto Run_all_probs(auto& writer, double max = 1.)
    {
        std::vector<std::future<void>> thread_results;
        thread_results.reserve(threads_num_);

        if (sample_size_ == 0)
        {
            return thread_results;
        }

        auto threads = Divide_into(sample_size_, threads_num_);

        for (const auto& thread_loopN : threads)
        {
            auto future = std::async(
                std::launch::async,
                [this, &writer, max](unsigned int loopN)
                {
                    // std::cout << loopN << "\n";
                    auto rnd = ROOT::Math::GSLRandomEngine{};
                    rnd.Initialize();
                    rnd.SetSeed(SEED_NUM);
                    std::string histname = fmt::format("hist_{}", threads_count_++);
                    auto inserter = UniformInserter{ &rnd, totalSamples_, histname };
                    Epoch(loopN, inserter, writer, max);
                    Add_inserter(std::move(inserter));
                },
                thread_loopN);
            thread_results.emplace_back(std::move(future));
        }
        return thread_results;
    }

    void Epoch(int loopN, auto& inserter, auto& writer, double max)
    {
        auto multinomial = MultiNomial(inserter.GetEngine());
        multinomial.SetEntrySize(totalSamples_);
        multinomial.SetLoopSize(loopSize_);
        std::vector<double> distribution;
        for (size_t counter{}; counter < loopN; ++counter)
        {
            inserter.Init();
            const auto [pre, mid, post] = dis_generator_(distribution, 0., max);
            multinomial.Loop_on(distribution, inserter);

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
    std::vector<UniformInserter> uniformInserter_{};
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
