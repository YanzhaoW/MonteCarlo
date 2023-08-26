#pragma once

#include "DistributionGen.hpp"
#include "MultiNomial.hpp"
#include "Sinker.hpp"
#include "UniformInserter.hpp"
#include "traits.hpp"
#include <fmt/core.h>
#include <fmt/std.h>
#include <future>
#include <range/v3/view.hpp>
#include <vector>

constexpr unsigned int BINSIZE = 3;
template <typename T>
concept Loopable = requires(T loopOp) {
                       loopOp.Init();
                       loopOp.Reset();
                   };

auto Divide_into(unsigned int totalSize, unsigned int num_of_threads) -> std::vector<unsigned int>;

class FineTimeMC
{
  public:
    FineTimeMC() = default;

    void SetThreadsNum(unsigned int num);
    void SetRndNumber(unsigned int num);
    void SetEntryN(unsigned int size);

    void RunFixedPbAllEntryN(double midProb, int min, int max, auto& writer);
    void RunFixedPbAllPa(double midProb, double min, double max, unsigned int num, auto& writer);
    void RunWithAllFixed(std::array<double, 3> distribution, auto& writer);

    void Wait();
    void Write();

  private:
    unsigned int entryN_ = 0;
    unsigned int rndNums_ = 0;
    unsigned int threads_num_ = 1;
    Parallel_run_input default_epoch_input_ = {};
    mutable std::atomic<int> threads_count_ = 0;
    DisGenerator<BINSIZE> dis_generator_;
    mutable std::vector<Sinker*> writers_;
    std::vector<std::future<void>> all_thread_results_;
    mutable std::mutex mu_recorder_;

    void Single_run(const std::array<double, 3>& distribution,
                    MultiNomial& multinomial,
                    auto& inserter,
                    auto& recorder,
                    const Parallel_run_input& input) const;
    void Parallel_run_all_cycles(Parallel_run_input input,
                                 const std::array<double, 3>& distribution,
                                 auto& inserter,
                                 auto& writer) const;
    void Parallel_run_pre(Parallel_run_input input, double mid_prob, auto& inserter, auto& writer) const;
};

void FineTimeMC::Single_run(const std::array<double, 3>& distribution,
                            MultiNomial& multinomial,
                            auto& inserter,
                            auto& writer,
                            const Parallel_run_input& input) const
{
    multinomial.SetEntryN(input.entryN);
    multinomial.SetRndNum(input.rndNum);
    inserter.Init();
    multinomial.Loop_on(distribution, inserter);
    auto result = Parallel_run_output{};
    result.histogram = inserter.GetHist();
    result.stat = inserter.GetResult();
    result.pre_prob = distribution[0];
    result.mid_prob = distribution[1];
    result.post_prob = distribution[2];
    result.entryN = multinomial.GetEntryN();

    {
        auto lock = std::scoped_lock{ mu_recorder_ };
        writer(result);
    }
    inserter.Reset();
}

void FineTimeMC::Parallel_run_all_cycles(Parallel_run_input input,
                                         const std::array<double, 3>& distribution,
                                         auto& inserter,
                                         auto& writer) const
{
    auto multinomial = MultiNomial(inserter.GetEngine());
    for (unsigned int sample_size{ input.entryN }; sample_size < input.entryN + input.entryNloopSize; ++sample_size)
    {
        input.entryN = sample_size;
        Single_run(distribution, multinomial, inserter, writer, input);
    }
}

void FineTimeMC::Parallel_run_pre(Parallel_run_input input, double mid_prob, auto& inserter, auto& writer) const
{
    auto distribution = std::array<double, 3>{ 0., mid_prob, 0. };
    auto multinomial = MultiNomial(inserter.GetEngine());
    for (int pre_index = 0; pre_index < input.pa_num; ++pre_index)
    {
        auto pre_value = pre_index * input.pa_step + input.pa_begin;
        distribution.front() = pre_value;
        distribution.back() = 1 - distribution[0] - distribution[1];
        Single_run(distribution, multinomial, inserter, writer, input);
    }
}

void FineTimeMC::RunFixedPbAllPa(double midProb, double min, double max, unsigned int num, auto& writer)
{
    auto threads = Divide_into(num, threads_num_);
    const double step = (max - min) / num;
    writers_.push_back(&writer);

    for (const auto& size : threads)
    {
        auto inputPar = default_epoch_input_;
        inputPar.pa_begin = min;
        min += step * size;
        inputPar.pa_num = size;
        inputPar.pa_step = step;

        auto future = std::async(std::launch::async,
                                 [inputPar, &writer, this, midProb]()
                                 {
                                     std::string histname = fmt::format("pa_hist_{}", threads_count_++);
                                     auto inserter = UniformInserter{ inputPar.entryN, histname };
                                     Parallel_run_pre(inputPar, midProb, inserter, writer);
                                 });
        all_thread_results_.emplace_back(std::move(future));
    }
}

void FineTimeMC::RunFixedPbAllEntryN(double midProb, int min, int max, auto& writer)
{

    auto threads = Divide_into(max - min, threads_num_);
    std::array<double, 3> distribution = {};
    dis_generator_(distribution, midProb);
    writers_.push_back(&writer);

    int init{ min };
    auto sample_size_starts = ranges::transform_view(threads,
                                                     [&init](const auto& ele)
                                                     {
                                                         auto old = init;
                                                         init += ele;
                                                         return old;
                                                     });
    for (const auto& [start, num] : ranges::views::zip(sample_size_starts, threads))
    {
        auto input = default_epoch_input_;
        input.entryN = start;
        input.entryNloopSize = num;
        auto future = std::async(std::launch::async,
                                 [input, this, &writer, distribution]()
                                 {
                                     std::string histname = fmt::format("entryN_hist_{}", threads_count_++);
                                     auto inserter = UniformInserter{ input.entryN + input.entryNloopSize, histname };
                                     Parallel_run_all_cycles(input, distribution, inserter, writer);
                                 });
        all_thread_results_.emplace_back(std::move(future));
    }
}

void FineTimeMC::RunWithAllFixed(std::array<double, 3> distribution, auto& writer)
{
    auto input = default_epoch_input_;
    writers_.push_back(&writer);
    auto future = std::async(std::launch::async,
                             [input, this, distribution, &writer]()
                             {
                                 auto inserter = UniformInserter{ entryN_, "fix distribution" };
                                 auto multinomial = MultiNomial(inserter.GetEngine());
                                 Single_run(distribution, multinomial, inserter, writer, input);
                             });
    all_thread_results_.emplace_back(std::move(future));
}
