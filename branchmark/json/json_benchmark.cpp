#include <ayr/filesystem.hpp>
#include <ayr/json.hpp>
#include <ayr/base/meta.hpp>
#include "../report.hpp"

using namespace ayr;

int main()
{

    {
        // Reuse the unit test fixtures; file I/O stays outside the timed region.
        CString data_dir = fs::join(fs::dirname(__FILE__), "../../test/json/data");
        benchmark_report::single_section("JSON parsing / file I/O excluded / 1 warmup + 7 samples");
        bool measured = false;
        for (auto&& filename : fs::listdir(data_dir))
        {
            measured = true;
            CString path = fs::join(data_dir, filename);
            Atring input = Atring::from_utf8(fs::AyrFile(path, "r").read());
            Array<double> samples(7);
            for (std::size_t i = 0; i <= samples.size(); ++i)
            {
                Timer_ms timer;
                timer.into();
                json::Json value = json::loads(input);
                const auto elapsed = timer.escape();
                BENCHMARK_EXPECT(value.is<json::JsonDict>());
                BENCHMARK_EXPECT(value.size() > 0);
                if (i > 0) samples[i - 1] = elapsed;
            }
            if (!benchmark_report::failures)
            {
                benchmark_report::single(filename, benchmark_report::summarize(samples));
            }
        }
        if (!measured)
            print("[SKIPPED] JSON parsing benchmark: no files in test/json/data.");
    };
    return benchmark_report::complete();
}
