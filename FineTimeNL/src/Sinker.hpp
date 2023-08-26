#pragma once

#include "LineDrawer.hpp"
#include "traits.hpp"
#include <TCanvas.h>
#include <TH1.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <range/v3/view.hpp>
#include <string>
#include <vector>

template <int index, typename... T1>
    requires NoLessThan<index, std::tuple_size_v<std::remove_cvref_t<T1>>...>
void Apply(auto&& Optn, T1&&... tuple)
{
}

template <int index = 0, typename... T1>
    requires LessThan<index, std::tuple_size_v<std::remove_cvref_t<T1>>...>
void Apply(auto&& Optn, T1&&... tuple)
{
    std::invoke(std::forward<decltype(Optn)>(Optn), std::get<index>(std::forward<T1>(tuple))...);
    Apply<index + 1>(std::forward<decltype(Optn)>(Optn), std::forward<T1>(tuple)...);
}

template <typename... T>
void Apply_element_wise(auto&& Optn, T&&... tuples)
{
    Apply(std::forward<decltype(Optn)>(Optn), std::forward<T>(tuples)...);
}

auto GetZipViewer(auto& Optn, auto&& tuple)
{
    return std::apply([&](auto&&... args) { return ranges::zip_with_view(Optn, args.get()...); },
                      std::forward<decltype(tuple)>(tuple));
}

auto GetNames(auto&& tuple)
{
    auto names = std::vector<std::string>();
    names.reserve(std::tuple_size_v<std::remove_cvref_t<decltype(tuple)>>);

    auto Add_name = [&names](auto&& ele) { names.emplace_back(ele.get_name()); };
    Apply_element_wise(Add_name, std::forward<decltype(tuple)>(tuple));
    return names;
}

class Sinker
{
  public:
    virtual void write() = 0;
};

template <typename WriteStrategy, typename... ColumnTypes>
class CSVWriter : public Sinker
{
  public:
    explicit CSVWriter(WriteStrategy&& strategy, ColumnTypes&&... columns)
        : columns_{ std::make_tuple(std::forward<ColumnTypes>(columns)...) }
        , names_(GetNames(columns_))
        , write_strategy_{ std::forward<WriteStrategy>(strategy) }
    {
    }

    void add_row(auto&&... args)
        requires Equal<sizeof...(args), sizeof...(ColumnTypes)>
    {
        auto Pusher = [](auto&& left, auto&& right) { left.push_back(std::forward<decltype(right)>(right)); };
        Apply_element_wise(Pusher, columns_, std::make_tuple(std::forward<decltype(args)>(args)...));
    }

    void write() override
    {
        if (filename_.empty())
        {
            throw std::logic_error("csv output filename not specified!");
        }
        std::cout << "writing to file " << filename_ << "\n";
        auto ostream = std::ofstream(filename_.c_str(), std::ios_base::out | std::ios_base::trunc);
        write_to_file(ostream);
        std::cout << "writing to file " << filename_ << " finished\n";
    }

    void SetFileName(std::string_view filename)
    {
        filename_ = filename;
    }

    void operator()(const auto& result)
    {
        write_strategy_(this, result);
    }

  private:
    std::tuple<ColumnTypes...> columns_;
    std::vector<std::string> names_;
    std::string filename_;
    WriteStrategy write_strategy_;

    void write_to_file(auto& ostream)
    {
        ostream << fmt::format("{}\n", fmt::join(names_, ", "));

        auto Join = [](auto&&... args)
        { return fmt::format("{}\n", fmt::join(std::make_tuple(std::forward<decltype(args)>(args)...), ", ")); };

        for (const auto& line : GetZipViewer(Join, columns_))
        {
            ostream << line;
        }
    }
};

template <typename DataType>
class CSVColumn
{
  public:
    using Type = DataType;
    explicit CSVColumn(std::string name)
        : name_{ std::move(name) }
    {
    }

    auto get() const -> const auto&
    {
        return data_;
    }

    auto get_name() const -> const auto&
    {
        return name_;
    }

    void push_back(auto&& value)
        requires Constructible_from<Type, decltype(value)>
    {
        data_.push_back(std::forward<decltype(value)>(value));
    }

  private:
    std::string name_;
    std::vector<DataType> data_;
};

template <typename WriteStrategy>
class HistDrawer : public Sinker
{
  public:
    HistDrawer(std::string_view filename, WriteStrategy&& strategy)
        : filename_{ filename }
        , write_strategy_{ std::forward<WriteStrategy>(strategy) }
    {
    }

    virtual void write()
    {
        constexpr int default_canvas_width = 1000;
        constexpr int default_canvas_height = 800;
        auto canvas = TCanvas{ "canvas", "canvas", default_canvas_width, default_canvas_height };
        fmt::print("histogram total entries: {}\n", histogram_->GetEntries());
        histogram_->Draw();
        canvas.Update();

        auto linesDraw = LineDrawer(2);
        const auto& [start, end] = boundaries_;
        auto lineL = linesDraw.Draw_vLine(&canvas, start);
        auto lineR = linesDraw.Draw_vLine(&canvas, end);
        lineL->Draw();
        lineR->Draw();

        canvas.SaveAs(filename_.c_str());
    }

    void operator()(const auto& result)
    {
        write_strategy_(this, result);
    }

    void Set(const Parallel_run_output& result)
    {
        // fmt::print("setting process: histogram total entries: {}\n", result.histogram->GetEntries());
        histogram_ = std::unique_ptr<TH1>(static_cast<TH1*>(result.histogram->Clone()));
        // fmt::print("pa {},pb {},pc {}, entryN {}", result.pre_prob, result.mid_prob, result.post_prob,
        // result.entryN);
        boundaries_ = { result.pre_prob * result.entryN, (1 - result.post_prob) * result.entryN };
    }

  private:
    std::string filename_;
    std::pair<double, double> boundaries_;
    std::unique_ptr<TH1> histogram_;
    std::remove_const_t<WriteStrategy> write_strategy_;
};
