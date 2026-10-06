# 仿真验证记录

日期：2026-10-06（Asia/Shanghai）。所有图片直接使用 QEMU QMP `screendump` 从真实 GOP framebuffer 捕获，再无损转换为 PNG；没有用 Pillow 合成菜单。

环境：Windows + 已有 WSL Ubuntu-24.04；QEMU 8.2.2 / TCG，GNU-EFI 3.0.15，GCC 13.2，OVMF 2024.02-2ubuntu0.10。q35、SMM、64 MiB VGA、3840×2160 GOP。固件使用 Secure Boot-capable CODE 和独立空白 VARS，未注册签名密钥。

| 验证 | 结果 |
|---|---|
| 完整 clean 构建 | 成功；新增动态模块无编译告警。上游 EfiLib 的 PreviousHandle 未使用告警仍存在 |
| 实际 4K framebuffer | 3840×2160 |
| CPU= max，RAM=2048 | 显示 QEMU TCG CPU version 2.5+、2048 MiB [SMBIOS] |
| CPU= qemu64，RAM=4096 | 显示 QEMU Virtual CPU version 2.5+、4096 MiB [SMBIOS] |
| 固件 | Ubuntu distribution of EDK II / revision 0x10000；UEFI 2.70 |
| SecureBoot EFI 变量 | Secure Boot-capable OVMF 中 OFF；普通 OVMF 缺少变量时 UNKNOWN |
| 主题 | 无文字 furry banner，紫粉动态文本、Ubuntu/Windows 图标、选择框和工具图标 |
| demo 启动 | 选择入口实际执行 demo.efi，串口输出 No operating system installed；按键返回后重绘菜单 |

两个配置使用相同 EFI 二进制与背景，没有修改资源或重编译。这证明 CPU/内存来自启动时数据，而不是烘焙在 PNG 中。

截图：`qemu-2048-4k.png`、`qemu-4096-4k.png`、`demo-loader.png`、`menu-return.png`。最终源码和二进制对应关系由 binary.sha256 记录。

不涵盖：实体机器；Secure Boot ON 的实际签名链；真实 Windows/Linux 操作系统启动；GPU/磁盘 provider；EFI memory map 回退在该 OVMF 配置下未触发。当前 Windows/Ubuntu 入口只启动 demo EFI。
