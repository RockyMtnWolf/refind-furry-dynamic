# 第二阶段验证记录

测试日期：2026-10-06/07（本机时区 Asia/Shanghai）。源码功能提交 `4f0b5af`。

## 构建与范围

- WSL Ubuntu-24.04、GCC 13.2、GNU-EFI 3.0.15，完整清理后重建 x86_64 EFI 成功。
- 最终未签名 `BOOTX64.EFI` SHA256：`0dfce9f52cfe5858a190c8bccd9f852ebad47d923760cf82587d2b08fe170547`。
- QEMU 8.2.2 / OVMF 2024.02，q35/SMM、VGA 64 MiB；KVM `host` CPU、一颗虚拟 CPU、隔离测试 VARS。签名版本由同一二进制派生，签名改变文件哈希。
- 测试 VM 无网络，不挂载实体硬盘。密钥只加入测试 VARS；没有写入宿主 ESP、NVRAM 或 Secure Boot 密钥。
- `test.sh` 对生产 SMBIOS 解析器运行 ASan/UBSan：多 DIMM、最低速度、扩展/KiB/未知容量、CPU Type 4 备用、校验和、短记录、缺失结束符和逐字节截断全部通过。记录见 `parser-tests.txt`。
- Shell/Python/PowerShell 脚本语法检查通过。构建中的 `BdsTianoCore.c` unused-variable 警告来自上游。

## 系统引导

### Windows

`windows-menu.png`：最终二进制、1920×1080，Secure Boot ON、6144 MiB，GPU `[1234:1111]`，NVMe 型号 QEMU NVMe Ctrl，12.8 GB 容量。

rEFInd 手动入口加载真实 Microsoft EFI boot manager，进入 Windows 11 ISO 的 WinPE。`windows-uefi-proof.png` 中运行：

```text
wpeutil UpdateBootInfo
reg query HKLM\System\CurrentControlSet\Control /v PEFirmwareType
```

显示 Windows `10.0.26100.8037`，更新成功，`PEFirmwareType REG_DWORD 0x2`，确认 UEFI 引导。`windows-winpe-console.png` 保留实际安装环境窗口。测试未安装完整 Windows；安装界面曾要求介质驱动，之后已补全 sources 子目录，未把这一修复宣称为完整安装验证。

### Ubuntu

`ubuntu-menu.png`：最终二进制、1920×1080，6144 MiB、Secure Boot ON、NVMe 512 MiB，额外整盘计数包含只读光盘。真实 Ubuntu ISO shim → GRUB → 签名 Linux 内核，原厂签名未替换。使用 ISO 的 casper/initrd 启动 Live 系统。

已通过 Ctrl+Alt+F3 进入 tty3，以 Live 用户 ubuntu 登录。`ubuntu-uefi-proof.png` 显示 `uname -a` 为 Linux 7.0.0-30-generic，`ls /sys/firmware/efi` 可读取 EFI runtime/efivars，`mokutil --sb-state` 返回 `SecureBoot enabled`。确认实际 Live 用户空间、UEFI 与签名链，没有把仅加载内核视为完整验证。

启动日志见 `serial-ubuntu.log`。跨 `/mnt/c` 的镜像随机读取和逐字符串口日志曾明显拖慢系统；运行脚本改为 WSL 本地顺序缓存镜像，串口日志也写入 `/tmp`，控制/退出时同步回工程。ISO 缓存使用临时文件后重命名，并校验长度/时间，避免中断留下的不完整缓存。

## 安全启动对照

`prepare-secure-lab.sh` 将测试证书加入 PK/KEK/db，并包含微软 2011 CA。`signature.txt` 为同证书验证结果，`enrolled-vars.txt` 为测试变量列表。

签名子程序能执行；未签名子程序被固件拒绝，`unsigned-child-rejected.png` 与 `serial-reject.log` 保留拒绝证据。该实验验证固件签名策略，没有绕过 Secure Boot。

## 布局和提供者回归

- `final-4k.png`：最终二进制、3840×2160、4096 MiB、Secure Boot ON。CPUID 为 KVM 传递的宿主品牌字符串；显卡和磁盘仍是虚拟设备。
- `signed-child.png`：最终二进制启动同证书签名 demo。返回后按 Esc 重新扫描，`rescan-4k.png` 保持动态文本、字体与图标尺寸正常。
- `smbios-sata-720p.png`：1280×720，关闭 CPUID 品牌字符串；显示合成 Type 4 `SMBIOS-Fallback-Test`，两条合成 DDR5 DIMM 共4096 MiB、2/2、6000 MT/s。磁盘切换到 SATA 后，型号正确变为 QEMU HARDDISK、504 MiB，Secure Boot OFF。
- `mapped-memory-1080p.png`：Type 17 容量为未知值 0xffff，实际配置4096 MiB；正确退回到4078 MiB `EFI mapped RAM`，没有将它冒充安装容量。CPU 同时切换为 TCG 的品牌字符串，证明文本来自当前 VM。

