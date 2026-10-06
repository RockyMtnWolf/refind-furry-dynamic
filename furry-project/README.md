# refind-furry-dynamic

在 Windows 桌面建立的 rEFInd 动态硬件信息主题实验工程。信息在 EFI 菜单绘制阶段读取，背景是独立的 3840×2160 PNG，不包含 CPU、内存等硬件文字。

源码基于官方 SourceForge 上游 https://git.code.sf.net/p/refind/code，基线 cf542da4459ff18535b0ae3a7910af032834e469，工作分支 `furry-dynamic`，远程 `upstream`。当前是本地工作副本，尚未在 GitHub 创建 fork 或 push。GitHub 连接器没有创建仓库接口，本机也没有已配置的 `gh`。

## 目录

- `refind/`：保留完整上游 Git 历史的独立源码仓库；改动见 `refind/refind/dynamic_info.c`。
- `theme-reference/`：聊天提供的 finn-term ZIP 解包副本，保留原始资源和许可证。
- `assets/furry-source.png`：桌面已有“霓虹狼影与紫色月环”背景副本。输入大图为 6688×3764，已整理并保存为 3840×2160，不宣称模型原生输出 4K。
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

- CPU：x86 CPUID 品牌字符串；不可用时明确显示 unavailable。
- 内存：SMBIOS 2/3 Type 17，支持扩展容量字段；表不可用或容量未知时退回 EFI memory map。后者标签明确为映射 RAM，不能当成 DIMM 安装容量。
- 固件：EFI System Table 的 FirmwareVendor、FirmwareRevision。
- UEFI：System Table Header Revision。
- Secure Boot：EFI 全局命名空间的 `SecureBoot` 变量，显示 ON / OFF / UNKNOWN，不把读取失败当 OFF。
- GPU / 磁盘：保留 provider 函数入口，显示 pending，后续增加 PCI / Block I/O / NVMe 实现。

叠层在背景绘制后、ScreenBackground 缓存前生成，因此菜单局部重绘与返回不会抹掉信息。字体和选择框保留 finn-term 紫粉配色。原字体是彩色 atlas；已适配 rEFInd 的暗背景反色规则。

Windows、Ubuntu 是明确标注的 demo EFI 菜单入口，用来验证图标/选择/启动/返回，不包含实际操作系统。请在移植到真实 ESP 时改为实际 bootmgfw.efi / grubx64.efi 路径。

## 真正发布到自己的 GitHub

官方上游位于 SourceForge，不能直接在 GitHub 上 fork 它。可在 GitHub 创建自己的**空仓库**（按个人偏好选公开或私有），导入上游历史并推送分支：

```powershell
cd .\refind
git remote add origin https://github.com/<你的账号>/refind-furry-dynamic.git
git push -u origin furry-dynamic
```

这会成为含完整上游历史的独立派生仓库，GitHub 不会显示官方 fork 标识。若希望显示 fork 标识，先在 GitHub fork 一个可信上游镜像，再把 `origin` 指向该 fork；仍保留 SourceForge `upstream` 作为权威同步源。

为了把本工程的脚本、资源和说明一并发布，源码仓库中提供 `furry-project/` 工程文件副本；可独立放到桌面运行，或用 `export-project.ps1` 导出完整开发包。大型 QEMU 依赖来自发行版，不提交运行中的 VARS。`git fetch upstream` 后先合并/变基并完整重建。

## 验证与后续

最终结果见 `evidence/VERIFICATION.md`，修改说明见 `CHANGES.md`。仅证明 QEMU + OVMF 中的行为；实体机器、Secure Boot ON 签名链、实际 Windows/Linux 引导尚未验证。后续工作见 `TODO.md`。

rEFInd 遵循 GPL-3.0-or-later；finn-term 和 fallout 图标保留其 MIT 许可证。背景来自用户已有资源。
