# 为什么 W25N01KWZEIG_nfcb_1bit.c 能配 W25N02KWZEIR 却配不了 W25N01KWZEIG

数据来源：
- `W25N02KW_Datasheet_E_20231103.pdf` (Rev E, 2023-11-03)
- `W25N01KWxxxGTR_Datasheet_Rev.E.pdf` (Rev E, 2025-12-22)
- 现有配置 `W25N01KWZEIG_nfcb_1bit.c`（`configCmdArgs[1] = 0x1D`）
- 失败日志 `W25N01KWZEIG_fail_log_1bit.txt`（`configure-memory 0x102` → 20809 / 0x5149）

## 0. 一句话结论

这份 NFCB 实际上是"按 W25N02KW 的寄存器/几何参数"写出来的。02KW 和 01KW
虽然同属 K 系列 1.8V QspiNAND，但**页缓冲区总长度不同、parity 区尺寸不同、
device ID 不同、ODS 编码不同**。直接导致失败的是前三项：

- `pageTotalSize = 4096` / `columnAddressWidth = 11` 对应 02KW 的 2048+128（用户区到 2175 B）；
  01KW 用户区只到 2111 B，840h 以上由 IC 独占，ROM 按 4096 访问 spare 会踩进 parity 区。
- `sflashA1Size = 0x10000000`（256 MB）是 02KW 的密度，01KW 只有 128 MB (`0x08000000`)。
- Device ID：02KW = BA22h，01KW = BE21h，若探测路径校验 ID 会直接失败。

ODS 编码差异（见第 1 节）是潜在的信号完整性隐患，但不是 0x5149 的直接原因。


---

## 1. SR-2 (Bxh) 的 ODS 编码在两颗器件上不同（次要，但需留意）


| ODS-1, ODS-0 | W25N02KW (Rev E 7.2.5) | W25N01KW (Rev E 6.2.5) |
|--------------|------------------------|------------------------|
| 0, 0 | Set 1 **(default)** NMOS 40 Ω / PMOS 55 Ω | Set 1 — 25 Ω |
| 0, 1 | Set 2 45 Ω / 65 Ω | Set 2 — 33 Ω |
| 1, 0 | Set 3 55 Ω / 90 Ω | Set 3 **(default)** — 50 Ω |
| 1, 1 | Set 4 95 Ω / 155 Ω | Set 4 — 75 Ω |

注意两点：

1. **默认值不同**：02KW 出厂 ODS = 00（Set 1），01KW 出厂 ODS = 10（Set 3，见 01KW Figure 11
   "Shipment default" 表：S2=ODS-1=1, S1=ODS-0=0, S0=H-DIS=1）。
2. **阻值编排不同**：02KW 分 NMOS/PMOS 两组 Ron，01KW 只有单一 Ron 且数值序列完全不同。

现有配置写 `SR-2 = 0x1D = 0b0001_1101`：

```
S7 OTP-L = 0
S6 OTP-E = 0
S5 SR1-L = 0
S4 ECC-E = 1
S3 BUF   = 1
S2 ODS-1 = 1
S1 ODS-0 = 0
S0 H-DIS = 1
```

- 在 **02KW** 上 ODS=10 → Set 3（55/90 Ω），比默认 Set 1 更弱但仍在合理范围，
  H-DIS=1 保持 Hold 关闭，ECC-E=1/BUF=1 正确 → **能配置成功**。
- 在 **01KW** 上 ODS=10 恰好等于出厂默认 Set 3（50 Ω），语义上"看起来"也对。
  所以单看 0x1D，01KW 这一位并不是致命伤 —— 但它也说明这份配置是照 02KW 推的，
  没有针对 01KW 复核过。真正致命的是下面第 2、3 条。

**结论**：ODS 编码差异会造成"同一个字节在两颗片子上驱动强度不同"，是潜在的信号完整性风险
（尤其若后续把 serialClkFreq 从 30 MHz 提到 120/133 MHz）；但它不是 0x5149 的直接原因。

## 2. 致命差异：页/spare/parity 结构不同 → `pageTotalSize` 与 `columnAddressWidth` 错

| 项目 | W25N02KW | W25N01KW |
|------|----------|----------|
| 用户可见页长 | **2048 + 128 = 2176 B**（Byte 0…2175） | **2048 + 64 = 2112 B**（Byte 0…2111） |
| Buffer Read 末尾 | "end of the data buffer (Byte 2,176)" (7.2.7) | "end of the data buffer (Byte 2,111)" (6.2.7) |
| 列地址 | CA[11:0]，Byte address 0–2175（Figure 3） | CA[11:0]，Byte address 0–2111 |
| Spare 区 | Spare0..3，每个 UD2 4B + UD1 12B，800h–83Fh | 同样 800h–83Fh |
| Parity 区 | **840h–87Fh，每 Parity = EPC 13B + NU 3B，共 64 B** | **840h–85Fh，每 Parity = EPC 7B + NU 1B，共 32 B** |
| ECC parity 存放 | 存在 "extra 64-Byte area"（7.2.4） | 存在 "extra 32-Byte area"（6.2.4），且 840h–85Fh **由 IC 控制** |
| ECC 能力 | 8-bit / sector | 4-bit / sector |
| Data Output Structure (BUF=1) | 2,048 + 128 | 2,048 + 64 |

