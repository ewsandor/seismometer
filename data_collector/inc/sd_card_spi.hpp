#ifndef __SD_CARD_SPI_HPP__
#define __SD_CARD_SPI_HPP__

void        sd_card_spi_init();
/* Mount SD card with given index, returns the mount path or NULL if error */
const char *sd_card_spi_mount (const unsigned int sd_index);
/* Unmount SD card, returns true if successful */
bool        sd_card_spi_unmount(const unsigned int sd_index);

#endif /* __SD_CARD_SPI_HPP__ */