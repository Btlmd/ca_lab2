#include <cstring>
#include <chrono>
#include <cassert>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>

typedef std::chrono::milliseconds time_unit_t;

void bind(size_t cpuid)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    assert(sched_setaffinity(0, sizeof(mask), &mask) >= 0);
}

template <typename T, size_t leap>
void test_cache_size(std::ofstream &fout)
{
    constexpr size_t upper_kb = 64 * 1024;
    constexpr size_t lower_kb = 1;

    constexpr size_t samples = 1'000'000'000;
    std::vector<size_t> test_sizes;

    for (size_t i = lower_kb * 1024; i <= upper_kb * 1024; i *= 2)
    {
        test_sizes.push_back(i);
    }

    const size_t max_arr_len = *std::max_element(test_sizes.cbegin(), test_sizes.cend()) / sizeof(T);
    register auto a = new T[max_arr_len];
    memset(a, 0, max_arr_len * sizeof(T));

    fout << '-' << std::endl;
    fout << leap << std::endl;

    for (const auto size : test_sizes)
    {
        size_t arr_len = size / sizeof(T);
        size_t size_mask = arr_len - 1;
        auto begin = std::chrono::high_resolution_clock::now();
        for (register size_t i = 0; i < samples * leap; i += leap)
        {
            a[i & size_mask] = i;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << size << "|" << elapsed << std::endl;
    }

    delete[] a;
}

template <typename T>
void test_cache_line_size(std::ofstream &fout)
{
    constexpr size_t mem_len = 64 * 1024;
    constexpr size_t arr_len = mem_len / sizeof(T);
    constexpr size_t samples = 1'000'000'000;
    std::vector<size_t> test_sizes;

    for (size_t i = 4; i <= 512; i *= 2)
    {
        test_sizes.push_back(i);
    }

    register auto a = new T[arr_len];
    memset(a, 0, arr_len * sizeof(T));

    for (const auto size : test_sizes)
    {
        const register size_t step = size / sizeof(T);
        auto begin = std::chrono::high_resolution_clock::now();
        size_t size_mask = arr_len - 1;
        for (register size_t i = 0; i < samples * step; i += step)
        {
            a[i & size_mask] = i;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << size << "|" << elapsed << std::endl;
    }
    delete[] a;
}

void test_cache_associativity(std::ofstream &fout)
{
    constexpr size_t l1d_size = 32 * 1024;
    constexpr size_t arr_len = l1d_size * 2 / sizeof(uint64_t);
    constexpr size_t arr_mask = arr_len - 1;
    constexpr size_t samples = 1'000'000'000;

    uint64_t *a = new uint64_t[arr_len];

    for (size_t n = 1; n <= 7; ++n)
    {
        const size_t block_count = 1 << n;
        const size_t stride = arr_len / block_count * 2;

        auto begin = std::chrono::high_resolution_clock::now();
        for (register size_t i = 0; i < samples * stride; i += stride)
        {
            a[i & arr_mask] = i;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << n << "|" << elapsed << std::endl;
    }
}

int main()
{
    bind(0);

    {
        std::ofstream fout("result_cache_size.txt");
        test_cache_size<uint8_t, 512>(fout);
        test_cache_size<uint8_t, 256>(fout);
        test_cache_size<uint8_t, 128>(fout);
        test_cache_size<uint8_t, 64>(fout);
        test_cache_size<uint8_t, 32>(fout);
        fout.close();
    }

    {
        std::ofstream fout("result_cache_line_size.txt");
        test_cache_line_size<uint8_t>(fout);
        fout.close();
    }

    {
        std::ofstream fout("result_cache_associativity.txt");
        test_cache_associativity(fout);
        fout.close();
    }

    return 0;
}