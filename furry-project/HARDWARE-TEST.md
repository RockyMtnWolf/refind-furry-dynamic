# 实体机器验证包与检查步骤

实体测试仍待用户在目标机器执行；本项目的 QEMU CPU/PCI/DIMM 信息属于虚拟环境。

1. 构建后 `test-esp/EFI/BOOT/` 是可拷贝到独立 FAT32 U 盘的 EFI 目录。保留原有系统 ESP 和启动项，用固件的一次性启动菜单选择 U 盘。
2. 先记录目标机器 CPU、DIMM 安装总容量/数量/配置速度、GPU PCI ID、引导磁盘型号/容量、固件名称和版本，作为对照值。
3. Secure Boot 关闭时可以测试当前未签名 EFI。开启时必须使用目标机器实际信任的签名链；项目仿真私钥不是目标机器信任的证书，不要直接导入宿主密钥数据库。
4. 对照 4K、1080p 可用 GOP 模式、选项切换、About 页面、子 loader 返回后重绘；拍摄 UEFI 菜单作为实体证据。
5. 先将 `refind.conf` 实际入口指向目标 ESP 的 `/EFI/Microsoft/Boot/bootmgfw.efi` 或发行版 shim/grub；保留系统现有 loader。启动 Ubuntu/Windows 后核对系统能够正常进入。
6. 未识别的 GPU 可以在 `themes/finn-term/gpu-names.txt` 添加经过核实的精确 PCI ID。磁盘固件不提供 Identify 时应该看到 `model unavailable`，不能填入猜测名称。

可临时在配置中加入 `log_level 2`，在同一 ESP 收集 rEFInd 日志。测试完成移除日志配置。无法进入菜单时用 `dynamic_info false` 对比上游 UI，记录固件、分辨率、PCI 控制器和 Secure Boot 状态，以便排查。

没有执行宿主重启、真实 ESP 修改、密钥注册或真实磁盘格式化。最终发布前须补齐目标实体机器证据；自动仿真验证不能替代这一步。
