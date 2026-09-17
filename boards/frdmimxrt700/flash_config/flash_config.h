/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FLASH_CONFIG_H_
#define FLASH_CONFIG_H_
#include <stdint.h>
#include "fsl_common.h"

/*! @name Driver version */
/*@{*/
/*! @brief FLASH_CONFIG driver version 2.0.0. */
#define FSL_FLASH_CONFIG_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*******************************************************************************
 * Definition
 ******************************************************************************/

/* XSPI memory config block related defintions */
#define FC_XSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FC_XSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/* !@brief XSPI clock configuration - When clock source is PLL */
enum
{
    Fc_XspiSerialClk_30MHz  = 1,
    Fc_XspiSerialClk_50MHz  = 2,
    Fc_XspiSerialClk_60MHz  = 3,
    Fc_XspiSerialClk_80MHz  = 4,
    Fc_XspiSerialClk_100MHz = 5,
    Fc_XspiSerialClk_120MHz = 6,
    Fc_XspiSerialClk_133MHz = 7,
    Fc_XspiSerialClk_166MHz = 8,
    Fc_XspiSerialClk_200MHz = 9,
};

/* !@brief LUT instructions supported by XSPI */
#define FC_CMD_STOP 0x00U /*!< Stop execution, deassert CS. */
#define FC_CMD_SDR 0x01U /*!< Transmit Command code to Flash, using SDR mode. */
#define FC_CMD_RADDR_SDR 0x02U /*!< Transmit Row Address to Flash, using SDR mode. */
#define FC_CMD_DUMMY_SDR 0x03U /*!< Leave data lines undriven by xSPI controller, using SDR mode. */
#define FC_CMD_MODE_SDR 0x04U /*!< Transmit 8-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_MODE2_SDR 0x05U /*!< Transmit 2-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_MODE4_SDR 0x06U /*!< Transmit 4-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_READ_SDR 0x07U /*!< Receive Read Data from Flash, using SDR mode. */
#define FC_CMD_WRITE_SDR 0x08U /*!< Transmit Programming Data to Flash, using SDR mode. */
#define FC_CMD_JMP_ON_CS 0x09U /*!< Stop execution, deassert CS and save operand[7:0] as the
                                               instruction start pointer for next sequence*/
#define FC_CMD_RADDR_DDR 0x0AU /*!< Transmit Row Address to Flash, using DDR mode. */
#define FC_CMD_MODE_DDR 0x0BU /*!< Transmit 8-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_MODE2_DDR 0x0CU /*!< Transmit 2-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_MODE4_DDR 0x0DU /*!< Transmit 4-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_READ_DDR 0x0EU /*!< Receive Read Data from Flash, using DDR mode. */
#define FC_CMD_WRITE_DDR 0x0FU /*!< Transmit Programming Data to Flash, using DDR mode. */
#define FC_CMD_LEARN_DDR 0x10U /*!< Receive Read Data or Preamble bit from Flash, DDR mode. */
#define FC_CMD_DDR 0x11U /*!< Transmit Command code to Flash, using DDR mode. */
#define FC_CMD_CADDR_SDR 0x12U /*!< Transmit Column Address to Flash, using SDR mode. */
#define FC_CMD_CADDR_DDR 0x13U /*!< Transmit Column Address to Flash, using DDR mode. */
#define FC_CMD_JUMP_TO_SEQ 0x14U

#define FC_XSPI_1PAD 0
#define FC_XSPI_2PAD 1
#define FC_XSPI_4PAD 2
#define FC_XSPI_8PAD 3

#define FC_XSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                                         \
    (XSPI_LUT_INSTR0(cmd0) | XSPI_LUT_PAD0(pad0) | XSPI_LUT_OPRND0(op0) | XSPI_LUT_INSTR1(cmd1) | \
     XSPI_LUT_PAD1(pad1) | XSPI_LUT_OPRND1(op1))

