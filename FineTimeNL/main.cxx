#include "CSVWriter.h"
#include "FineTimeMC.hpp"
#include <chrono>
#include <iostream>
#include <string>

const unsigned int SEED_NUM = 0;

auto main() -> int
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    constexpr unsigned int BINSIZE = 5;
    const auto fullCycles = 400;
    const auto insertingNum = 5'000'000;
    const auto epochsN = 100;
    auto fineTimeMC = FineTimeMC<BINSIZE>{ fullCycles };
    fineTimeMC.SetLoopSize(insertingNum);
    fineTimeMC.SetEpochSize(epochsN);
    fineTimeMC.SetThreadsNum(4);

    auto writer = CSVWriter{ CSVColumn<float>{ "pre_prob" },
                             CSVColumn<float>{ "mid_prob" },
                             CSVColumn<float>{ "post_prob" },
                             CSVColumn<float>{ "mean" },
                             CSVColumn<float>{ "stderr" } };
    fineTimeMC.Run_all_probs(writer);
    writer.write("data.csv");

    auto [res, probs] = fineTimeMC.RunWithFixDistribution(0.3);
    auto& [pre, post] = probs;
    res.Draw({ pre, post });

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << "[ms]" << std::endl;
    return 0;
}