现有配置：

```c
.columnAddressWidth = 11u,   /* 注释写 "2048 + 128 byte spare" —— 这是 02KW 的描述 */
.pageDataSize       = 2048u,
.pageTotalSize      = 4096u,
```

- `pageTotalSize` 按 header 定义是 "usually it equals 2 ^ width of column address"，
  即 2^11 = 2048。这里填 4096 与 `columnAddressWidth = 11` **自相矛盾**（4096 = 2^12）。
- 02KW 上因为用户区确实到 2176 B（>2048），ROM 用 4096 这个"宽松上界"去做 page/spare 访问
  仍落在合法列地址内，写 spare 时不会踩到 IC 私有区；
- 01KW 上用户区只到 2111 B，而 **840h(2112) 起就是 IC 独占的 parity 区**。
  ROM 一旦按 2048+2048 的假设去写/校验 spare（例如写 DBBT/BBT 标记、
  或按 pageTotalSize-pageDataSize 计算 spare 长度 = 2048 B），
  就会向 840h 以上区域发 Random Load（84h）+ Program Execute（10h）。
  01KW 的这段地址"由 IC 控制"，程序无效或 ECC parity 被破坏，
  接着的回读/校验必然失败 → ROM 报错退出，就是日志里的 0x5149。

**必改**：
```c
.pageDataSize   = 2048u,
.pageTotalSize  = 2112u,   /* 01KW: 2048 + 64，不能是 4096 */
```
（若 ROM 要求 pageTotalSize 必须是 2^columnAddressWidth，则应把
`columnAddressWidth` 与 `pageTotalSize` 一起按 01KW 的 2112/2^12 关系重新核对，
但绝不能让 ROM 认为 2048 B 的 spare 可写。）

## 3. 致命差异：JEDEC Device ID 不同

| 器件 | MF ID | Device ID (ID15–ID0) |
|------|-------|----------------------|
| W25N02KW | EFh | **BA22h** (02KW 8.1.1) |
| W25N01KW | EFh | **BE21h** (01KW 7.1.1) |

若 ROM / blhost 的 configure-memory 流程里带了 ID 探测或白名单校验，
用 02KW 参数（BA22h）配 01KW（BE21h）会直接在探测阶段失败。
`W25N01KWZEIG_fail_log_4bit.txt` 里只读 `0x000000EF` 的探测序列返回 0x5149，
与此吻合。

## 4. 容量/块数：现有配置已按 01KW 填，但要确认没有从 02KW 抄错

| 项目 | W25N02KW | W25N01KW | 现有配置 |
|------|----------|----------|----------|
| 密度 | 2 Gbit / 256 MB | 1 Gbit / 128 MB | `sflashA1Size = 0x10000000` |
| 页数 | 131,072 | 65,536 | — |
| 块数 | 2,048 | 1,024 | `blocksPerDevice = 1024u` ✔ |
| PA 位宽 | PA[16:0]，PA[23:17] 忽略 | PA[15:0]，PA[23:16] 忽略 | `RADDR 0x14` = 20 bit ✔（对 01KW 富余，对 02KW 刚好） |
| 每块页数 | 64 | 64 | `pagesPerBlock = 64u` ✔ |

注意 `sflashA1Size = 0x10000000` = 256 MB，注释却写 "1Gbit = 128 MByte"。
1 Gbit = 128 MB = `0x08000000`。**这个值本身就是 02KW 的 256 MB**，
注释与数值不符，需按 01KW 改成 `0x08000000u`。这会影响 ROM 的地址范围校验。

## 5. ECC 状态位：mask 位置相同，但阈值语义/位数不同

两颗器件 SR-3 (Cxh) 的 ECC-1/ECC-0 都在 S5/S4，编码表也一致
（00 无错 / 01 已纠正未超阈值 / 10 不可纠 / 11 已纠正但超阈值）。
所以 `eccStatusMask = 0x30` / `eccFailureMask = 0x20` 在两颗上位置都对。

差别在：

| 项目 | W25N02KW | W25N01KW |
|------|----------|----------|
| ECC 能力 | ≤ 8 bit/sector 可纠 | ≤ 4 bit/sector 可纠 |
| BFD 位 | BFD[3:0] @10h S7–S4，默认 0100（4 bit） | BFD[2:0] @10h S6–S4，默认 011（3 bit） |
| BFR 位 | BFR[15:0]，4 bit/sector | BFR[14:12]/[10:8]/[6:4]/[2:0]，3 bit/sector |
| MBF 位 | MBF[3:0] | MBF[2:0] |

01KW 的 BFD 位宽比 02KW 少一位，默认阈值也更低（3 bit vs 4 bit）。
在 01KW 上更容易出现 ECC[1:0] = 0b11（已纠正但超阈值）。
若 ROM 用 `(status & 0x30) >= 0x20` 之类的比较，01KW 正常器件也会被判失败。
**建议**：ECC 失败判定改为精确比较 `(status & 0x30) == 0x20`。