/* !@brief XSPI Read Sample Clock Source definition */
typedef enum _FlashReadSampleClkSource
{
    kXSPIReadSampleClk_LoopbackInternally      = 0,
    kXSPIReadSampleClk_LoopbackFromDqsPad      = 2,
    kXSPIReadSampleClk_ExternalInputFromDqsPad = 3,
} fc_xspi_read_sample_clk_t;

/* !@brief Misc feature bit definitions */
enum
{
    Fc_XspiMiscOffset_DiffClkEnable            = 0, /* !< Bit for Differential clock enable */
    Fc_XspiMiscOffset_WordAddressableEnable    = 3, /* !< Bit for Word Addressable enable */
    Fc_XspiMiscOffset_SafeConfigFreqEnable     = 4, /* !< Bit for Safe Configuration Frequency enable */
    Fc_XspiMiscOffset_DdrModeEnable            = 6, /* !< Bit for DDR clock confiuration indication. */
};

typedef struct
{
    uint8_t time_100ps;  /* !< Data valid time, in terms of 100ps */
    uint8_t delay_cells; /* !< Data valid time, in terms of delay cells */
} fc_xspi_dll_time_t;

/* !@brief XSPI LUT Sequence structure */
typedef struct _lut_sequence
{
    uint8_t seqNum; /* !< Sequence Number, valid number: 1-16 */
    uint8_t seqId;  /* !< Sequence Index, valid number: 0-15 */
    uint16_t reserved;
} fc_xspi_lut_seq_t;

