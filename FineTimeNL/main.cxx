#include "CSVWriter.h"
#include "FineTimeMC.hpp"
#include <chrono>
#include <cxxopts.hpp>
#include <iostream>
#include <string>

const unsigned int SEED_NUM = 0;

auto main(int argc, char** argv) -> int
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    constexpr unsigned int BINSIZE = 11;

    cxxopts::Options options("FineTimeNL", "Command flags for FineTime NeuLAND");
    options.add_options()("t,thread", "thread numbers", cxxopts::value<int>())(
        "d, dataN", "number of data points", cxxopts::value<int>())(
        "c, cycles", "number of full cycles", cxxopts::value<int>())(
        "r, r_num", "number of random values", cxxopts::value<int>())(
        "max", "max probability of the central bin", cxxopts::value<double>()->default_value("1."))("h,help",
                                                                                                    "Print usage");

    auto optresult = options.parse(argc, argv);
    if (optresult.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const unsigned int fullCycles = optresult.count("cycles") ? optresult["cycles"].as<int>() : 400;
    const auto insertingNum = optresult.count("r_num") ? optresult["r_num"].as<int>() : 500'000;
    const auto epochsN = optresult.count("dataN") ? optresult["dataN"].as<int>() : 100;
    const auto threadNum = optresult.count("thread") ? optresult["thread"].as<int>() : 4;
    auto fineTimeMC = FineTimeMC<BINSIZE>{ fullCycles };

    fineTimeMC.SetLoopSize(insertingNum);
    fineTimeMC.SetEpochSize(epochsN);
    fineTimeMC.SetThreadsNum(threadNum);

    auto writer = CSVWriter{ CSVColumn<float>{ "pre_prob" },
                             CSVColumn<float>{ "mid_prob" },
                             CSVColumn<float>{ "post_prob" },
                             CSVColumn<float>{ "mean" },
                             CSVColumn<float>{ "stderr" } };
    auto all_prob_futures = fineTimeMC.Run_all_probs(writer, optresult["max"].as<double>());

    auto futureFixProb = fineTimeMC.RunWithFixDistribution(0.3);

    // waiting for results:
    for (auto& future : all_prob_futures)
    {
        future.get();
    }
    writer.write("data.csv");
    futureFixProb.get();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << "[ms]" << std::endl;
    return 0;
}
