# 后续 TODO

- [x] PCI Display Controller/GOP 枚举、真实 ID、精确匹配的可更新 GPU 名称映射。
- [x] Block I/O + Device Path + Disk Info/NVMe Identify 获取型号、容量、连接类型；未知协议/型号明确标记 unavailable。
- [x] CPU CPUID 空值时 SMBIOS Type 4 备用读取；宿主解析器回归已通过。
- [x] SMBIOS 内存类型、插槽、配置速度、扩展/KiB/未知容量；多 DIMM、校验和与截断数据回归已通过 ASan/UBSan。
- [x] 720p/1080p/4K 字体与图标适配，长行省略；修复重新扫描后的字体缓存与尺寸恢复。
- [x] 仿真 Secure Boot ON 签名链与未签名子程序拒绝对照。
- [x] 真正 Linux/Windows EFI 引导测试；独立测试 ESP 使用实际 shim/boot manager 路径。Ubuntu Live 登录后确认 UEFI/SecureBoot enabled；WinPE 确认 PEFirmwareType=2。
- [ ] 在实体硬件上验证 GOP、CPUID、SMBIOS 与 Secure Boot 变量；不以虚拟机结果代替实机证据。
- [x] VMware 第二平台基础验证：2026-10-07 授权服务可用，补齐 PCIe 桥后 NVMe 启动成功；1080p菜单正确读取 VMware 固件、GPU 与 NVMe。
- [ ] VMware 内 Ubuntu/MOK Secure Boot 与第二块 Windows 系统盘完整启动。
- [ ] Hyper-V 第二平台验证：vmms 已运行，但 Get-VM 实测被当前账户权限拒绝，尚未创建或修改 Hyper-V VM。
- [ ] 发布到用户 GitHub：用户本次要求停在 push 之前，保留 upstream 与 furry-dynamic 本地分支。发布步骤见 README。
