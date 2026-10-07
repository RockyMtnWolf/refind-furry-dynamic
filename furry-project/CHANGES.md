# 修改说明

- 添加 `refind/dynamic_info.c/.h`，菜单阶段读取 CPUID、SMBIOS、EFI memory map、System Table 与 SecureBoot 变量。
- 在 `screen.c` 背景绘制和缓存之间调用动态叠层，局部重绘继续使用含动态文字的背景缓存。
- 添加 `dynamic_info` 配置项，默认关闭；同步更新 `global.h` 结构和 `main.c` 初始化表。
- `refind/Makefile` 加入新增模块，未修改扫描/引导算法。
- 使用用户提供的主题 ZIP 和桌面 furry 图片。背景整理至 3840×2160，字体与选择框按 1.5 倍适配。手工 menuentry 的 icon 从 ESP 根目录加载，配置使用绝对路径。
- QEMU 使用 TCG/q35/SMM/OVMF Secure Boot-capable 固件，独立 VARS 默认未注册密钥，实际 SecureBoot 为 OFF。普通 OVMF 未提供 SecureBoot 变量时显示 UNKNOWN。
- 构建每次先 clean，避免上游头文件依赖缺失导致对象结构布局过期。

## 第二阶段

- 增加 `dynamic_devices.c/.h`：PCI Display Controller/GOP、可更新名称映射、Block I/O 整盘选择、Disk Info ATA/NVMe/SCSI 型号、NVMe Pass Thru 只读 Identify Controller。
- 增加 CPUID 空值时 SMBIOS Type 4 备用读取；Type 17 类型/插槽/配置速度，多 DIMM 最低速度、容量未知回退；内存映射统计排除不可用内存。
- 按实际 GOP 模式缩放字体 atlas、大小图标及徽章；信息限制在左侧 62% 并省略超长行。字体缩放清理衍生缓存。
- 字体重新加载也清理衍生缓存；重新扫描时重新应用动态布局，避免扫描后 atlas 宽度与缓存宽度不同导致文字乱码或消失。
- 生产 SMBIOS 解析器直接包含到宿主回归测试，使用 ASan/UBSan 测试坏校验、截断、扩展/KiB/未知容量和 CPU 字符串；测试数据不写入产品。
- 增加专用签名环境、签名/未签名子程序对照、Ubuntu/Windows 实际介质准备、KVM/TCG 实验启动、VMware VMDK 模板、QMP 控制与截图。
- EDK II DiskInfo/NvmExpressPassthru 协议头保留 BSD-2-Clause-Patent 版权，项目新增实现使用 GPL-3.0-or-later。
- 保持 GitHub 推送前状态，不修改宿主启动项或密钥。

## Ubuntu 安装器

- `scripts/install-ubuntu.sh`：UEFI/Ubuntu/x86_64/ESP 检查、源码完整构建、独立 EFI 目录、生产系统扫描、原安装与 BootOrder 备份，支持更新和保留默认顺序。
- Secure Boot 开启时使用本机专用 MOK + Ubuntu shim，注册前不切换默认启动项；一次重启确认后再运行完成安装。
- 支持不读写系统 ESP/NVRAM 的 `--stage` 打包路径，用于验证安装配置和文件布局。

## VMware 配置修复

- 原简化 VMX 缺少 PCIe 桥，NVMe 控制器无法分配插槽；模板与现有测试 VM 补齐 pciBridge0、pciBridge4–7/pcieRootPort。
- 准备脚本同时生成 VMX，独立 ESP 请求 VMware 实际支持的1920×1080 GOP；保留4K背景及动态布局缩放。
- 通过 localhost VNC 抓取实际固件菜单，VMware 基础启动与 GPU/NVMe 信息读取实测通过。

## 默认 Windows 与5秒倒计时

- 测试 ESP、Ubuntu 生产安装器和 Windows 实际介质配置统一使用 `timeout 5` 与 `default_selection "Windows,Microsoft"`。
- 使用 rEFInd 原生菜单倒计时；Windows 名称优先，Microsoft 名称作为自动扫描入口的备用匹配。任意键取消倒计时，Windows 缺失时回退首项。
- 安装器打包回归检查倒计时和默认选择；VMware 实际截图确认5秒显示、Windows 高亮与无输入自动跳转（测试入口是 demo EFI）。