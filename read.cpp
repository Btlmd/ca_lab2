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
T test_cache_size(std::ofstream &fout)
{
    constexpr size_t upper = 128;
    constexpr size_t deno = 4;
    constexpr size_t samples = 100'000'000;
    std::vector<size_t> test_sizes;
    for (size_t c = 0; c < deno; ++c)
    {
        for (size_t i = 1 * 1024; i <= 1024 * 1024 * upper; i *= 2)
        {
            test_sizes.push_back(i + i * c / deno);
        }
    }
    std::sort(test_sizes.begin(), test_sizes.end());

    const size_t max_arr_len = *std::max_element(test_sizes.cbegin(), test_sizes.cend()) / sizeof(T);
    register auto a = new T[max_arr_len];
    memset(a, 0, max_arr_len * sizeof(T));

    fout << '-' << std::endl;
    fout << leap << std::endl;

    for (const auto size : test_sizes)
    {
        size_t arr_len = size / sizeof(T);
        auto begin = std::chrono::high_resolution_clock::now();
        for (register size_t i = 0; i < samples * leap; i += leap)
        {
            register auto pos = i % arr_len;
            a[pos] += 11452 + pos;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << size << "|"<< elapsed << std::endl;
    }

    auto ret = a[a[0] % max_arr_len];
    delete[] a;
    return ret;
}

template <typename T>
T test_cache_line_size(std::ofstream &fout)
{
    constexpr size_t mem_len = 1024 * 1024 * 1024;
    constexpr size_t arr_len = mem_len / sizeof(uint8_t);
    constexpr size_t samples = 100'000'000;
    std::vector<size_t> test_sizes;

    for (size_t i = 4; i <= 512; i *= 2)
    {
        test_sizes.push_back(i);
    }

    register auto a = new T[arr_len];
    memset(a, 0, arr_len * sizeof(T));

    for (const auto size : test_sizes)
    {
        const register size_t step = size / sizeof(uint8_t);
        auto begin = std::chrono::high_resolution_clock::now();
        for (register size_t i = 0; i < samples * step; i += step)
        {
            a[i % arr_len] += 114 + i;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << size << "|" << elapsed << std::endl;
    }

    auto ret = a[a[0] % arr_len];
    delete[] a;
    return ret;
}

void test_cache_associativity(std::ofstream &fout)
{
    constexpr size_t l1_size = 1024 * (1024 + 512);
    constexpr size_t l1_len = l1_size / sizeof(size_t);
    constexpr size_t samples = 100'000'000;
    constexpr size_t max_step = 32;
    auto a = new size_t[l1_len * max_step];

    // warm up
    {
        register size_t step = 10;
        for (register size_t i = 0, pos = 0; i < samples; ++i, pos += l1_len)
        {
            if (__glibc_unlikely(pos >= l1_len * step)) {
                pos -= l1_len * step;
            }
            a[pos]++;
        }
    }

    // test cache associativity
    for (register size_t step = 1; step < max_step; ++step)
    {
        size_t pos = 0;
        auto begin = std::chrono::high_resolution_clock::now();
        for (register size_t i = 0, pos = 0; i < samples; ++i, pos += l1_len)
        {
            if (__glibc_unlikely(pos >= l1_len * step)) {
                pos -= l1_len * step;
            }
            a[pos]++;
        }
        auto elapsed = std::chrono::duration_cast<time_unit_t>(std::chrono::high_resolution_clock::now() - begin).count();
        fout << step << "|" << elapsed << std::endl;
    }
    delete[] a;
}

int main()
{
    bind(105);
    
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