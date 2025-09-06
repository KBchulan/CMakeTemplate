# CMake Template

### 环境要求

常规环境不介绍了，需要安装 gtest 和 cppcheck。

```bash
sudo pacman -S gtest cppcheck
```

### 基本使用

此处介绍一下基本使用：

```bash
mkdir build && cd build

# 构建生成和构建
cmake ..
ninja # 或者 cmake --build .
```

此时可执行文件就生成到 `build/bin` 目录下了，接着可选的操作有： install、cpack：

```bash
ninja install # 会把所有的头文件、可执行文件及动态库下载到 build/install 下，方便部署到服务器上
cpack # 会进行打包，包括 sh、tgz
```

### 可选配置

可以选择启用测试，会测试你在 tests 下写的测试文件：

```bash
mkdir build && cd build

# 构建生成和构建
cmake .. -DBUILD_TESTING=ON
ninja # 或者 cmake --build .

# 运行测试
ctest # 或者 ctest --output-on-failure
```

也可以选择分析代码，此处我们集成了三种静态分析工具，可以自行选择，这里选择所有的，需要注意的是前两种也就是 AddressSanitizer 和 UndefinedBehaviorSanitizer 只是增加选项，在 ninja 时会自动启用，但是最后一个 cppcheck 需要手动运行：

```bash
mkdir build && cd build

# 构建生成和构建
cmake .. -DENABLE_ASAN=ON -DENABLE_UBSAN=ON -DENABLE_CPPCHECK=ON
ninja # 或者 cmake --build .

# 运行 cppcheck
cmake --build . --target cppcheck
```

最后，还提供了通过 **perf + 火焰图** 进行性能分析的功能，如果你需要进行性能分析，可以尝试运行 `scripts/profile.sh` 脚本，不管是普通程序还是服务器类型的程序都可以进行采样，并生成一张漂亮的火焰图，只需要根据提示操作即可：

```bash
mkdir build && cd build

# 开启性能分析选项
cmake .. -DENABLE_PROFILING=ON
ninja # 或者 cmake --build .

# 进行性能分析
./scripts/profile.sh
```

最后，本框架还集成了一个 CI 工作流，详细的内容可以自行查阅，对应 CD 流可以根据需要自己完成。
