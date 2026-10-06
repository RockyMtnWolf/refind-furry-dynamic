# 修改说明

- 添加 `refind/dynamic_info.c/.h`，菜单阶段读取 CPUID、SMBIOS、EFI memory map、System Table 与 SecureBoot 变量。
- 在 `screen.c` 背景绘制和缓存之间调用动态叠层，局部重绘继续使用含动态文字的背景缓存。
- 添加 `dynamic_info` 配置项，默认关闭；同步更新 `global.h` 结构和 `main.c` 初始化表。
- `refind/Makefile` 加入新增模块，未修改扫描/引导算法。
- 使用用户提供的主题 ZIP 和桌面 furry 图片。背景整理至 3840×2160，字体与选择框按 1.5 倍适配。手工 menuentry 的 icon 从 ESP 根目录加载，配置使用绝对路径。
- QEMU 使用 TCG/q35/SMM/OVMF Secure Boot-capable 固件，独立 VARS 默认未注册密钥，实际 SecureBoot 为 OFF。普通 OVMF 未提供 SecureBoot 变量时显示 UNKNOWN。
- 构建每次先 clean，避免上游头文件依赖缺失导致对象结构布局过期。
