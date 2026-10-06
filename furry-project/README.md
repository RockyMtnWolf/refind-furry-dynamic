# refind-furry-dynamic

在 Windows 桌面建立的 rEFInd 动态硬件信息主题实验工程。信息在 EFI 菜单绘制阶段读取，背景是独立的 3840×2160 PNG，不包含 CPU、内存等硬件文字。

源码基于官方 SourceForge 上游 https://git.code.sf.net/p/refind/code，基线 cf542da4459ff18535b0ae3a7910af032834e469，工作分支 `furry-dynamic`，远程 `upstream`。当前是本地工作副本，尚未在 GitHub 创建 fork 或 push。GitHub 连接器没有创建仓库接口，本机也没有已配置的 `gh`。

## 目录

以下按桌面工程/导出包布局说明。在源码仓库内直接使用 `furry-project/scripts/` 时，脚本自动从父目录查找 rEFInd 源码。

- `refind/`：保留完整上游 Git 历史的独立源码仓库；改动见 `refind/refind/dynamic_info.c`。
- `theme-reference/`：聊天提供的 finn-term ZIP 解包副本，保留原始资源和许可证。
- `assets/furry-source.png`：桌面已有“霓虹狼影与紫色月环”大图副本，6688×3764；运行时整理为 3840×2160，不宣称模型原生输出 4K。
- `theme/`：运行时 finn-term 主题资源，背景没有硬件数据。
- `scripts/`：依赖安装、资源准备、构建、启动、QMP 截图与控制。
- `test-esp/`：仅用于仿真的 FAT 目录，包含 `EFI/BOOT/BOOTX64.EFI`、配置与演示 EFI。
- `firmware/`：OVMF 固件和独立可写 VARS 副本。
- `evidence/`：构建日志、二进制 SHA256 和实际 QEMU 截图。

## 快速运行（PowerShell）

```powershell
cd "$env:USERPROFILE\Desktop\refind-furry-dynamic"
.\scripts\setup.ps1          # 首次安装 WSL 依赖；使用现有 Ubuntu-24.04
.\scripts\build.ps1
.\scripts\run.ps1 -Window  # 使用 WSLg 的 QEMU GTK 窗口
```

无需窗口时执行 `.\scripts\run.ps1`，通过 QMP 截图查看。启动命令会保持运行，在第二个 PowerShell 中：

```powershell
.\scripts\capture.ps1 -Output qemu-4k.png
.\scripts\capture.ps1 -Key right
.\scripts\capture.ps1 -Key ret
.\scripts\capture.ps1 -Quit
```

截图位于 `evidence/`。QMP 默认 localhost:4444；VNC 默认 localhost:5909，可用 VNC 查看器交互。启动早期可能暂时显示未初始化显示器，通常等待 30–60 秒后再截图。不要同时运行两个实例，因为会争用 VARS、端口和测试 ESP。

2 GiB/4 GiB 对照：先退出当前仿真，再运行 `.\scripts\run.ps1 -MemoryMiB 4096`。Linux 原生命令为 `RAM=4096 bash scripts/run.sh`。CPU 可通过 Linux 环境变量 `CPU=qemu64` 改变。CPU 与内存反映的是虚拟机，并非 Windows 宿主。

当前 Windows 未发现原生 QEMU，故构建和实际仿真均使用现有 WSL Ubuntu-24.04，遵循上游推荐的 Linux/GNU-EFI 构建方式。无需修改 Windows 启动项或真实 ESP。4K 模式用 VGA 64 MiB 显存与 3840×2160 GOP 请求验证。

需要重做资源时：`python scripts/prepare_assets.py`（需 Pillow）。然后重新构建。**不要运行原主题 `theme/src/gen.py`，它会重新生成写死数据的背景。**

## 动态信息

`dynamic_info true` 启用叠层；删除这一行或设置 `false` 后恢复普通 rEFInd 主题行为。

