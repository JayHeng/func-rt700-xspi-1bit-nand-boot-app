/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "flash_config.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.nand_config"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE == 1)
#if defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".flash_conf"), used))
#elif defined(__ICCARM__)
#pragma location = ".flash_conf"
#endif

/* W25N01JWZEAG (1Gbit 1.8V Serial NAND) NAND Flash Config Block (NFCB).
 * Decoded from W25N01JWZEAG_nfcb.bin: 1-bit SPI commands with Quad (1-1-4)
 * page read at 120 MHz.
 */
const fc_xspi_nfcb_t nand_config = {
    .crcChecksum             = 0x1F3D0B52u,  /* pre-computed CRC in the golden binary */
    .fingerprint             = 0x4E464342u,  /* ascii "BCFN" */
    .version                 = 0x00000001u,
    .DBBTSearchAreaStartPage = 64u,
    .searchStride            = 64u,
    .searchCount             = 1u,
    .firmwareCopies          = 2u,
    .firmwareTable =
        {
            [0] = {.startPage = 128u, .pagesInFirmware = 64u},
            [1] = {.startPage = 192u, .pagesInFirmware = 64u},
        },
    .nand_config =
        {
            .memConfig =
                {
                    .tag = FC_XSPI_CFG_BLK_TAG,
                    /* The golden binary carries V1.0.0 (0x56010000), not the V1.4.0 value of
                     * FC_XSPI_CFG_BLK_VERSION. Keep it identical to the known working image.
                     */
                    .version            = 0x56010000u,
                    .readSampleClkSrc   = kXSPIReadSampleClk_LoopbackFromDqsPad,
                    .csHoldTime         = 3u,
                    .csSetupTime        = 3u,
                    .columnAddressWidth = 11u, /* 2048 byte page + 128 byte spare -> 11 bit column */
                    .configCmdEnable    = 1u,
                    .configCmdSeqs =
                        {
                            /* Unlock all blocks: write protect register 0xA0 = 0x00 */
                            [0] = {.seqNum = 1u, .seqId = 2u},
                            /* Enable buffer read mode + ECC: config register 0xB0 = 0x19 */
                            [1] = {.seqNum = 1u, .seqId = 6u},
                        },
                    .configCmdArgs =
                        {
                            [0] = 0x00000000u,
                            [1] = 0x00000019u,
                        },
                    .deviceType      = 2u, /* 2 - Serial NAND */
                    .sflashPadType   = 4u, /* Quad pads for page data read */
                    .serialClkFreq   = Fc_XspiSerialClk_120MHz,
                    .sflashA1Size    = 0x10000000u, /* 1Gbit = 128 MByte */
                    .commandInterval = 50u,
                    .lookupTable =
                        {
                            /* Seq 0: Read from cache buffer, 0x6B + 16-bit column address
                             * + 8 dummy cycles + quad data read */
                            [5 * 0 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x6B,
                                                          FC_CMD_CADDR_SDR, FC_XSPI_1PAD, 0x10),
                            [5 * 0 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_DUMMY_SDR, FC_XSPI_4PAD, 0x08,
                                                          FC_CMD_READ_SDR, FC_XSPI_4PAD, 0x80),

                            /* Seq 1: Read status register, 0x0F + register address 0xC0 + 1 data byte */
                            [5 * 1 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x0F,
                                                          FC_CMD_SDR, FC_XSPI_1PAD, 0xC0),
                            [5 * 1 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_READ_SDR, FC_XSPI_1PAD, 0x01,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 2: Write protect register, 0x1F + register address 0xA0 + 1 data byte */
                            [5 * 2 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x1F,
                                                          FC_CMD_SDR, FC_XSPI_1PAD, 0xA0),
                            [5 * 2 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_WRITE_SDR, FC_XSPI_1PAD, 0x01,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 3: Write enable, 0x06 */
                            [5 * 3 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x06,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 5: Block erase, 0xD8 + 8 dummy bits + row address */
                            [5 * 5 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0xD8,
                                                          FC_CMD_MODE4_SDR, FC_XSPI_1PAD, 0x00),
                            [5 * 5 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_SDR, FC_XSPI_1PAD, 0x14,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 6: Write configuration register, 0x1F + register address 0xB0 + 1 data byte */
                            [5 * 6 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x1F,
                                                          FC_CMD_SDR, FC_XSPI_1PAD, 0xB0),
                            [5 * 6 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_WRITE_SDR, FC_XSPI_1PAD, 0x01,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 9: Quad load program data, 0x34 + 16-bit column address + quad data write */
                            [5 * 9 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x34,
                                                          FC_CMD_CADDR_SDR, FC_XSPI_1PAD, 0x10),
                            [5 * 9 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_WRITE_SDR, FC_XSPI_4PAD, 0x40,
                                                          FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 11: Page read to cache, 0x13 + 8 dummy bits + row address */
                            [5 * 11 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x13,
                                                           FC_CMD_MODE4_SDR, FC_XSPI_1PAD, 0x00),
                            [5 * 11 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_SDR, FC_XSPI_1PAD, 0x14,
                                                           FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 13: Read status register (copy used for ECC status check) */
                            [5 * 13 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x0F,
                                                           FC_CMD_SDR, FC_XSPI_1PAD, 0xC0),
                            [5 * 13 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_READ_SDR, FC_XSPI_1PAD, 0x01,
                                                           FC_CMD_STOP, FC_XSPI_1PAD, 0x00),

                            /* Seq 14: Program execute, 0x10 + 8 dummy bits + row address */
                            [5 * 14 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD, 0x10,
                                                           FC_CMD_MODE4_SDR, FC_XSPI_1PAD, 0x00),
                            [5 * 14 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_SDR, FC_XSPI_1PAD, 0x14,
                                                           FC_CMD_STOP, FC_XSPI_1PAD, 0x00),
                        },
                },
            .pageDataSize         = 2048u,
            .pageTotalSize        = 4096u,
            .pagesPerBlock        = 64u,
            .eccCheckCustomEnable = 1u,
            .eccStatusMask        = 0x00000030u, /* status register bits [5:4] hold ECC status */
            .eccFailureMask       = 0x00000020u, /* value 0x2 in ECC status field means uncorrectable */
            .blocksPerDevice      = 1024u,
        },
    .xmc_config =
        {
            .xmc_header = {.U = 0x83040449u},
            .xmc_option = {0xA8900108u, 0x88222D22u, 0xB3046015u, 0x2A90018Eu, 0x70800855u},
        },
};

#endif /* BOOT_HEADER_ENABLE */
