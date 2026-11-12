#ifndef AYR_BRANCHMARK_REPORT_HPP
#define AYR_BRANCHMARK_REPORT_HPP

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ayr/base/Array.hpp>
#include <ayr/filesystem.hpp>

namespace benchmark_report
{
    using namespace ayr;

    inline unsigned checks = 0, failures = 0, rows = 0;
    inline unsigned ayr_faster = 0, std_faster = 0, tied = 0;

    inline void check(bool passed, const CString& expression, const CString& file, int line)
    {
        ++checks;
        if (!passed)
        {
            ++failures;
            ayr_error(fmt("[FAIL] {}:{}: {}", file, line, expression));
        }
    }

    // Scripts pass a filename only; reports always belong to branchmark.
    inline void append_report(const CString& text)
    {
        const CString filename = vstr(std::getenv("AYR_BRANCHMARK_REPORT"));
        if (filename.empty()) return;
        try
        {
            if (filename.contains("/") || filename.contains("\\") || filename.contains(":"))
                ValueError("Benchmark report must be a filename in branchmark");
            fs::AyrFile(fs::join(fs::dirname(__FILE__), filename), "a").write(text);
        }
        catch (const AyrError& error)
        {
            ++failures;
            ayr_error(fmt("[FAIL] Cannot append benchmark report: {}", error.error()));
        }
    }

    inline CString cell(const CString& text)
    {
        Buffer result;
        for (char ch : text)
        {
            if (ch == '|') result << "\\|";
            else if (ch == '\n' || ch == '\r') result << ' ';
            else result << ch;
        }
        return from_buffer(std::move(result));
    }

    struct Timing { double median, minimum, maximum; };

    inline Timing summarize(Array<double> samples)
    {
        if (samples.empty()) ValueError("Cannot summarize empty benchmark samples");
        std::sort(samples.data(), samples.data() + samples.size());
        const auto n = samples.size();
        const double median = n % 2 ? samples[n / 2]
            : (samples[n / 2 - 1] + samples[n / 2]) / 2;
        return {median, samples.front(), samples.back()};
    }

    inline void section(const CString& title, const CString& details)
    {
        print(fmt("\n{}\n{}", title, details));
        print("Median ms; STD/AYR > 1: AYR faster; negative AYR time %: less time than STD.");
        print(fmt("{} {} {} {} {} {}", left_pad("Operation", 24), right_pad("AYR ms", 11),
            right_pad("STD ms", 11), right_pad("STD/AYR", 10), right_pad("AYR time %", 12), right_pad("Faster", 8)));
        append_report(fmt("\n## {}\n\n{}\n\n", cell(title), cell(details)));
        append_report("| Operation | AYR median ms | STD median ms | STD/AYR | AYR time change | Faster | AYR min..max ms | STD min..max ms |\n"
            "|---|---:|---:|---:|---:|---|---:|---:|\n");
    }

    inline void comparison(const CString& operation, Timing ayr, Timing standard)
    {
        ++rows;
        if (!(ayr.median > 0 && standard.median > 0)
            || !std::isfinite(ayr.median) || !std::isfinite(standard.median))
        {
            print(fmt("{} {} {} n/a n/a n/a", left_pad(operation, 24),
                right_pad(fixed(ayr.median, 4), 11), right_pad(fixed(standard.median, 4), 11)));
            append_report(fmt("| {} | {} | {} | n/a | n/a | n/a | n/a | n/a |\n",
                cell(operation), fixed(ayr.median, 4), fixed(standard.median, 4)));
            return;
        }
        const double ratio = standard.median / ayr.median;
        const double change = (ayr.median / standard.median - 1) * 100;
        const CString faster = ayr.median < standard.median ? "AYR" : ayr.median > standard.median ? "STD" : "equal";
        if (ayr.median < standard.median) ++ayr_faster;
        else if (ayr.median > standard.median) ++std_faster;
        else ++tied;
        const auto ratio_text = fmt("{}x", fixed(ratio, 2));
        const auto change_text = fmt("{}{}%", change >= 0 ? "+" : "", fixed(change, 1));
        print(fmt("{} {} {} {} {} {}", left_pad(operation, 24),
            right_pad(fixed(ayr.median, 4), 11), right_pad(fixed(standard.median, 4), 11),
            right_pad(ratio_text, 10), right_pad(change_text, 12), right_pad(faster, 8)));
        append_report(fmt("| {} | {} | {} | {} | {} | {} | {}..{} | {}..{} |\n",
            cell(operation), fixed(ayr.median, 4), fixed(standard.median, 4), ratio_text, change_text, faster,
            fixed(ayr.minimum, 4), fixed(ayr.maximum, 4), fixed(standard.minimum, 4), fixed(standard.maximum, 4)));
    }

    inline void single_section(const CString& title)
    {
        print(fmt("\n{}\n{} {} {} {}", title, left_pad("Input", 28),
            right_pad("Median ms", 12), right_pad("Min ms", 12), right_pad("Max ms", 14)));
        append_report(fmt("\n## {}\n\nFile I/O excluded; 1 warmup + 7 samples.\n\n", cell(title)));
        append_report("| Input | Median ms | Min ms | Max ms |\n|---|---:|---:|---:|\n");
    }

    inline void single(const CString& input, Timing timing)
    {
        ++rows;
        print(fmt("{} {} {} {}", left_pad(input, 28), right_pad(fixed(timing.median, 4), 12),
            right_pad(fixed(timing.minimum, 4), 12), right_pad(fixed(timing.maximum, 4), 14)));
        append_report(fmt("| {} | {} | {} | {} |\n", cell(input),
            fixed(timing.median, 4), fixed(timing.minimum, 4), fixed(timing.maximum, 4)));
    }

    inline int complete()
    {
        if (ayr_faster + std_faster + tied > 0)
        {
            print(fmt("\nRows with lower observed median: AYR {} | STD {} | equal {}", ayr_faster, std_faster, tied));
            append_report(fmt("\nRows with lower observed median: **AYR {} / STD {} / equal {}**.\n", ayr_faster, std_faster, tied));
        }
        append_report(fmt("\nValidation: **{}** ({} checks, {} failures).\n", failures ? "FAIL" : "PASS", checks, failures));
        print(fmt("\n[{}] {} result rows; {} checks; {} failures", failures ? "FAIL" : "PASS", rows, checks, failures));
        return failures ? 1 : 0;
    }
}

#define BENCHMARK_EXPECT(expr) benchmark_report::check(bool(expr), #expr, __FILE__, __LINE__)
#define BENCHMARK_EXPECT_EQ(a, b) benchmark_report::check((a) == (b), #a " == " #b, __FILE__, __LINE__)

#endif // AYR_BRANCHMARK_REPORT_HPP