- CPU：优先读取 x86 CPUID 品牌字符串，空值或不支持时读取 SMBIOS Type 4 的 Processor Version；均不可用才显示 unavailable。
- 内存：SMBIOS 2/3 Type 17，支持扩展容量、KiB 单位、DIMM 数量、DDR3/4/5 和配置速度；多条速度显示最低值。表不可用或容量未知时退回 EFI memory map（排除 reserved、unusable 和 MMIO）。后者标签明确为映射 RAM，不能当成 DIMM 安装容量。
- 固件：EFI System Table 的 FirmwareVendor、FirmwareRevision。
- UEFI：System Table Header Revision。
- Secure Boot：EFI 全局命名空间的 `SecureBoot` 变量，显示 ON / OFF / UNKNOWN，不把读取失败当 OFF。
- GPU：PCI I/O 枚举 Display Controller，优先带 GOP 的设备，显示真实 PCI vendor/device ID。`theme/gpu-names.txt` 使用 `vvvv:dddd|名称` 精确匹配；未知型号显示厂商与 ID，不猜型号。虚拟显卡明确标记 virtual。
- 磁盘：Block I/O 枚举整盘，优先当前引导盘；通过 Device Path 判断 NVMe/SATA/USB/SCSI。Disk Info 或只读 NVMe Identify Controller 获取型号，容量来自 Block I/O；不支持的固件明确显示 model unavailable。GB 使用十进制，MiB 使用二进制。

叠层在背景绘制后、ScreenBackground 缓存前生成，因此菜单局部重绘与返回不会抹掉信息。字体和选择框保留 finn-term 紫粉配色。原字体是彩色 atlas；已适配 rEFInd 的暗背景反色规则。

默认 ESP 的 Windows、Ubuntu 是标注的 demo EFI 菜单入口，用来验证图标/选择/启动/返回。系统测试使用独立 `vm-tests/` 介质与实际 loader 配置，见下面的实验说明。背景、字体、图标按实际 GOP 分辨率缩放，信息最长占屏幕左侧 62%，超长文本加省略号。

## 扩展验证实验

```powershell
.\scripts\lab.ps1 -Action prepare # 新建/重置隔离签名环境，必须先退出当前实验
.\scripts\lab.ps1 -Action test    # 生产 SMBIOS 解析器的 ASan/UBSan 测试
.\scripts\lab.ps1 -Action on      # KVM；当前 WSL 中用 root 访问 /dev/kvm
# 第二个 PowerShell：
.\scripts\lab.ps1 -Action capture -Output phase2/latest.png
.\scripts\lab.ps1 -Action quit
.\scripts\lab.ps1 -Action reject  # 签名 rEFInd + 未签名子程序，回车验证拒绝
.\scripts\lab.ps1 -Action off -Width 1280 -Height 720
```

实验 QMP 为 localhost:4450，VNC 为 localhost:5910。单个实验锁防止同时使用同一环境。KVM 默认一颗虚拟 CPU；本机嵌套 SMP 启动曾卡住，可用 `-Accelerator tcg`。VARS 和逐字符串口日志写入 WSL `/tmp`，避免 Windows 挂载目录的小写入拖慢固件/内核。截图/控制时同步日志，正常退出后保存 VARS 和最终日志。若曾强制中断固件初始化，用 `prepare` 重建专用 VARS 后再试。

签名密钥只存 `vm-tests/keys/`，仅加入仿真 VARS；没有修改 Windows 宿主 UEFI 密钥或启动项。实验数据库包含测试证书与微软 2011 CA，方便实际安装介质验证；它是开发测试配置，不是发布用的安全策略。

Ubuntu：先用 root 执行 `scripts/prepare-ubuntu-lab.sh`（ISO 默认指向本机 Downloads，可设置 `UBUNTU_ISO`），再运行：

