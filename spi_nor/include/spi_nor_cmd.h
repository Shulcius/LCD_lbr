/**
 * @file spi_nor_cmd.h
 * @brief Опкоды SPI NOR (JEDEC / совместимые с LE25 / W25Q / GD25).
 */
#ifndef SPI_NOR_CMD_H
#define SPI_NOR_CMD_H

#define SPI_NOR_CMD_WRITE_ENABLE      0x06u
#define SPI_NOR_CMD_WRITE_DISABLE     0x04u
#define SPI_NOR_CMD_READ_STATUS       0x05u
#define SPI_NOR_CMD_WRITE_STATUS      0x01u
#define SPI_NOR_CMD_READ              0x03u
#define SPI_NOR_CMD_FAST_READ         0x0Bu
#define SPI_NOR_CMD_PAGE_PROGRAM      0x02u
#define SPI_NOR_CMD_SECTOR_ERASE_4K   0x20u
#define SPI_NOR_CMD_SECTOR_ERASE_4K_ALT 0xD7u /* LE25 small-sector alias */
#define SPI_NOR_CMD_BLOCK_ERASE_64K   0xD8u
#define SPI_NOR_CMD_CHIP_ERASE        0xC7u
#define SPI_NOR_CMD_CHIP_ERASE_ALT    0x60u  /* LE25 / некоторые SST */
#define SPI_NOR_CMD_JEDEC_ID          0x9Fu
#define SPI_NOR_CMD_READ_ID           0xABu  /* release PD / device ID */
#define SPI_NOR_CMD_POWER_DOWN        0xB9u
#define SPI_NOR_CMD_READ_SFDP         0x5Au

/* Status Register bit 0 */
#define SPI_NOR_SR_WIP                0x01u
#define SPI_NOR_SR_WEL                0x02u

#endif /* SPI_NOR_CMD_H */
