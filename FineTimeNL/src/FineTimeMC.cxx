#include "FineTimeMC.hpp"

auto Divide_into(unsigned int totalSize, unsigned int num_of_threads) -> std::vector<unsigned int>
{
    num_of_threads = (totalSize >= num_of_threads) ? num_of_threads : totalSize;
    const auto divide = totalSize / num_of_threads;
    auto vec = std::vector<unsigned int>(num_of_threads, divide);

    auto res = totalSize - divide * num_of_threads;
    for (size_t index{}; index < res; ++index)
    {
        ++vec[index];
    }

    return vec;
}

void FineTimeMC::SetThreadsNum(unsigned int num)
{
    threads_num_ = num;
}

void FineTimeMC::SetRndNumber(unsigned int num)
{
    default_epoch_input_.rndNum = num;
}

void FineTimeMC::SetEntryN(unsigned int size)
{
    entryN_ = size;
    default_epoch_input_.entryN = size;
}

void FineTimeMC::Wait()
{
    for (auto& future : all_thread_results_)
    {
        future.get();
    }
}

void FineTimeMC::Write()
{
    for (auto* writer : writers_)
    {
        writer->write();
    }
}