## 6. 其他次要差异（不是当前失败原因，但迁移时要注意）

| 项目 | W25N02KW | W25N01KW |
|------|----------|----------|
| 指令条数 | 28 条 | 26 条 |
| 4-Byte Address 读命令 (0Ch/3Ch/6Ch/BCh/ECh) | **有** | **无** |
| BBM (A1h) / Read BBM LUT (A5h) / Last ECC Fail PA (A9h) | **无** | **有** |
| LUT-F 状态位 (SR-3 S6) | **无** | **有** |
| 硬件复位信令协议 (toggle /CS + 0b0101) | **无** | **有** (5.1.5) |
| 非连续读模式命名 | Sequential Read (BUF=0, 要求 ECC-E=0) | Continuous Read (BUF=0，ECC-E 任意) |
| BUF=0 时输出结构 | 2,048 + 128 | **2,048**（不含 spare） |
| 封装 | WSON 8x6 / TFBGA 24-ball | WSON 6x5 / WSON 8x6（无 BGA） |
| xxxR 后缀 | 无此说明 | **有：BUF 固定为 1，不可写** (6.2.7) |
| 出厂有效块 | Block 0–7 | Block 0–7 **和 Block 1020–1023** |

现有 1bit LUT 用到的 0x0B / 0x13 / 0x84 / 0xD8 / 0x10 / 0x0F / 0x1F / 0x06
在两颗器件上时序完全一致（0x0B: CA15-0 + 8 dummy；0x13/0x10/0xD8: 8 dummy + PA；
CA 只有 CA[11:0] 有效），**LUT 本身不需要改**。这进一步说明失败点在
`pageTotalSize` / `sflashA1Size` / ID，而不在 LUT。

另外用户是 `W25N01KWZEI**G**`（I = 工业级，G = BUF 默认 1），
写 BUF=1 是幂等操作，没问题；但如果换成 xxxR 料，BUF 写入会被忽略，
配置流程要能容忍。

---

## 7. 建议的修改（按优先级）

1. **必改** `pageTotalSize`：`4096u` → `2112u`（01KW 用户可见页 = 2048 + 64）。
   同时修正 `columnAddressWidth` 的注释（当前写的是 02KW 的 "2048 + 128"）。
2. **必改** `sflashA1Size`：`0x10000000u`（256 MB，02KW）→ `0x08000000u`（128 MB，01KW）。
3. **必改** 改完任何字节后**重算 `crcChecksum`** —— 否则 ROM 直接 CRC 失败，
   同样以 configure-memory 报错的形式出现。可用 `task9_nfcb_parse` /
   `task10_crc32_variant` 下的脚本重算。
4. **确认** ROM/工具侧的 JEDEC ID 校验是否含 **BE21h**（01KW），
   而不只是 BA22h（02KW）。
5. **建议** ECC 失败判定改为精确相等 `(SR3 & 0x30) == 0x20`，
   避免 01KW 上 0b11（已纠正但超 3-bit 阈值）被误判为不可纠。
6. **可选** `configCmdArgs[1]`：01KW 上 `0x1D` 的 ODS=10 恰好等于出厂默认 50 Ω，
   可以保留；但若把 `serialClkFreq` 提到 120/133 MHz，需按 01KW 的
   ODS 表（0,0=25 Ω / 0,1=33 Ω / 1,0=50 Ω / 1,1=75 Ω）重新选档，
   不要沿用 02KW 的 NMOS/PMOS 双值表推断。
7. **注意** 不要给 01KW 用 0Ch/3Ch/6Ch/BCh/ECh 这些 4-Byte Address 读命令
   （01KW 没有），若将来做 4bit/Quad 配置尤其要留意。

## 8. 复现验证建议

改完 1–3 项并重算 CRC 后：

```
blhost -u 0x1fc9,0x0026 -- write-memory 0x20020000 .\W25N01KWZEIG_nfcb_1bit.bin
blhost -u 0x1fc9,0x0026 -- configure-memory 0x102 0x20020000
```

若仍报 0x5149，用 `fill-memory` 手工分段定位：

1. 只读 JEDEC ID（9Fh + dummy），期望 **EF BE 21**（不是 EF BA 22）；
2. 读 SR-1/SR-2/SR-3（0Fh + Axh/Bxh/Cxh）：
   确认 `A0h = 0x00`（BP[3:0]/TB 已解锁）、`B0h = 0x1D`、`C0h` 的 BUSY=0；
3. 单页 13h（8 dummy + PA）→ 0Bh（CA + 8 dummy）读回 Byte 0…2111，
   确认数据非全 0xFF / 非全 0，且读到 2112 B 之后 DO 进入 Hi-Z；
4. 单独验证 spare 写入：只对 800h–83Fh 做 84h + 10h，**不要触碰 840h 以上**，
   回读确认成功。若步骤 4 在 840h 以上失败、在 83Fh 以内成功，
   就直接证实了第 2 节的 `pageTotalSize` 结论。
