# **ayr — augment your runtime**

**ayr** 是一个以“简洁语法 + 轻量高效”为理念的现代 C++20 工具库。它提供标准库未支持的能力，同时为许多已有能力提供更快、更易用、更一致的替代方案。

设计目标：
- **简洁性**：提供类似 Python/Go 的极简 API，减少样板代码。
- **高性能**：通过 SSO 优化、Robin Hood 哈希、异步 IO 等技术榨取性能。
- **现代化**：深度集成 C++20 特性（Concepts, Coroutines, Ranges）。

---

## 1. 字符串处理

### CString (字节串)
0 开销的 C 风格字符串封装，支持从 `char*`, `std::string`, `std::string_view` 无缝切换。
- `vstr()`: 创建视图（不拷贝数据）。
- `dstr()`: 拷贝数据并构造。

```cpp
using namespace ayr;
auto view = vstr("hello"); 
auto data = dstr("world");
```

### Atring (Unicode 字符串)
支持多种编码（UTF-8/16/32），提供丰富的操作，大部分方法支持 `constexpr`。
```cpp
using namespace ayr::literals;

constexpr Atring s1 = "hello"as + " world"as;
constexpr Atring s2 = "你好世界"as.vslice(0, 2); // "你好"
bool has = s1.contains("hello"as);
auto parts = "a,b,c"as.split(","as); // 返回 Array<Atring>
```

---

## 2. 高性能容器

### DynArray (动态数组)
在处理非平凡拷贝（Non-trivial copy）类型时，性能显著优于 `std::vector`。
```cpp
DynArray<int> arr;
arr.append(1);
arr.append(2);
print(arr[0], arr[1]);
```

### Dict & Set (字典与集合)
基于 **Robin Hood 哈希**实现，提供 O(1) 的平均查找速度。`Dict` 保证迭代顺序与插入顺序一致。
```cpp
Dict<Atring, int> scores;
scores["Alice"as] = 100;
scores["Bob"as] = 95;

if (scores.contains("Alice"as)) {
    print(scores["Alice"as]);
}

for (auto [name, score] : scores.items()) {
    print(name, ":", score);
}
```

### Chain (双向链表)
```cpp
Chain<int> list;
list.append(1);
list.prepend(0);
```

---

## 3. JSON 解析
高效的 JSON 加载与导出，支持复杂的嵌套结构。
```cpp
#include <ayr/json.hpp>

auto loader = json::JsonLoader();
auto val = loader(R"({"name": "ayr", "version": 1.0, "tags": ["cpp", "fast"]})"as);

print(val["name"as]);      // "ayr"
print(val["tags"as][0]);   // "cpp"
```

---

## 4. 异步与协程

### ThreadPool (线程池)
```cpp
#include <ayr/async/ThreadPool.hpp>

async::ThreadPool pool(4);
auto future = pool.push_future([]{ return 42; });
print("Result:", future.get());
```

### AsyncExecutor (任务图执行器)
支持定义任务间的依赖关系（DAG），并利用线程池并行执行。
```cpp
#include <ayr/async.hpp>

async::AsyncExecutor exec(4);
auto t1 = exec.create_task([] { print("Task 1"); });
auto t2 = exec.create_task([] { print("Task 2"); });
auto t3 = exec.create_task([] { print("Task 3"); });

// 定义依赖：t2 和 t3 必须在 t1 完成后才执行
t1->add_child(t2);
t1->add_child(t3);

exec.run(); // 阻塞直至所有任务完成
```

### Generator (生成器)
利用 C++20 协程实现 Python 风格的生成器。
```cpp
coro::Generator<int> range(int n) {
    for (int i = 0; i < n; ++i) co_yield i;
}

for (int i : range(5)) print(i); // 0 1 2 3 4
```

### Task & IoContext (异步 IO)
基于协程的非阻塞 IO 框架。
```cpp
coro::IoContext ctx;
ctx.run([](coro::IoContext* io) -> coro::Task<void> {
    // 异步逻辑...
    co_return;
}(&ctx));
```

---

## 5. 网络编程
支持高性能的异步 Socket 与 HTTP 请求。

### TCP Client/Server
```cpp
// 异步读取示例
auto client = co_await net::open_connect("127.0.0.1", 8080, io_context);
co_await client.write("hello");
Buffer buf(1024);
co_await client.read(buf);
```

### HTTP 请求
```cpp
auto res = co_await net::get(io_context, net::uri("http://baidu.com"as));
print(res.text());
```

---

## 6. 文件系统
提供类似 Python `os.path` 的简洁接口。
```cpp
#include <ayr/filesystem.hpp>

fs::AyrFile file("test.txt", "w");
file.write("hello ayr");
file.close();

if (fs::exists("test.txt")) {
    for (auto line : fs::AyrFile("test.txt", "r").readlines()) {
        print(line);
    }
}

auto path = fs::join("home", "user", "config.json");
```

---

## 7. 实用工具

### Printer & Logging
支持带颜色的终端输出和便捷的变量打印。
```cpp
print("Hello", 123, true); // 自动格式化并以空格分隔
tlog(scores);               // 打印变量名和值: scores = {"Alice": 100, ...}
ayr_error("Something went wrong"); // 红色输出
```

### Timer (性能计时)
```cpp
Timer_ms timer;
timer.into();
// 执行耗时操作...
print("Elapsed:", timer.escape(), "ms");

// 或者直接测量函数
double ms = timer([]{ /* work */ });
```

### Itertools (迭代器工具)
```cpp
for (auto [i, val] : enumerate(arr)) { ... }
for (auto i : range(0, 10, 2)) { ... }
for (auto [a, b] : zip(arr1, arr2)) { ... }
```

### Optional
增强版的 `std::optional`，支持函数式操作。
```cpp
Optional<int> opt = 10;
auto res = opt.map([](int x){ return x * 2; })
              .filter([](int x){ return x > 15; })
              .value_or(0);
```

---

## 编译要求
- 支持 **C++20** 的编译器 (GCC 10+, Clang 12+, MSVC 19.28+)
- CMake 3.15+
