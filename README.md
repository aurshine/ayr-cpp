# ayr-cpp

**ayr — augment your runtime**

ayr-cpp 是一个以“简洁语法 + 轻量高效”为理念的现代 C++20 工具库。

目标是用简洁、接近 Python的接口提供常用运行时能力。

项目包含 Unicode 字符串、容器、JSON、线程池、协程、文件系统、异步 TCP/TLS 和 HTTP 客户端。


## 目录

- [功能概览](#功能概览)
- [环境与第三方依赖](#环境与第三方依赖)
- [引入项目](#引入项目)
- [构建与测试](#构建与测试)
- [快速开始](#快速开始)
- [模块使用说明](#模块使用说明)
- [使用注意事项](#使用注意事项)
- [许可证](#许可证)

## 功能概览

| 模块 | 推荐头文件 | 主要能力 |
| --- | --- | --- |
| 基础库 | `<ayr/base.hpp>` | `Atring`、`CString`、`Buffer`、`Array`、`Shared`、打印、计时器和迭代工具 |
| 通用组件 | `<ayr/air/*.hpp>` | `DynArray`、`Dict`、`Set`、`Chain`、`Optional`、Robin Hood 哈希表和实验性日志 |
| JSON | `<ayr/json.hpp>` | JSON 解析、类型访问、数组/对象修改和序列化 |
| 多线程 | `<ayr/async.hpp>` | 线程池、返回 `future` 的任务、任务依赖图 |
| 协程 | `<ayr/coro.hpp>` | `Task`、`Generator`、`IoContext`、定时等待 |
| 文件系统 | `<ayr/filesystem.hpp>` | 文件读写、路径处理、目录遍历、递归遍历与删除 |
| 异步 Socket/TLS | `<ayr/net/Socket.hpp>` | 非阻塞 TCP 客户端/服务端、异步读写、TLS |
| HTTP | `<ayr/net/http.hpp>` | URI、请求头、HTTP/1.1 请求构造、响应解析、HTTP/HTTPS 客户端 |

ayr-cpp 不需要单独编译静态库或动态库。链接 CMake 接口目标 `ayr` 后，
头文件路径、OpenSSL、可选 fmt 以及 Windows 所需系统库会自动传递给使用方。

## 环境与第三方依赖

### 基本要求

- 支持 C++20 的编译器和标准库
- CMake 3.28 或更高版本
- Windows 或 Linux（异步网络模块的完整支持平台）

文件系统代码还包含 macOS 分支，但 `IoContext` 的事件选择器目前没有 macOS
实现，因此在 macOS 上不能使用完整的协程网络模块。

### 依赖说明

| 依赖 | 是否必需 | 用途 |
| --- | --- | --- |
| OpenSSL | 必需 | TLS/HTTPS、证书校验和加解密 |
| fmt | 可选 | 格式化支持；找到 fmt 时定义 `AYR_USE_FMT=1` |
| 标准库 `<format>` | 条件必需 | 未找到 fmt 时作为格式化后端 |
| Crypt32 | Windows 自动链接 | 从 Windows 系统证书存储加载受信任根证书 |

即使只使用基础容器，当前顶层 `CMakeLists.txt` 仍会执行
`find_package(OpenSSL REQUIRED)`，因此通过 `add_subdirectory` 引入时也必须提供
OpenSSL。

Ubuntu/Debian：

```bash
sudo apt update
sudo apt install cmake ninja-build libssl-dev libfmt-dev
```

其中 `libfmt-dev` 可以省略，但此时编译器的 C++20 标准库必须提供
`std::format`。

Windows 可以通过 vcpkg 安装依赖：

```powershell
vcpkg install openssl:x64-windows fmt:x64-windows
```

配置项目时将对应的 vcpkg toolchain 文件传给 CMake。

## 引入项目

推荐将仓库放到项目的 `third_party` 目录，并通过 CMake 引入：

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/ayr-cpp)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ayr)
```

目录结构示例：

```text
my_app/
├── CMakeLists.txt
├── main.cpp
└── third_party/
    └── ayr-cpp/
```

如果不使用 CMake，也可以直接添加 `ayr-cpp/include` 到编译器头文件搜索路径，
但需要自行链接 OpenSSL 的 SSL/Crypto 库；Windows 还要链接 `Crypt32`。因此更
推荐使用仓库提供的 `ayr` 目标。

## 构建与测试

克隆并构建仓库：

```bash
git clone https://github.com/aurshine/ayr-cpp.git
cd ayr-cpp
cmake -S . -B out/build -DBUILD_TESTING=ON
cmake --build out/build --parallel
ctest --test-dir out/build --output-on-failure
```

对于 Visual Studio 等多配置生成器，需要指定配置：

```powershell
cmake -S . -B out/build -DBUILD_TESTING=ON
cmake --build out/build --config Debug --parallel
ctest --test-dir out/build -C Debug --output-on-failure
```

测试程序按 `test/*.cpp` 分别生成，例如 `atring_test`、`json_test`、
`fs_test`、`socket_test` 和 `http_test`。其中：

- `socket_test` 需要本机 `127.0.0.1:7777` 端口可用；
- `http_test` 包含真实 HTTPS 请求，需要可用的互联网连接；
- `tls_test` 会在本地生成测试证书并验证 TLS 握手与分片传输。

## 快速开始

```cpp
#include <ayr/base.hpp>
#include <ayr/air/Dict.hpp>
#include <ayr/air/DynArray.hpp>

using namespace ayr;

int main()
{
    Atring message = "你好，"as + "ayr-cpp"as;
    print(message);

    DynArray<int> numbers{1, 2, 3};
    numbers.append(4);

    Dict<Atring, int> scores;
    scores["Alice"as] = 100;
    scores["Bob"as] = 95;

    for (const auto& [name, score] : scores.items())
        print(name, score);
}
```

`using namespace ayr;` 同时使 `as` 字面量可用。`"文本"as` 会按默认编码
（当前为 UTF-8）创建 `Atring`。

## 模块使用说明

### 1. 基础库：字符串、缓冲区与迭代工具

`Atring` 按 Unicode 码点存储和索引，适合文本处理；`CString` 按字节存储，
适合文件、Socket 和与 C API 交互。

- `Atring::from_utf8`、`encode`：UTF-8 解码和编码；
- `slice`、`index`、`replace`、`split`、`join`：常用字符串操作；
- `vstr`：创建不拥有内存的 `CString` 视图；
- `dstr`：创建拥有数据的 `CString` 深拷贝；
- `Buffer`：可增长的字节缓冲区，支持 `<<` 追加和读写位置管理；
- `arr`、`range`、`enumerate`、`zip`：固定数组和迭代辅助工具。

```cpp
#include <ayr/base.hpp>

using namespace ayr;

int main()
{
    Atring text = Atring::from_utf8("你好，C++");
    print(text.size());               // 6 个 Unicode 码点
    print(text.slice(0, 2));          // 你好
    print(text.encode());             // UTF-8 CString

    Atring words = "alpha,beta,gamma"as;
    for (const auto& item : words.split(","as))
        print(item);

    CString owned = dstr("raw bytes");
    CString view = vstr(owned.data(), owned.size());
    print(view);

    auto values = arr(10, 20, 30);
    for (auto [index, value] : enumerate(values))
        print(index, value);

    for (auto value : range(0, 10, 2))
        print(value);                 // 0 2 4 6 8

    Buffer buffer;
    buffer << "count=" << values.size();
    print(vstr(buffer.peek(), buffer.readable_size()));
}
```

`CString` 视图不延长原始内存的生命周期。需要保存数据时使用 `dstr`、
`clone()`，或确保被引用的数据始终有效。

`Shared<T>` 是项目提供的引用计数对象：

```cpp
#include <ayr/base.hpp>

using namespace ayr;

int main()
{
    Shared<int> value(42);
    Shared<int> alias = value;

    *alias = 7;
    print(*value, value.use_count()); // 7 2
}
```

### 2. 通用组件：容器、Optional 与日志

主要容器如下：

- `DynArray<T>`：分块扩容的动态数组，支持随机访问；
- `Dict<K, V>`：保留插入顺序的 Robin Hood 哈希字典；
- `Set<T>`：保留插入顺序的 Robin Hood 哈希集合；
- `Chain<T>`：双向链表；
- `Optional<T>`：可选值，支持 `map`、`and_then`、`transform`、`filter`
  和 `or_else`；
- `Appender<T>`、`Table<T>`：供上层容器使用的连续追加器和底层哈希表。

```cpp
#include <ayr/air/Dict.hpp>
#include <ayr/air/DynArray.hpp>
#include <ayr/air/Optional.hpp>
#include <ayr/air/Set.hpp>

using namespace ayr;

int main()
{
    DynArray<int> values{1, 2, 3};
    values.insert(1, 10);             // [1, 10, 2, 3]
    values.pop();                     // 删除末尾元素

    Set<int> left{1, 2, 3};
    Set<int> right{3, 4};
    print(left & right);              // {3}
    print(left | right);              // {1, 2, 3, 4}

    Dict<int, CString> names{{1, "one"}, {2, "two"}};
    names.setdefault(3, "three");
    for (const auto& [id, name] : names.items())
        print(id, name);

    Optional<int> answer(21);
    auto doubled = answer
        .filter([](int value) { return value > 0; })
        .map([](int value) { return value * 2; });
    print(doubled.value());           // 42
}
```

`Dict` 和 `Set` 的键必须满足 ayr 的 `Hashable` 约束：可以由 `std::hash`
处理，或提供 `__hash__()` 成员函数。

`<ayr/air.hpp>` 还会包含当前的实验性日志实现。该实现目前存在 MSVC
编译兼容性和多翻译单元链接限制，因此跨平台项目建议像上例一样按需包含容器
子头文件，暂时不要依赖 `Log` 作为正式日志方案。

### 3. JSON

JSON 模块使用 `Atring` 作为输入文本和字符串值。`Json` 可保存 null、整数、
浮点数、布尔值、字符串、数组和对象。

```cpp
#include <ayr/json.hpp>

using namespace ayr;

int main()
{
    json::Json document = json::loads(R"({
        "name": "ayr",
        "version": 1,
        "tags": ["cpp", "coroutine"]
    })"as);

    print(document["name"as].as<json::JsonStr>());
    print(document["tags"as][0].as<json::JsonStr>());

    document["version"as] = json::integer(2);
    document["tags"as].append("http"as);

    Atring compact_or_pretty = json::dumps(document);
    print(compact_or_pretty);
}
```

常用接口：

- `json::loads(text)`：解析一个 JSON 值；
- `json::loads_prefix(text)`：返回 `{解析结果, 剩余文本}`；
- `json::dump(value, buffer)`：序列化为 `Buffer`；
- `json::dumps(value)`：序列化为 `Atring`；
- `is<*>()`、`as<*>()`：检查并取得具体类型；
- `operator[]`、`append`、`pop`、`clear`：操作数组或对象。

`JsonLoader::MAX_DEPTH` 控制最大解析深度。类型不匹配或输入非法时会抛出
`AyrError` 的派生错误。

### 4. 多线程：ThreadPool 与任务依赖图

向线程池提交有返回值的任务：

```cpp
#include <ayr/async.hpp>

using namespace ayr;

int main()
{
    async::ThreadPool pool(4);

    auto first = pool.push_future([] { return 20; });
    auto second = pool.push_future([] { return 22; });

    print(first.get() + second.get()); // 42
}
```

`push` 提交无返回值任务，`push_future` 返回 `std::future`。`run()` 等待当前
队列完成但保持线程池可用；`stop()` 停止并丢弃尚未开始的任务。线程池对象
析构时会停止工作线程。

`AsyncExecutor` 用 `then` 表达任务间的依赖：

```cpp
#include <ayr/async.hpp>

using namespace ayr;

int main()
{
    async::AsyncExecutor executor(4);

    auto prepare = executor.create_task([] { print("prepare"); });
    auto compile = executor.create_task([] { print("compile"); });
    auto package = executor.create_task([] { print("package"); });

    prepare->then(compile);
    compile->then(package);

    executor.run();
}
```

一个任务可以依赖多个父任务；所有父任务完成后才会被调度。任务必须属于同一
个 `AsyncExecutor`。当前执行器的 `run()` 是一次性运行入口。

### 5. 协程：Generator、Task 与 IoContext

`Generator<T>` 可以直接用于范围 `for`：

```cpp
#include <ayr/coro.hpp>

using namespace ayr;

coro::Generator<int> countdown(int from)
{
    for (int value = from; value > 0; --value)
        co_yield value;
    co_return coro::finish;
}

int main()
{
    for (int value : countdown(3))
        print(value);                 // 3 2 1
}
```

生成器也允许 `co_return value`，该返回值会作为序列的最后一个元素；如果
不需要末尾元素，使用 `co_return coro::finish`。

`Task<T>` 配合 `IoContext` 运行异步任务和定时器：

```cpp
#include <chrono>

#include <ayr/coro.hpp>

using namespace ayr;

coro::Task<int> delayed_answer(coro::IoContext* io)
{
    co_await coro::Sleep(std::chrono::milliseconds(100), io);
    co_return 42;
}

int main()
{
    coro::IoContext io;
    auto task = delayed_answer(&io);
    print(io.run(task));
}
```

`IoContext` 不拥有加入其中的协程句柄。如果使用 `io.add(task.coroutine())`
同时调度多个任务，所有 `Task` 对象必须一直存活到 `io.run()` 返回。

### 6. 文件系统

```cpp
#include <ayr/filesystem.hpp>

using namespace ayr;

int main()
{
    CString directory = dstr("data");
    CString path = fs::join(directory, "hello.txt");

    fs::mkdir(directory, true);
    fs::write(path, "hello ayr");

    print(fs::read(path));
    print(fs::basename(path));        // hello.txt
    print(fs::filesize(path));        // 9

    for (const auto& entry : fs::listdir(directory))
        print(entry);

    for (auto& [root, dirs, files] : fs::walk(directory))
        print(root, dirs, files);
}
```

常用接口：

- 文件：`AyrFile`、`fs::read`、`fs::write`、`fs::writelines`；
- 路径：`join`、`abspath`、`basename`、`dirname`、`split`、`splitext`；
- 查询：`exists`、`isfile`、`isdir`、`filesize`；
- 目录：`mkdir`、`listdir`、`walk`、`remove`。

`AyrFile` 支持 `"r"`、`"w"`、`"a"` 三种模式。`fs::remove` 会递归删除
非空目录；调用前务必确认路径。`mkdir` 只创建指定层级，不等价于递归
`mkdir -p`。

### 7. 异步 TCP Socket

下面的服务端接受一次连接，并把收到的数据原样写回：

```cpp
#include <ayr/net/Socket.hpp>

using namespace ayr;

coro::Task<void> echo_once(coro::IoContext* io)
{
    net::Acceptor acceptor("127.0.0.1", 7777, io);
    acceptor.listen();

    net::Socket socket = co_await acceptor.accept();

    Buffer buffer(1024);
    net::IoResult result = co_await socket.read(buffer);
    if (!result.ok())
        RuntimeError(result.error);

    result = co_await socket.write(buffer);
    if (!result.ok())
        RuntimeError(result.error);
}

int main()
{
    coro::IoContext io;
    auto task = echo_once(&io);
    io.run(task);
}
```

客户端使用
`co_await net::open_connect(host, port, io_context)` 建立连接。`Socket::read`
和 `Socket::write` 返回 `IoResult`；调用方应检查 `ok()`，并可通过 `bytes`
取得本次传输字节数。向 `write` 传入的 `Buffer` 会在成功写入后消费已发送
数据。

TLS 由 `TlsLayer` 提供。HTTP 客户端会根据 URI 的 `http`/`https` scheme
自动决定是否启用 TLS；直接使用 Socket 时也可以向 `open_connect` 或
`Acceptor` 传入配置好的 `TlsLayer`。

### 8. HTTP/HTTPS

`Session` 提供 `request`、`get` 和 `post`。下面演示 HTTPS GET：

```cpp
#include <ayr/net/http.hpp>

using namespace ayr;

coro::Task<void> fetch(
    coro::IoContext* io,
    net::Session* session
)
{
    net::HttpHeaders headers;
    headers.insert("User-Agent"as, "ayr-cpp-example"as);

    net::HttpResponse response = co_await session->get(
        io,
        net::uri("https://example.com/"as),
        std::move(headers)
    );

    print(response.status_code);
    print(response.body);
}

int main()
{
    coro::IoContext io;
    net::Session session;
    auto task = fetch(&io, &session);
    io.run(task);
}
```

`Session` 和顶层任务都应覆盖异步请求的完整生命周期。对于多个并发请求，
先创建并保存所有 `Task`，用 `io.add(task.coroutine())` 加入事件循环，再
调用 `io.run()`。

HTTP 模块还提供：

- `net::uri` / `Uri`：解析 scheme、host、port、path、query 和 fragment；
- `HttpHeaders`：大小写不敏感的头部查询、插入和删除；
- `HttpRequest`：请求行、请求头和正文构造与序列化；
- `ResponseParser`：增量解析固定长度、chunked、无正文和连接关闭定界响应；
- `net::request`、`net::get`、`net::post`：单次请求的便捷函数。

当前客户端每次请求都会新建 TCP 连接，尚未实现 Cookie 管理和连接池。

## 更多示例

更多真实用法和边界行为可以参考 [`test`](test) 目录中的测试程序。

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
