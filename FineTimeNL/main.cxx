#include "CSVWriter.h"
#include "DistributionGen.hpp"
#include "LineDrawer.hpp"
#include "MultiNomial.hpp"
#include <TCanvas.h>
#include <TGraph.h>
#include <TRandom3.h>
#include <concepts>
#include <fmt/core.h>
#include <iostream>
#include <numeric>
// #include <ranges>
#include <string>

template <typename T>
concept Loopable = requires(T loopOp) {
    loopOp.Init();
    loopOp.Finish();
};

struct MeanError
{
    double mean = 0.;
    double err = 0.;
};

struct Result
{
    MeanError stat{};
    double mid_prob = 0.;
    double pre_prob = 0.;
    double post_prob = 0.;
};

template <typename T>
auto GetCenterBoundary(const std::vector<T>& vec, double multiplier = 1)
{
    if (vec.size() < 3)
    {
        throw std::logic_error("vector size too small!");
    }
    const auto offSet = (vec.end() - vec.begin()) / 2;
    auto start = accumulate(vec.begin(), vec.begin() + offSet, 0.);
    auto end = start + vec.at(offSet);
    return std::make_pair(start * multiplier, end * multiplier);
}

class UniformInserter
{
  public:
    explicit UniformInserter(unsigned int num)
    {
        constexpr int hist_entries = 1500;
        histogram_ = std::make_unique<TH1I>("hist", "hist", hist_entries, 0, num);
        histogram_->AddDirectory(false);
    }

    template <typename T>
    void operator()(const std::vector<T>& vec)
    {
        const auto [start, end] = GetCenterBoundary(vec);
        auto rndValue = rnd.Uniform(start, end);
        histogram_->Fill(rndValue);
    }

    [[nodiscard]] auto GetHist() -> auto*
    {
        return histogram_.get();
    }

    void Init()
    {
        result_.reset();
    }

    [[nodiscard]] auto Finish()
    {
        SetResult();
        histogram_->Reset("M");
        return result_;
    }

    void Draw(TCanvas* canvas, std::pair<double, double> boundary)
    {
        histogram_->Draw();
        canvas->Update();

        auto linesDraw = LineDrawer(2);
        const auto& [start, end] = boundary;
        auto lineL = linesDraw.Draw_vLine(canvas, start);
        auto lineR = linesDraw.Draw_vLine(canvas, end);
        lineL->Draw();
        lineR->Draw();

        canvas->SaveAs("distri.png");
    }

  private:
    std::unique_ptr<TH1I> histogram_;
    TRandom3 rnd = TRandom3{ 0 };
    std::optional<MeanError> result_;

    void SetResult()
    {
        auto mean = histogram_->GetMean();
        auto err = histogram_->GetStdDevError();
        result_ = MeanError{ mean, err };
    }
};

template <int binSize, typename Writer>
    requires OddLargeNumber<binSize>
class FineTimeMC
{
  public:
    FineTimeMC() = default;
    explicit FineTimeMC(unsigned int num, Writer&& writer)
        : totalSamples_{ num }
        , csv_{ std::forward<Writer>(writer) }
    {
        multinomial_.SetEntrySize(num);
    }

    void SetLoopSize(unsigned int loopSize)
    {
        loopSize_ = loopSize;
        multinomial_.SetLoopSize(loopSize);
    }

    auto GetOperator() -> auto&
    {
        return uniformInserter_;
    }

    void LoopWithFixDistribution(double midProb)
    {
        std::vector<double> distribution;
        const auto probs = dis_generator_(distribution, midProb);
        multinomial_.Loop_on(distribution, uniformInserter_);
        SetResult(probs);
    }

    void Run()
    {
        if (sample_size_ == 0)
        {
            throw std::logic_error("Epoch size is 0!");
        }
        for (size_t counter{}; counter < threads_num_; ++counter)
        {
            Epoch(sample_size_ / threads_num_);
        }
    }

    void Epoch(int loopN)
    {
        for (size_t counter{}; counter < loopN; ++counter)
        {
            std::vector<double> distribution;
            uniformInserter_.Init();
            const auto probs = dis_generator_(distribution);
            multinomial_.Loop_on(distribution, uniformInserter_);
            SetResult(probs);
            RecordResult();
        }
    }

    void SetResult(auto tuple)
    {
        const auto [pre, mid, post] = tuple;
        epoch_result_.stat = uniformInserter_.Finish().value();
        epoch_result_.mid_prob = mid;
        epoch_result_.pre_prob = pre;
        epoch_result_.post_prob = post;
    }

    void SetEpochSize(unsigned int size)
    {
        sample_size_ = size;
    }

    void Write_to(std::string_view filename)
    {
        csv_.write(filename);
    }

  private:
    unsigned int totalSamples_ = 0;
    unsigned int loopSize_ = 0;
    unsigned int sample_size_ = 1;
    unsigned int threads_num_ = 1;
    Result epoch_result_;
    DisGenerator<binSize> dis_generator_;
    MultiNomial multinomial_;
    UniformInserter uniformInserter_{ totalSamples_ };
    Writer csv_;

    void RecordResult()
    {
        csv_.add_row(epoch_result_.pre_prob,
                     epoch_result_.mid_prob,
                     epoch_result_.post_prob,
                     epoch_result_.stat.mean,
                     epoch_result_.stat.err);
    }
};

auto main() -> int
{
    constexpr unsigned int BINSIZE = 5;
    const auto fullCycles = 1000;
    const auto loopSize = 500000;
    auto writer = CSVWriter{ CSVColumn<double>{ "pre_prob" },
                             CSVColumn<double>{ "mid_prob" },
                             CSVColumn<double>{ "post_prob" },
                             CSVColumn<double>{ "mean" },
                             CSVColumn<double>{ "stderr" } };
    auto fineTimeMC = FineTimeMC<BINSIZE, decltype(writer)>{ fullCycles, std::move(writer) };
    fineTimeMC.SetLoopSize(loopSize);
    fineTimeMC.SetEpochSize(10);

    fineTimeMC.Run();
    fineTimeMC.Write_to("test.csv");

    // auto& uniInsert = fineTimeMC.GetOperator();

    // auto canvas = TCanvas{ "canvas", "canvas", 1000, 800 };
    // fineTimeMC.Draw(&canvas);
    // uniInsert.Draw(&canvas, fineTimeMC.GetBoundary());

    // auto* th1 = uniInsert.GetHist();
    // std::cout << "mean: " << th1->GetMean() << "\n";

    return 0;
}