/* !@brief XSPI Memory Configuration Block */
typedef struct _XSPIConfig
{
    uint32_t tag;       /* !< [0x000-0x003] Tag, fixed value 0x42464346UL */
    uint32_t version;   /* !< [0x004-0x007] Version,[31:24] -'V', [23:16] - Major, [15:8] - Minor, [7:0] - bugfix */
    uint32_t reserved0; /* !< [0x008-0x00b] Reserved for future use */
    uint8_t readSampleClkSrc;    /* !< [0x00c-0x00c] Read Sample Clock Source, valid value:
                                       0: internal sampling
                                       2: DQS pad loopback
                                       3: External DQS signal */
    uint8_t csHoldTime;          /* !< [0x00d-0x00d] CS hold time, default value: 3 */
    uint8_t csSetupTime;         /* !< [0x00e-0x00e] CS setup time, default value: 3 */
    uint8_t columnAddressWidth;  /* !< [0x00f-0x00f] Column Address with, for HyperBus protocol, it is fixed to 3, others to 0. */
    uint8_t deviceModeCfgEnable; /* !< [0x010-0x010] Device Mode Configure enable flag, 1 - Enable, 0 - Disable */
    uint8_t deviceModeType;      /* !< [0x011-0x011] Specify the configuration command type.
                                    0: No mode change
                                    1: Quad enable (switch from SPI to Quad mode)
                                    2: Spi2Xpi (switch from SPI to DPI, QPI, or OPI mode)
                                    3: Xpi2Spi (switch from DPI, QPI, or OPI to SPI mode) */
    uint16_t waitTimeCfgCommands;    /* !< [0x012-0x013] Wait time for Device mode configuration command, unit: 100us */
    fc_xspi_lut_seq_t deviceModeSeq; /* !< [0x014-0x017] Device mode sequence info [ 7:0] - Number of required sequences [15:8] - Sequence index */
    uint32_t deviceModeArg;          /* !< [0x018-0x01b] Argument/Parameter for device configuration */
    uint8_t configCmdEnable;         /* !< [0x01c-0x01c] Configure command Enable Flag, 1 - Enable, 0 - Disable */
    uint8_t configModeType[3];       /* !< [0x01d-0x01f] Configure Mode Type, similar as deviceModeTpe */
    fc_xspi_lut_seq_t configCmdSeqs[3]; /* !< [0x020-0x02b] Sequence info for Device Configuration command, similar as deviceModeSeq */
    uint8_t  ipedMode;                  //!< [0x02c] IPED mode: 0=None,1=CTR,2=GCM,3=XEX
    uint8_t  x16DllaSlvFineOffset;      //!< [0x02d] Fine offset delay elements for incoming DQS1
    uint8_t  maxCsLowInterval;          //!< [0x02e] PSRAM tCSM, unit 0.1us
    uint8_t  ahbAlignment;              //!< [0x02f] AHB alignment: 0=None,1=256B,2=512B,3=1KB
    //uint32_t reserved1;                 /* !< [0x02c-0x02f] Reserved for future use */
    uint32_t configCmdArgs[3];          /* !< [0x030-0x03b] Arguments/Parameters for device Configuration commands */
    uint8_t  ahbSplitEn;                //!< [0x03c] Enable AHB transaction split for PSRAM
    uint8_t reserved2[3];               /* !< [0x03d-0x03f] Reserved for future use */
    //uint32_t reserved2;               /* !< [0x03c-0x03f] Reserved for future use */
    uint32_t controllerMiscOption;      /* !< [0x040-0x043] Controller Misc Options.
					   Bit 0: Differential clock enable: 1 for HyperFlash NOR flash memory 1V8 device and 0 for other devices
                                           Bit 3: WordAddressableEnable: 1 for HyperFlash NOR flash memory and 0 for other devices
                                           Bit 4: SafeConfigFreqEnable: set to 1 if expecting to configure the chip with a safe frequency
                                           Bit 6: DDR mode enable: set to 1 if DDR read is expected
                                           Other bits Reserved; set to 0 */
    uint8_t deviceType;    /* !< [0x044-0x044] Device Type: 1 for Serial NOR flash memory */
    uint8_t sflashPadType; /* !< [0x045-0x045] Serial Flash Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
    uint8_t serialClkFreq; /* !< [0x046-0x046] Serial Flash Frequencey
                                 1: 30 MHz
                                 2: 50 MHz
                                 3: 60 MHz
                                 4: 80 MHz
                                 5: 100 MHz
                                 6: 120 MHz
                                 7: 133 MHz
                                 8: 166 MHz
                                 9: 200 MHz */
    uint8_t lutCustomSeqEnable; /* !< [0x047-0x047] LUT customization Enable, it is required if the program/erase cannot
                                      be done using 1 LUT sequence, currently, only applicable to HyperFLASH */
    uint32_t reserved3[2];      /* !< [0x048-0x04f] Reserved for future use */
    uint32_t sflashA1Size;      /* !< [0x050-0x053] Size of Flash connected to A1 */
    uint32_t sflashA2Size;      /* !< [0x054-0x057] Size of Flash connected to A2 */
    uint32_t sflashB1Size;      /* !< [0x058-0x05b] Size of Flash connected to B1 */
    uint32_t sflashB2Size;      /* !< [0x05c-0x05f] Size of Flash connected to B2 */
    uint32_t csPadSettingOverride;       /* !< [0x060-0x063] CS pad setting override value */
    uint32_t sclkPadSettingOverride;     /* !< [0x064-0x067] SCK pad setting override value */
    uint32_t dataPadSettingOverride;     /* !< [0x068-0x06b] data pad setting override value */
    uint32_t dqsPadSettingOverride;      /* !< [0x06c-0x06f] DQS pad setting override value */
    uint32_t timeoutInMs;                /* !< [0x070-0x073] Timeout threshold for read status command */
    uint32_t commandInterval;            /* !< [0x074-0x077] CS deselect interval between two commands */
    fc_xspi_dll_time_t dataValidTime[2]; /* !< [0x078-0x07b] CLK edge to data valid time for PORT A and PORT B */
    uint16_t busyOffset;                 /* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
    uint16_t busyBitPolarity;            /* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when flash device is busy, 1 -
                                            busy flag is 0 when flash device is busy */
#if defined(FSL_FEATURE_SILICON_VERSION_A)
    uint32_t lookupTable[80];            /* !< [0x080-0x1bf] Lookup table holds Flash command sequences */
    fc_xspi_lut_seq_t lutCustomSeq[12];  /* !< [0x1c0-0x1ef] Customizable LUT Sequences */
    uint32_t dllCraSdrVal;               /* !< [0x1f0-0x1f3] Customizable DLLCRA for SDR setting */
    uint32_t smprSdrVal;                 /* !< [0x1f4-0x1f7] Customizable SMPR SDR setting */
    uint32_t dllCraDdrVal;               /* !< [0x1f8-0x1fb] Customizable DLLCRA for DDR setting */
    uint32_t smprDdrVal;                 /* !< [0x1fc-0x1ff] Customizable SMPR DDR setting */
#else
    uint32_t lookupTable[90];            /* !< [0x080-0x1e7] B0 Lookup table holds Flash command sequences */
    fc_xspi_lut_seq_t lutCustomSeq[12];  /* !< [0x1e8-0x217] Customizable LUT Sequences */
    uint32_t dllCraSdrVal;               /* !< [0x218-0x21b] Customizable DLLCRA for SDR setting */
    uint32_t smprSdrVal;                 /* !< [0x21c-0x21f] Customizable SMPR SDR setting */
    uint32_t dllCraDdrVal;               /* !< [0x220-0x223] Customizable DLLCRA for DDR setting */
    uint32_t smprDdrVal;                 /* !< [0x224-0x227] Customizable SMPR DDR setting */
#endif
} fc_xspi_mem_config_t;
/*
 *  Serial NOR configuration block
 */
