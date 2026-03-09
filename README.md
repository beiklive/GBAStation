# BeikLiveStation
基于 libretro 核心 + borealis UI框架的模拟器


你的任务是：
1. 参考项目根目录的CMakeLists.txt, 编写 extern 目录下的CMakeLists.txt, 用于构建 extern 目录下的borealis库。
2. extern 目录下的CMakeLists.txt 需要对 mGBA库 进行libretro编译
3. 修改根目录的CMakeLists.txt, 将 extern 目录下的CMakeLists.txt 添加到构建过程中
4. 以上所有构建都要兼容 Windows 、 APPLE平台 、Linux平台、Switch平台, 并且需要使用 CMake 进行构建管理。可以参考各仓库的官方文档和示例来实现跨平台构建。
