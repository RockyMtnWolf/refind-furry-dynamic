# 后续 TODO

- [ ] PCI Display Controller 枚举，输出 vendor/device ID；建立可更新的 GPU 名称映射，避免将虚拟适配器误当宿主 GPU。
- [ ] Block I/O + Device Path + NVMe Pass Thru 获取磁盘模型、容量与连接类型，考虑固件权限和不支持协议情况。
- [ ] CPU 的 SMBIOS Type 4 备用读取（非 x86 平台目前显示 unavailable）。
- [ ] SMBIOS 内存频率、类型、插槽；处理多个 DIMM 与固件不完整表。
- [ ] 更低分辨率字体和布局自动适配；长固件名/CPU 名截断策略。
- [ ] Secure Boot ON：生成测试签名、在仿真专用 VARS 注册密钥并签署 rEFInd / demo，验证整条签名链。
- [ ] 真正 Linux/Windows EFI 引导测试；将演示入口换成实际路径。
- [ ] 在实体硬件上验证 GOP、CPUID、SMBIOS 与 Secure Boot 变量；不以虚拟机结果代替实机证据。
- [ ] 发布到用户 GitHub；选择镜像 fork 或 SourceForge 历史导入路线。