typedef struct _fc_xspi_nor_config
{
    fc_xspi_mem_config_t memConfig; /* !< Common memory configuration info via XSPI */
    uint32_t pageSize;              /* !< Page size of Serial NOR */
    uint32_t sectorSize;            /* !< Sector size of Serial NOR */
    uint8_t ipcmdSerialClkFreq;     /* !< Clock frequency for IP command */
    uint8_t isUniformBlockSize;     /* !< Sector/Block size is the same */
    uint8_t isDataOrderSwapped;     /* !< Data order (D0, D1, D2, D3) is swapped (D1,D0, D3, D2) */
    uint8_t reserved0[1];           /* !< Reserved for future use */
    uint8_t serialNorType;          /* !< Serial NOR Flash type: 0/1/2/3 */
    uint8_t needExitNoCmdMode;      /* !< Need to exit NoCmd mode before other IP command */
    uint8_t halfClkForNonReadCmd;   /* !< Half the Serial Clock for non-read command: true/false */
    uint8_t needRestoreNoCmdMode;   /* !< Need to Restore NoCmd mode after IP commmand execution */
    uint32_t blockSize;             /* !< Block size */
    uint32_t flashStateCtx;         /* !< Flash State Context */
#if defined(FSL_FEATURE_SILICON_VERSION_A)
    uint32_t reserved2[58];          /* !< Reserved for future use */
#else
    uint32_t reserved2[48];          /* !< Reserved for future use */
#endif
} fc_xspi_nor_config_t;

/*
 *  Serial NAND configuration block
 */
