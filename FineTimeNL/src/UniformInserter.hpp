#pragma once

#include "LineDrawer.hpp"
#include "traits.hpp"
#include <Math/GSLRndmEngines.h>
#include <TCanvas.h>
#include <TH1I.h>
#include <fmt/core.h>
#include <numeric>

extern const unsigned int SEED_NUM;

void Print(std::string_view str);

struct Parallel_run_output;

auto GetCenterBoundary(const auto& vec, double multiplier = 1)
{
    if (vec.size() < 3)
    {
        throw std::logic_error("vector size too small!");
    }
    const auto offSet = (vec.end() - vec.begin()) / 2;
    auto start = std::accumulate(vec.begin(), vec.begin() + offSet, 0.);
    auto end = start + vec.at(offSet);
    return std::make_pair(start * multiplier, end * multiplier);
}

class UniformInserter
{
  public:
    ~UniformInserter() = default;
    UniformInserter(const UniformInserter&) = delete;
    UniformInserter(UniformInserter&&) = default;
    auto operator=(const UniformInserter&) -> UniformInserter& = delete;
    auto operator=(UniformInserter&&) -> UniformInserter& = default;

    explicit UniformInserter(unsigned int num, std::string_view histname)
    {
        engine_.Initialize();
        engine_.SetSeed(SEED_NUM);
        TH1::AddDirectory(false);
        constexpr int hist_entries = 10000;
        constexpr int histBin_entries = 100;
        histogram_ = std::make_unique<TH1I>(histname.data(), histname.data(), hist_entries, 0, num);
    }

    void operator()(const auto& vec)
    {
        const auto [start, end] = GetCenterBoundary(vec);
        if (start == end)
        {
            return;
        }
        auto binValue = engine_.Rndm() * static_cast<double>(end - start);
        auto value = binValue + static_cast<double>(start);
        histogram_->Fill(value);
        // Print(fmt::format("filling histogram with entries {}\n", histogram_->GetEntries()));
        // histogramBin_->Fill(binValue);
    }

    [[nodiscard]] auto GetCloneHist() -> std::unique_ptr<TH1>
    {
        return std::unique_ptr<TH1>(static_cast<TH1*>(histogram_->Clone()));
    }
    [[nodiscard]] auto GetHist() -> TH1*
    {
        // Print("passing here......");
        // Print(fmt::format("getting histogram with entries {}\n", histogram_->GetEntries()));
        return histogram_.get();
    }

    [[nodiscard]] auto GetResult()
    {
        SetResult();
        return result_;
    }

    void Init()
    {
        result_ = {};
    }

    void Reset()
    {
        histogram_->Reset("M");
    }

    // void DrawAll(std::pair<double, double> boundary, std::string_view filename = "distri")
    // {
    //     Draw(boundary, fmt::format("{}.png", filename));
    // }

    auto GetEngine() -> auto*
    {
        return &engine_;
    }

  private:
    std::unique_ptr<TH1I> histogram_;
    ROOT::Math::GSLRandomEngine engine_ = {};
    MeanError result_;

    void SetResult()
    {
        auto mean = static_cast<float>(histogram_->GetMean());
        auto err = static_cast<float>(histogram_->GetStdDev());
        if (err == 0)
        {
            Print(fmt::format("WARN: 0 stderr! histogram entries: {}", histogram_->GetEntries()));
        }
        result_ = MeanError{ mean, err };
    }

    static void Draw(std::string_view filename, TH1* hist)
    {
        constexpr int default_canvas_width = 1000;
        constexpr int default_canvas_height = 800;
        auto canvas = TCanvas{ "canvas", "canvas", default_canvas_width, default_canvas_height };
        hist->Draw();
        canvas.Update();
        canvas.SaveAs(filename.data());
    }
};
