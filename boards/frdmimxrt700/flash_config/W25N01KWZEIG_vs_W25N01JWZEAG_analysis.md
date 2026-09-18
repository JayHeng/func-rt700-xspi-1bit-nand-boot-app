# W25N01JWZEAG 可配置、W25N01KWZEIG 失败的原因分析

数据来源：
- `W25N01JWxxxGT_RevD_220921.pdf` (Rev D, 2022-09-21)
- `W25N01KWxxxGTR_Datasheet_Rev.E.pdf` (Rev E, 2025-12-22)
- 现有配置 `W25N01JWZEAG_nfcb_1bit.c`
- 失败日志 `W25N01KWZEIG_fail_log_1bit.txt` / `_4bit.txt`
  （`configure-memory 0x102` 返回 20809 / 0x5149）

## 1. 关键结论

现有 NFCB 里 **`configCmdArgs[1] = 0x18` 写 SR-2(0xB0)** 这一步是按 J 系列
（W25N01JW）的 SR-2 位定义构造的。K 系列（W25N01KW）的 SR-2 位定义完全不同，
同一个 0x18 在 KW 上含义被改变，直接导致 ROM 后续的 page read / 校验失败，
`configure-memory` 报错退出。这是最主要、也是最确定的差异点。

### SR-2 (地址 Bxh) 位定义对比

| 位 | W25N01JW (Rev D 7.2) | W25N01KW (Rev E 6.2) |
|----|----------------------|----------------------|
| S7 | OTP-L                | OTP-L                |
| S6 | OTP-E                | OTP-E                |
| S5 | SR1-L                | SR1-L                |
| S4 | ECC-E                | ECC-E                |
| S3 | BUF                  | BUF                  |
| S2 | Reserved (R)         | **ODS-1**            |
| S1 | Reserved (R)         | **ODS-0**            |
| S0 | **QE (Quad Enable)** | **H-DIS (Hold Disable)** |

写入 0x18 = 0b0001_1000 的实际效果：

- 在 JW 上：ECC-E=1, BUF=1, QE=0（Quad 关掉，/WP、/HOLD 保留，正是 1bit 配置想要的）。
- 在 KW 上：ECC-E=1, BUF=1，但同时
  - ODS-1=0, ODS-0=0 → 输出驱动强度被从默认 Set 3 (50 Ω) 改成 Set 1 (25 Ω)；
  - **H-DIS=0 → Hold 功能被使能**（KW 出厂默认 H-DIS=1，即 Hold 关闭）。

KW 上 IO3//HOLD 若在板上没有上拉（或被 XSPI 控制器在 1bit 模式下留作浮空/被驱低），
使能 Hold 后只要 /HOLD 被拉低，器件在 /CS 有效期间会被挂起：DO 变高阻、CLK/DI 被忽略。
ROM 随后读 SR-3 / 读页数据得到 0xFF 或超时，于是 `configure-memory` 失败。
同时 KW 没有 QE 位（Quad 由 WP-E 控制，见 6.1.2），所以原注释里"把 QE 写 0 关掉 Quad"
在 KW 上根本不成立 —— KW 的 Quad 一直是使能的，除非置 WP-E=1。

**KW 上正确的 SR-2 值应为 `0x19`**（ECC-E=1, BUF=1, ODS=00, H-DIS=1，保持 Hold 关闭），
更保守的是 `0x1D`（ECC-E=1, BUF=1, ODS-1=1/ODS-0=0 保持默认 50 Ω, H-DIS=1）。
注意 `W25N01JWZEAG_nfcb_4bit.c` 用的正好是 0x19（在 JW 上是 QE=1），
这也解释了为什么 4bit 版本在 KW 上"看起来"更接近正确但仍失败（见下）。

## 2. 其他必须一并修改/确认的差异

### 2.1 JEDEC ID 不同（若 ROM/工具做 ID 检查则直接失败）
- W25N01JW: MF=EFh, Device ID = **BC21h**（Rev D 8.1.1）
- W25N01KW: MF=EFh, Device ID = **BE21h**（Rev E 7.1.1）

`W25N01KWZEIG_fail_log_4bit.txt` 中那条只读 `0x000000EF` 的探测序列返回
0x5149，说明 ID/探测路径也需要按 BE21h 适配。

### 2.2 Spare 区结构与 ECC parity 位置不同 → `pageTotalSize` 语义变化
- JW（Rev D 7.2.4）：页 = 2048 + 64 B，**ECC parity code (EPC) 就放在这 64 B spare 内**
  （每个 spare 16 B = UD2 8B + UD1 4B + EPC 4B）。
- KW（Rev E 2. FEATURES / 6.2.4）：页 = **2048 + 64 B + 额外 32 B 由 Flash 内部管理**用于
  ECC parity；spare 64 B 里 **只有 UD2/UD1，没有 EPC**，parity 在 840h–85Fh 由 IC 控制，
  用户不可见/不可用。

所以在 KW 上：
- 主机可访问列地址范围仍是 0x000–0x83F（2112 B），但 `columnAddressWidth=11`（2048 B 概念）
  与 `pageTotalSize=4096` 的取法需要重新确认；
- 更重要的是：不能像 JW 那样把 spare 的 EPC 区当成可写数据，否则 program/verify 会异常。

### 2.3 ECC 能力与 ECC 状态位含义不同 → `eccStatusMask/eccFailureMask` 需重算
- JW（Rev D 7.3.2）：Hamming，**1-bit 纠错 / 2-bit 检测**。
  SR-3 ECC[1:0]：00 无错，01 纠正 1bit，10 单页 2bit 不可纠，11 多页 2bit 不可纠。
