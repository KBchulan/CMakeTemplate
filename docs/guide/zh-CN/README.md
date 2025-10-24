# CMake Template

该项目是一个基于 CMake 的模板项目，旨在完成基建，让您可以专心于业务开发。

### 环境要求

常规环境不介绍了，其余需要安装 gtest、 cppcheck 和 benchmark。

```bash
sudo pacman -S gtest cppcheck benchmark
```

如果真不想安装也 ok，cmake中做了配置，会自动克隆用于当前构建，那就比较慢了，且每次都需要等，建议配一下环境。

### 快速开始

此处介绍一下基本使用：

```bash
mkdir build && cd build

# 构建生成和构建
cmake ..
ninja # 或者 cmake --build .
```

此时可执行文件就生成到 `build/bin` 目录下了，接着可选的操作有： install、cpack：

```bash
ninja install  # 会把所有的头文件、可执行文件及动态库下载到 build/install 下，方便部署到服务器上
cpack          # 会进行打包，包括 sh、tgz
```

除此以外，配置了一些可选项用于构建，可以根据需要自行选择:

#### 格式化

可以使用已经制作好的脚本进行代码格式化，会按照 .clang-format 的要求进行格式化：

```bash
./scripts/format.sh
```

#### 静态分析代码

也可以选择分析代码，此处我们集成了三种静态分析工具，可以自行选择，这里选择所有的，需要注意的是前两种也就是 AddressSanitizer 和 UndefinedBehaviorSanitizer 只是增加选项，在 ninja 时会自动启用，但是最后一个 cppcheck 需要手动运行：

```bash
mkdir build && cd build

# 构建生成和构建
cmake .. -DENABLE_CPPCHECK=ON
ninja # 或者 cmake --build .

# 运行 cppcheck
cmake --build . --target cppcheck
```

#### 单元测试选项

可以选择启用测试，会测试你在 tests 下写的测试文件：

```bash
mkdir build && cd build

# 构建生成和构建
cmake .. -DBUILD_TESTING=ON
ninja # 或者 cmake --build .

# 运行测试
ctest # 或者 ctest --output-on-failure
```

#### 代码覆盖率测试

支持使用 `lcov` 和 `genhtml` 生成详细的代码覆盖率报告，会对在 tests 中引用的所有项目文件进行测试，需要先安装依赖:

```bash
sudo pacman -S lcov
```

然后启用覆盖率选项并运行测试，注意一定要打开测试选项:

```bash
# 启用测试和覆盖率
cmake .. -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON
ninja

# 运行覆盖率测试
ninja coverage
```

覆盖率报告会生成到 `build/coverage/index.html`，可以用浏览器查看。

#### 基准测试

同时也可以选择指定benchmark，常规编译即可，基准测试文件就会生成到 build/bin 下：

```bash
mkdir build && cd build

# 构建生成和构建
cmake .. -DBUILD_BENCHMARK=ON
ninja # 或者 cmake --build .
```

**注意**:  logger 的基准测试不可避免的输出内容，会干扰到我们正常的 BM 输出，可以简单的选择 grep 来过滤:

```bash
./build/bin/bench_logger | grep "BM_"
```

#### 性能分析

最后，还提供了通过 **perf + 火焰图** 进行性能分析的功能，如果你需要进行性能分析，可以尝试运行 `scripts/profile.sh` 脚本，不管是普通程序还是服务器类型的程序都可以进行采样，并生成一张漂亮的火焰图，只需要根据提示操作即可：

```bash
mkdir build && cd build

# 开启性能分析选项
cmake .. -DENABLE_PROFILING=ON
ninja # 或者 cmake --build .

# 进行性能分析
./scripts/profile.sh
```

### 目录介绍

这里我们介绍一下当前项目的已有内容，方便你能对此项目有更多认知。

```bash
CMakeTemplate/
|-- .github/           # Github Actions 工作流
|-- benchmark/         # 基准测试
|-- docs/              # 项目相关文档
|-- include/           # 公共头文件
|-- scripts/           # 辅助脚本
|-- src/               # 主工程源文件
|-- tests/             # GoogleTest 单元测试
|-- .clang-format      # Google 风格的格式化选项
|-- .clangd            # clangd 插件分析配置
|-- .gitignore         # 忽略git提交内容
|-- CMakeLists.txt     # 顶层 CMake 构建脚本，集中管理选项、目标和依赖
|-- Config.cmake.in    # 导出给下游项目的配置模板
|-- CTestConfig.cmake  # CDash 配置使用
|-- LICENSE            # 开源许可
--- README.md          # 自述文件
```

该项目中我们提供的组件有: 无锁队列(BM 测试100M+)，日志组件(BM 测试260M+)，id生成组件(BM测试 50M+)。

同时还提供了三个脚本: format.sh(用于格式化所有代码)，profile.sh(可以搜索所有的可执行文件，用于性能分析)，workflow.sh(在本地执行所有工作流)。

其余部分由于内容不多，基本可以见名知义，这里不再介绍，如果需要开发新内容，我在每个目录都写了示例，可直接参考。
