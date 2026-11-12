#include <ayr/base/meta/error.hpp>

#include <source_location>
#include <utility>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
    UTEST_SCOPE("AyrError retains explicit source location and a null terminated what message")
    {
        auto location = std::source_location::current();
        AyrError error("SampleError", "bad value", location);
        UTEST_EXPECT_EQ(error.name(), "SampleError");
        UTEST_EXPECT(error.is("SampleError"));
        UTEST_EXPECT(!error.is("OtherError"));
        UTEST_EXPECT_EQ(error.error(), "SampleError: bad value");
        UTEST_EXPECT(error.details().contains(location.file_name()));
        UTEST_EXPECT(error.details().contains(location.function_name()));
        UTEST_EXPECT(error.details().contains(fmt("{}, {}", location.line(), location.column())));
        UTEST_EXPECT_EQ(error.details()[-1], '\0');
        UTEST_EXPECT_EQ(vstr(error.what()),
            vstr(error.details().data(), error.details().size() - 1));
    };

    UTEST_SCOPE("AyrError copy and move assignment preserve owned diagnostic text")
    {
        AyrError source("LongError", cstr((CString("x") * 256)));
        CString details = dstr(source.details());
        AyrError copied(source);
        AyrError moved(std::move(copied));
        AyrError assigned("OldError", "old");
        assigned = moved;
        assigned = assigned;
        UTEST_EXPECT_EQ(assigned.details(), details);
        AyrError move_assigned("OldError", "old");
        move_assigned = std::move(assigned);
        move_assigned = std::move(move_assigned);
        UTEST_EXPECT_EQ(move_assigned.details(), details);
        UTEST_EXPECT_EQ(move_assigned.error(), cstr(CString("LongError: ") + (CString("x") * 256)));
    };

    UTEST_SCOPE("raise_error and error macros throw the expected name and message")
    {
        const bool previous = AyrError::show_error_while_throw;
        struct RestoreShow
        {
            bool value;
            ~RestoreShow() { AyrError::show(value); }
        } restore{previous};
        AyrError::show(false);
        UTEST_EXPECT(!AyrError::show_error_while_throw);
        auto check = [](auto action, const CString& name) {
            bool caught = false;
            try { action(); }
            catch (const AyrError& error)
            {
                caught = true;
                UTEST_EXPECT_EQ(error.name(), name);
                UTEST_EXPECT_EQ(error.error(), fmt("{}: message", name));
                UTEST_EXPECT(error.details().contains("message"));
            }
            UTEST_EXPECT(caught);
        };
        check([] { raise_error(AyrError("DirectError", "message")); }, "DirectError");
        check([] { KeyError("message"); }, "KeyError");
        check([] { ValueError("message"); }, "ValueError");
        check([] { TypeError("message"); }, "TypeError");
        check([] { RuntimeError("message"); }, "RuntimeError");
        check([] { NotImplementedError("message"); }, "NotImplementedError");
        check([] { NullPointerError("message"); }, "NullPointerError");
        check([] { FileNotFoundError("message"); }, "FileNotFoundError");
        check([] { PermissionError("message"); }, "PermissionError");
        check([] { EncodingError("message"); }, "EncodingError");
        check([] { SystemError("message"); }, "SystemError");
        check([] { SSLError("message"); }, "SSLError");
        check([] { JsonValueError("message"); }, "JsonValueError");
        AyrError::show(true);
        UTEST_EXPECT(AyrError::show_error_while_throw);
    };
    return UTEST_COMPLETE();
}