```powershell
.\scripts\lab.ps1 -Action ubuntu -Width 1920 -Height 1080 -MemoryMiB 6144 -IsoPath "$env:USERPROFILE\Downloads\ubuntu-26.04.1-desktop-amd64.iso"
```

Ubuntu/Windows 使用带 GPT/FAT32 的实际磁盘镜像。启动前将镜像和 ISO 顺序复制到 WSL 本地缓存，再进行大量随机扇区访问；Windows 挂载目录下的 vvfat 在多个 loader 场景出现目录读取错误，因此系统测试不依赖它。运行副本上的磁盘写入留在 WSL 临时副本，安装介质主副本保持可重复测试状态。

Windows：用 root 执行 `prepare-windows-media.sh`（自动只读挂载 Downloads ISO，可设置 `WINDOWS_ISO`），待完成后执行 `attach-windows-refind.sh`，最后运行 `lab.ps1 -Action windows -Width 1920 -Height 1080 -MemoryMiB 6144`。脚本只格式化新建的 12 GiB 镜像，在 WSL ext4 完成随机 FAT 写入后导出到项目；分割 install.wim 为 FAT32 可容纳的 SWM，保留原厂签名 boot manager/WinPE 和 sources 子目录。验证范围是启动进入 WinPE 与 UEFI 类型，不表示已经安装 Windows 到虚拟硬盘。

SMBIOS 回归场景由 `make-smbios-fixtures.py` 生成**合成固件数据**，不是本机硬件规格。Linux 环境变量 `SMBIOS_FILE`、`SMBIOS_CPU`、`CPU` 与 `DISK_BUS=sata` 可验证多 DIMM、CPU 备用读取、EFI memory map 退回与 SATA 型号。命令及观察值记录在 `evidence/phase2/VERIFICATION.md`。

VMware：`prepare-vmware.sh` 生成独立 VMDK；模板位于 `scripts/vmware-test.vmx`，复制到 `vm-tests/vmware/refind-test.vmx` 即可用 vmrun 启动和 captureScreen。当前机器 VMware Authorization Service 停止，当前会话启动服务无权限，故没有 VMware 通过证据；QEMU 实验正常运行。

Hyper-V 的 vmms 已运行，但本次 `Get-VM` 实测返回授权策略权限错误，未创建或修改现有 Hyper-V VM。第二平台结果保留为待验证。

## 真正发布到自己的 GitHub

官方上游位于 SourceForge，不能直接在 GitHub 上 fork 它。可在 GitHub 创建自己的**空仓库**（按个人偏好选公开或私有），导入上游历史并推送分支：

```powershell
cd .\refind
git remote add origin https://github.com/<你的账号>/refind-furry-dynamic.git
git push -u origin furry-dynamic
```

这会成为含完整上游历史的独立派生仓库，GitHub 不会显示官方 fork 标识。若希望显示 fork 标识，先在 GitHub fork 一个可信上游镜像，再把 `origin` 指向该 fork；仍保留 SourceForge `upstream` 作为权威同步源。

为了把本工程的脚本、资源和说明一并发布，源码仓库中提供 `furry-project/` 工程文件副本，在仓库内可直接运行；需要移到桌面时，用 `export-project.ps1` 导出包含源码历史的完整开发包。大型 QEMU 依赖来自发行版，不提交运行中的 VARS。`git fetch upstream` 后先合并/变基并完整重建。

## 验证与后续

第一阶段结果见 `evidence/VERIFICATION.md`；扩展实验结果见 `evidence/phase2/VERIFICATION.md`，修改说明见 `CHANGES.md`。实体固件兼容性须按 `HARDWARE-TEST.md` 单独验证；虚拟机结果不代替实体机器证据。尚未创建 GitHub origin 或推送，符合本次“停在推送之前”的要求。

rEFInd 遵循 GPL-3.0-or-later；finn-term 和 fallout 图标保留其 MIT 许可证。背景来自用户已有资源。
