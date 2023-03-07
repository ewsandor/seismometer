#include <cstdio>

#include <hardware/gpio.h>
#include <pico/binary_info.h>

#include <ff.h>
#include <f_util.h>
#include <hw_config.h>

#include "sd_card_spi.hpp"
#include "seismometer_utils.hpp"

#define SPI_0_SCK_PIN  PICO_DEFAULT_SPI_SCK_PIN
#define SPI_0_MOSI_PIN PICO_DEFAULT_SPI_TX_PIN
#define SPI_0_MISO_PIN PICO_DEFAULT_SPI_RX_PIN

static spi_t spi[] =
{
  {
    .hw_inst                  = spi0,
    .miso_gpio                = SPI_0_MISO_PIN,
    .mosi_gpio                = SPI_0_MOSI_PIN,
    .sck_gpio                 = SPI_0_SCK_PIN,
    //.baud_rate                = (12500*1000),
    .baud_rate                = (25*1000*1000),
    .set_drive_strength       = false,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .sck_gpio_drive_strength  = GPIO_DRIVE_STRENGTH_12MA,
  }
};

#define SD_CARD_0_CS_PIN 20

static sd_card_t sd_card[] = 
{
  {
    .pcName                 =  "0:",
    .spi                    = &spi[0],
    .ss_gpio                =  SD_CARD_0_CS_PIN,
    .use_card_detect        =  true,
    .card_detect_gpio       =  SD_CARD_0_CD_PIN,
    .card_detected_true     =  1,
    .set_drive_strength     =  false,
    .ss_gpio_drive_strength =  GPIO_DRIVE_STRENGTH_12MA,
  },
};

size_t sd_get_num() { return count_of(sd_card); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_card[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spi); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &spi[num];
    } else {
        return NULL;
    }
}
void sd_card_spi_init()
{
  printf("Initializing SD Card.\n");

  bi_decl(bi_3pins_with_func(SPI_0_MOSI_PIN, SPI_0_MISO_PIN, SPI_0_SCK_PIN, GPIO_FUNC_SPI));
  bi_decl(bi_1pin_with_name(SD_CARD_0_CS_PIN,  "SD Card SPI Chip Select Output"));
  bi_decl(bi_1pin_with_name(SD_CARD_0_CD_PIN,  "SD Card Chip Detect Input"));

/*
  sd_card_t *pSD = &sd_card[0];
  FIL fil;
  const char* const filename = "filename.txt";
  fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
  if (FR_OK != fr && FR_EXIST != fr)
      panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
  if (f_printf(&fil, "Hello, world!\n") < 0) {
      printf("f_printf failed\n");
  }
  fr = f_close(&fil);
  if (FR_OK != fr) {
      printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
  }
  f_unmount(pSD->pcName);
  */
}
const char *sd_card_spi_mount (const unsigned int sd_index)
{
  const char * ret_val = nullptr;
  assert(sd_index < sd_get_num());
  sd_card_t *pSD = &sd_card[sd_index];

  error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, true);

  printf("Mounting SD SPI %u.\n", sd_index);
  FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
  if(FR_OK == fr)
  {
    ret_val = pSD->pcName;
    error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, false);
  }
  else
  {
    printf("SD SPI error (%u) mounting card %u - %s.\n", fr, sd_index, FRESULT_str(fr));
  }

  return ret_val;
}
bool sd_card_spi_unmount(const unsigned int sd_index)
{
  bool ret_val = false;

  assert(sd_index < sd_get_num());
  sd_card_t *pSD = &sd_card[sd_index];

  error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, true);

  printf("Unmounting SD SPI %u.\n", sd_index);
  FRESULT fr = f_unmount(pSD->pcName);
  if(FR_OK == fr)
  {
    ret_val = true;
  }
  else
  {
    printf("SD SPI error (%u) unmounting card %u - %s.\n", fr, sd_index, FRESULT_str(fr));
  }

  return ret_val;
}