- KW（Rev E 6.3.2）：**4-bit/sector ECC**，SR-3 ECC[1:0]：
  00 无 bit flip，01 已纠正且未超阈值，**10 多 bit 不可纠**，
  **11 = 已纠正但超过 BFD 阈值（数据仍然有效，只是建议刷新）**。

当前配置：
```c
.eccStatusMask  = 0x30,  /* SR-3 bit[5:4] */
.eccFailureMask = 0x20,  /* 只有 0b10 视为失败 */
```
mask 位置对两者都对（都是 S5/S4），但语义上在 KW 上 `0b11` 是"可用但需刷新"，
若 ROM 用 `(status & mask) >= failureMask` 或按位判断，KW 上正常老化后的页
会被误判为 ECC 失败。建议在 KW 上按"仅 0b10 为失败"精确比较。

另外 KW 新增了 BFD/BFS/BFR/MBF 扩展 ECC feature 寄存器（地址 10h/20h/30h/40h/50h，
Rev E 6.4），默认 BFD=011（3 bit 阈值）。这些寄存器 JW 上不存在，可以不配，
但要意识到 KW 出厂默认就会在 3-bit flip 时把 ECC[1:0] 报成 11。

### 2.4 KW 没有 J 系列的一堆命令 → LUT 里不能沿用
KW（Rev E 7.1.2/7.1.3）指令集中 **没有**：
- 4-Byte Address 系列（0Ch/3Ch/6Ch/BCh/ECh）
- 全部 DTR 读命令（0Dh/0Eh/3Dh/6Dh/BDh/BEh/EDh/EEh）
- Write Data Learning Pattern (4Ah)、SR-4 (Dxh)、Extended Register、HS 位、DLP-E

当前 1bit LUT 只用到 0x0B/0x13/0x84/0xD8/0x10/0x0F/0x1F/0x06，
这些在 KW 上都存在且时序一致（0x0B Buffer Read: CA15-0 + 8 dummy clk；
0x13/0x10/0xD8: 8 dummy + PA15-0；CA 只有 CA[11:0] 有效；PA 16 bit），
**因此 LUT 本身不需要改**。这也侧面证明失败不在 LUT，而在 configCmd（SR-2 写值）。

KW 还有一点：Block Erase (D8h) 的地址被文档写成 PA23-16(忽略)+PA15-8+PA7-0，
即"8 dummy + 16bit PA"，与现有 `MODE4 + MODE4? + RADDR 0x14` 编码等价，无需改。

### 2.5 xxxG / xxxT / **xxxR** 型号后缀差异
- JW 只有 G（BUF 默认 1）和 T（BUF 默认 0）。
- KW 增加了 **R：BUF 固定为 1，不可写**（Rev E 6.2.7）。
  `W25N01KWZEIG` 是 **I** 温度等级 + **G** 后缀（BUF 默认 1），所以 BUF 本身没问题；
  但如果拿到的是 xxxR 料，写 BUF 会被忽略，配置流程也要能容忍这一点。

### 2.6 KW 无 /RESET 引脚
JW 的 SOIC/TFBGA 有专用 /RESET（Rev D 6.1.7），KW 只有 8-pad WSON、**只有软复位**
（FFh 或 66h+99h，Rev E 7.2.1）。若 ROM 依赖硬复位恢复，KW 上需改用软复位。

## 3. 建议的修改（按优先级）

1. **必改**：新建 `W25N01KWZEIG_nfcb_1bit.c`，把 `configCmdArgs[1]` 从 `0x18` 改为
   `0x19`（或 `0x1D` 保持默认 ODS），确保 KW 的 H-DIS 保持为 1。
   同时更新注释：KW 的 SR-2 S0 是 H-DIS 不是 QE，S2/S1 是 ODS。
2. **必改**：重新计算 `crcChecksum`（改了任何字节都要重算，否则 ROM 直接 CRC 失败，
   这本身也会报 configure-memory 错误）。可用 `task9_nfcb_parse` / `task10_crc32_variant`
   下的脚本重算。
3. **确认**：ROM/工具侧的 JEDEC ID 检查是否硬编码 BC21h，需要加入 BE21h。
4. **确认**：`pageTotalSize` / spare 区使用方式是否与 KW 的
   "2048+64 用户可见 + 32 内部 parity" 结构一致（KW 上 EPC 不在用户 spare 里）。
5. **建议**：ECC 失败判定改为"仅 ECC[1:0]==0b10 为不可纠"，避免 KW 上 0b11
   （超阈值但已纠正）被误判。
6. **可选**：若想用 4bit，KW 上不要通过写 QE 来开 Quad（KW 无 QE 位），
   而应保证 WP-E=0（默认即 0），并直接用 6Bh/34h；`_4bit.c` 里 `0x19` 在 KW 上
   恰好等于 ECC-E=1/BUF=1/ODS=00/H-DIS=1，语义可用，但 ODS 被改成 25 Ω，
   在 120 MHz 下可能引入信号完整性问题，建议改用 `0x1D`。

## 4. 复现验证建议

修 SR-2 值 + 重算 CRC 后，按下列顺序验证：
```
blhost -u 0x1fc9,0x0026 -- write-memory 0x20020000 .\W25N01KWZEIG_nfcb_1bit.bin
blhost -u 0x1fc9,0x0026 -- configure-memory 0x102 0x20020000
```
若仍报 0x5149，再用 `fill-memory` 手工序列单独验证：
1. 只读 JEDEC ID（期望 EF BE 21）；
2. 读 SR-1/SR-2/SR-3（0Fh + Axh/Bxh/Cxh），确认 BP[3:0]/TB 已被 0xA0=0x00 解锁、
   SR-2 值确为预期；
3. 单页 13h + 0Bh 读回，确认数据非 0xFF/非全 0。
这样可以把"寄存器配置失败"与"读时序失败"区分开。
