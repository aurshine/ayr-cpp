# 项目规范

## 命名规范
- 以类名命名的文件，首字母大写，如`MyClass.h`、`MyClass.cpp`等。
- 以文件功能命名的文件，首字母小写，如`main.cpp`、`config.h`等。
- 变量名、函数名采用下划线命名法，如`my_variable`、`my_function`等。
- 成员变量在命名后添加`_`，如`my_variable_`等。
- 类名采用驼峰命名法，如`MyClass`、`MySecondClass`等。
- 常量名采用全大写命名法，如`MY_CONSTANT`、`MY_SECOND_CONSTANT`等。

----

## 代码风格规范
- 缩进使用4个空格。
- .h文件使用#define #ifndef 的方式防止重复包含。
- 类的成员变量写在构造函数之前
- 类和函数声明上一行应该有注释，描述其功能。

----

## 头文件引入顺序
- 相关头文件，如 MyClass.cpp 应首先包含 MyClass.h）
- 系统头文件
- 标准库头文件
- 三方头文件
- 项目内部头文件

-----
## 注释风格（Doxygen风格）
### 类注释
```cpp
/**
 * @brief 简要说明
 *
 * @details 详细说明（可选）
 */
class MyClass { ... };

```
### 函数注释
```cpp
/**
 * @brief 简要说明
 *
 * @details 详细说明（可选）
 *
 * @param a 参数说明
 *
 * @return 返回值说明
 *
 * @note 备注信息，如使用限制、性能考虑等（可选）。
 *
 * @warning 警告信息，如线程不安全、必须先调用初始化等（可选）。
 */
bool fn(Arg a);
```

### 变量注释
```cpp
// 连接失败时的重试次数
int _retry_count; 
```