合成备用路径的 Linux 复现命令（先 `lab.ps1 -Action fixtures`）：

```bash
MODE=off WIDTH=1280 HEIGHT=720 RAM=4096 ACCEL=tcg CPU=max,model-id= \
SMBIOS_CPU=SMBIOS-Fallback-Test DISK_BUS=sata \
SMBIOS_FILE="$PWD/vm-tests/fixtures/two-dimms.bin" bash scripts/run-lab.sh
# 退出上一场景后测试未知容量：
MODE=off WIDTH=1920 HEIGHT=1080 RAM=4096 ACCEL=tcg CPU=max \
SMBIOS_FILE="$PWD/vm-tests/fixtures/unknown-memory.bin" bash scripts/run-lab.sh
```

## 复现

在工程根目录运行 PowerShell `scripts/lab.ps1`；默认 QMP 4450、VNC 5910。先退出当前实验再准备或切换场景。

```powershell
.\scripts\build.ps1
.\scripts\lab.ps1 -Action prepare
.\scripts\lab.ps1 -Action test
.\scripts\lab.ps1 -Action fixtures
.\scripts\lab.ps1 -Action on
# 在第二个终端截图或退出：
.\scripts\lab.ps1 -Action capture -Output phase2/repeat.png
.\scripts\lab.ps1 -Action quit
```

真实系统介质准备步骤见 README。`SMBIOS_FILE` fixture 为合成测试数据，不是本机 DIMM 规格。

## 尚未验证

- 实体硬件 GOP、GPU/NVMe 型号与 SMBIOS 差异，按 `HARDWARE-TEST.md` 另行测试。
- VMware 内 Ubuntu/MOK 的 Secure Boot 链及第二块 Windows 系统盘完整启动尚未验证；基础菜单结果见下方补测。
- Hyper-V：vmms 运行，但 Get-VM 返回授权策略权限错误；当前进程不在 Administrator 角色，没有修改现有 VM。
- 不声称已安装 Ubuntu/Windows 到硬盘或验证了完整系统功能。
- 没有添加 GitHub origin、在线 fork 或 push；本次要求停在远程推送之前。

## VMware 补测（2026-10-07）

用户手动启动暴露 `No PCIe slot available for nvme0`。原简化 VMX 没有 PCIe root port；补齐 pciBridge0 与 pciBridge4–7 后，vmrun start 成功。

VMware Workstation 25.0.0 build-24995812，虚拟硬件21、EFI、4 GiB RAM、2 vCPU，512 MiB NVMe GPT/FAT32测试盘。当前固件提供1920×1080 GOP但没有3840×2160；专用主题配置改为1080p，背景仍为4K。

`vmware-menu.png` 是实际 VNC framebuffer：Firmware VMware, Inc.、Secure OFF、GPU VMware SVGA II `[15ad:0405]`、磁盘 VMware Virtual NVMe Disk / 512 MiB / NVMe。内存4096 MiB，VMware SMBIOS报告1/128逻辑槽位、4800 MT/s，不代表实体机器 DIMM 插槽数。CPU为传递的 i9-13905H 品牌字符串。

授权服务当前可用，之前权限阻塞记录已解决。VIX captureScreen 需要 guest 登录，改用专用 localhost VNC 截图，不依赖 guest OS 或 VMware Tools。没有因此声称 VMware Secure Boot/MOK 或完整 Windows 系统启动已通过。

## Windows 默认项与倒计时补测（2026-10-07）

使用未改动 EFI 二进制，测试配置 `timeout 5`、`default_selection "Windows,Microsoft"`。VMware 冷启动过程中仅请求 VNC framebuffer，没有发送键盘或鼠标输入。

- `vmware-windows-countdown-5s.png`：Windows 默认高亮，原生菜单文字 `Automatic boot in 5 seconds`。
- `vmware-windows-countdown-2s.png`：Windows 保持选中，倒计时到2秒。
- `vmware-windows-timeout-loader.png`：倒计时结束自动进入 demo EFI。这里证明默认入口与超时跳转，不表示已经启动完整 Windows；真实安装配置扫描 Windows/Microsoft loader。
- Ubuntu 安装器 `test-install-ubuntu.sh` 两次打包检查均通过，包含timeout/default_selection精确检查以及实际文件布局、二进制/背景一致性与非法PE拒绝。