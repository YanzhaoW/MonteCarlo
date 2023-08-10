#pragma once

#include <concepts>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fstream>
#include <functional>
#include <range/v3/view.hpp>
#include <string>
#include <vector>

template <int size1, std::size_t... sizes>
concept Equal = ((size1 == sizes) && ...);

template <int size1, std::size_t... sizes>
concept LessThan = ((size1 < sizes) && ...);

template <int size1, std::size_t... sizes>
concept NoLessThan = ((size1 >= sizes) && ...);

template <typename Type, typename Par>
concept Constructible_from = requires(Par par) { std::remove_cvref_t<Type>{ par }; };

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

template <typename... ColumnTypes>
class CSVWriter
{
  public:
    explicit CSVWriter(ColumnTypes&&... columns)
        : columns_{ std::make_tuple(std::forward<ColumnTypes>(columns)...) }
        , names_(GetNames(columns_))
    {
    }

    void add_row(auto&&... args)
        requires Equal<sizeof...(args), sizeof...(ColumnTypes)>
    {
        auto Pusher = [](auto&& left, auto&& right) { left.push_back(std::forward<decltype(right)>(right)); };
        Apply_element_wise(Pusher, columns_, std::make_tuple(std::forward<decltype(args)>(args)...));
    }

    void write(std::string_view filename)
    {
        auto ostream = std::ofstream(filename.data(), std::ios_base::out | std::ios_base::trunc);
        write_to_file(ostream);
        ostream.close();
    }

  private:
    std::tuple<ColumnTypes...> columns_;
    std::vector<std::string> names_;

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
