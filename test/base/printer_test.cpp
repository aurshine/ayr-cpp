#include <ayr/base/meta/Printer.hpp>
#include <ayr/base/utest.hpp>
#include <cstdio> // Printer requires a FILE* stream.
#include <ayr/filesystem.hpp>

using namespace ayr;

namespace
{
    // Only the FILE* adapter uses stdio; creation, reading, and cleanup use fs.
    struct PrinterOutput
    {
        CString path = fs::join(fs::dirname(__FILE__), "printer_test.tmp");
        FILE* file = nullptr;
        PrinterOutput()
        {
            fs::write(path, "");
            file = std::fopen(path.c_str(), "ab");
        }
        ~PrinterOutput()
        {
            if (file) std::fclose(file);
            fs::remove(path);
        }
    };
}

int main()
{
    UTEST_SCOPE("Printer raw writes preserve binary data and only unread Buffer bytes")
    {
        PrinterOutput output;
        FILE* file = output.file;
        UTEST_EXPECT(file != nullptr);
        if (!file) return;
        Printer printer(file, "|", "!");
        UTEST_EXPECT(printer.file() == file);
        printer.write("a\0bignored", 3);
        Buffer buffer;
        buffer << "discard:tail";
        buffer.retrieve(8);
        buffer.close();
        printer.write(buffer);
        printer.write(vstr("c\0d", 3));
        printer();
        printer.setend("");
        printer();
        printer("x", "y");
        printer.flush();
        const CString actual = fs::read(output.path);
        const CString expected("a\0btailc\0d!x|y", sizeof("a\0btailc\0d!x|y") - 1);
        UTEST_EXPECT_EQ(actual, expected);
    };

    UTEST_SCOPE("ColorPrinter wraps each complete output and applies color changes")
    {
        PrinterOutput output;
        FILE* file = output.file;
        UTEST_EXPECT(file != nullptr);
        if (!file) return;
        ColorPrinter printer(file, Color::RED);
        printer("value", 42);
        printer.setcolor(Color::GREEN);
        printer(vstr("a\0b", 3));
        Printer(file).flush();
        const CString actual = fs::read(output.path);
        const CString expected("\033[31mvalue 42\n\033[0m\033[32ma\0b\n\033[0m", sizeof("\033[31mvalue 42\n\033[0m\033[32ma\0b\n\033[0m") - 1);
        UTEST_EXPECT_EQ(actual, expected);
    };

    UTEST_SCOPE("Printer separators, endings, binary text, and flush")
    {
        PrinterOutput output;
        FILE* file = output.file;
        UTEST_EXPECT(file != nullptr);
        if (!file) return;
        Printer printer(file, "|", "!");
        printer(42, fixed(1.25, 1), "x");
        printer();
        printer.setsep(":");
        printer.setend("\n");
        printer("a", "b");
        printer.write(vstr("x\0y", 3));
        printer.flush();
        const CString actual = fs::read(output.path);
        const CString expected("42|1.2|x!!a:b\nx\0y", sizeof("42|1.2|x!!a:b\nx\0y") - 1);
        UTEST_EXPECT_EQ(actual, expected);
    };
    UTEST_SCOPE("Transferred and consumed buffers retain owned text")
    {
        CString owned = [] { Buffer buffer; buffer << "abcdef"; return from_buffer(std::move(buffer)); }();
        UTEST_EXPECT_EQ(owned, "abcdef");
        CString tail = [] { Buffer buffer; buffer << "abcdef"; buffer.retrieve(2); return from_buffer(std::move(buffer)); }();
        UTEST_EXPECT_EQ(tail, "cdef");
    };
    return UTEST_COMPLETE();
}