typedef struct _xspi_nand_config
{
    fc_xspi_mem_config_t memConfig; //!< [0x000-0x227] Generic Memory configuration block over FlexSPI
    uint32_t pageDataSize;       //!< [0x228-0x22b] Data Size in one page, usually it is 2048 or 4096
    uint32_t pageTotalSize;   //!< [0x22c-0x22f] Total size in one page, usually, it equals 2 ^ width of column address
    uint32_t pagesPerBlock;   //!< [0x230-0x233] Pages per block
    uint8_t bypassReadStatus; //!< [0x234-0x234] Bypass Read Status
    uint8_t bypassEccRead;    //!< [0x235-0x235] Bypass ECC check
    uint8_t hasMultiPlanes;   //!< [0x236-0x236] Has multiple planes
    uint8_t skipOddBlocks;    //!< [0x237-0x237] Skip odd blocks
    uint8_t eccCheckCustomEnable; //!< [0x238-0x238] Determine if use the customized ECC check related masks
    uint8_t ipCmdSerialClkFreq;   //!< [0x239-0x239] IP Command Serial Clock Frequency, keep 0
    uint16_t readPageTimeUs;  //!< [0x23a-0x23b] Time needed for read page to cache, effective when bypassReadStatus=1
    uint32_t eccStatusMask;   //!< [0x23c-0x23f] ECC status mask
    uint32_t eccFailureMask;  //!< [0x240-0x243] ECC failure mask
    uint32_t blocksPerDevice; //!< [0x244-0x247] Blocks per device, not used in ROM
    uint32_t reserved1[8];    //!< [0x248-0x267] Reserved for future use.
} fc_xspi_nand_config_t;

/*
 *  Serial PSRAM configuration block
 */
typedef struct _fc_xspi_psram_config
{
    uint32_t xmcdHeader;            /* !< XMCD header */
    uint32_t xmcdOpt0;              /* !< Simplified XSPI RAM Configuration Option 0 */
    uint32_t xmcdOpt1;              /* !< Simplified XSPI RAM Configuration Option 1 */
    uint32_t reserved2[189];        /* !< Reserved for future use */
} fc_xspi_psram_config_t;

typedef struct {
    fc_xspi_nor_config_t xspi_fcb_block; /* !< Configure structure for boot device connected to XSPI0/XSPI1 interface. */
    fc_xspi_psram_config_t psram_config_block; /* !< Configure structure for PSRAM device connected to XSPI0/XSPI1 interface. */
    uint8_t xspi2_fcb_block[768];        /* !< Configure structure for PSRAM device connected to XSPI2 interface. Only for
                                               users' usage, Boot ROM doesn't use this part */
    uint8_t reserved[1792];              /* !< Reserved for future usage */
} fc_static_platform_config_t;


typedef struct _firmware_info
{
    uint32_t startPage;             //!< [+0x0] Firmware start page index
    uint32_t pagesInFirmware;       //!< [+0x4] Number of pages in this firmware copy
} firmware_info_t;

typedef struct _fc_xmcd_info
{
    union
    {
        struct
        {
            uint32_t size            : 12; //!< Size in bytes
            uint32_t configBlockType :  4; //!< Config block type
            uint32_t instance        :  4; //!< XSPI instance
            uint32_t memoryInterface :  4; //!< Memory interface type
            uint32_t version         :  4; //!< Version
            uint32_t tag             :  4; //!< Tag
        } B;
        uint32_t U;
    } xmc_header;
    uint32_t xmc_option[5];
} fc_xmcd_info_t;

typedef struct _xSpiFCBStruct_t
{
    uint32_t crcChecksum;                            //!< [0x000-0x003]
    uint32_t fingerprint;                            //!< [0x004-0x007]
    uint32_t version;                                //!< [0x008-0x00b]
    uint32_t DBBTSearchAreaStartPage;                //!< [0x00c-0x00f]
    uint16_t searchStride;                           //!< [0x010-0x011] Not used by ROM
    uint16_t searchCount;                            //!< [0x012-0x013] Not used by ROM
    uint32_t firmwareCopies;                         //!< [0x014-0x017]
    uint32_t reserved0[10];                          //!< [0x018-0x03f]
    firmware_info_t firmwareTable[8];                //!< [0x040-0x07f]
    uint32_t reserved1[32];                          //!< [0x080-0x0ff];
    fc_xspi_nand_config_t nand_config;               //!< [0x100-0x367];
    fc_xmcd_info_t xmc_config;                     //!< [0x368-0x37f];
    uint32_t reserved2[32];                          //!< [0x380-0x3ff];
} fc_xspi_nfcb_t;
 
#endif
