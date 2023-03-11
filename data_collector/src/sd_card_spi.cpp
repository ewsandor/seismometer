#include <cstdio>

#include <hardware/gpio.h>
#include <pico/binary_info.h>

#include <ff.h>
#include <f_util.h>
#include <hw_config.h>
#include <diskio.h> 
#include <zlib.h>


#include "sd_card_spi.hpp"
#include "seismometer_debug.hpp"
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
    .baud_rate                = (5000*1000),
    //.baud_rate                = (12500*1000),
    //.baud_rate                = (25*1000*1000),
    .set_drive_strength       = true,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .sck_gpio_drive_strength  = GPIO_DRIVE_STRENGTH_12MA,
  }
};

#define SD_CARD_0_CD_PIN 15
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
    .set_drive_strength     =  true,
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
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Initializing SD Card.\n");

  bi_decl(bi_3pins_with_func(SPI_0_MOSI_PIN, SPI_0_MISO_PIN, SPI_0_SCK_PIN, GPIO_FUNC_SPI));
  bi_decl(bi_1pin_with_name(SD_CARD_0_CS_PIN,  "SD Card SPI Chip Select Output"));
  bi_decl(bi_1pin_with_name(SD_CARD_0_CD_PIN,  "SD Card Chip Detect Input"));
}
const char *sd_card_spi_mount (const unsigned int sd_index)
{
  const char * ret_val = nullptr;
  assert(sd_index < sd_get_num());
  sd_card_t *pSD = &sd_card[sd_index];

  error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, true);

  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Mounting SD SPI %u.\n", sd_index);
  FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
  if(FR_OK == fr)
  {
    ret_val = pSD->pcName;
    error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, false);
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "SD SPI error (%u) mounting card %u - %s.\n", fr, sd_index, FRESULT_str(fr));
  }

  return ret_val;
}
bool sd_card_spi_unmount(const unsigned int sd_index)
{
  bool ret_val = false;

  assert(sd_index < sd_get_num());
  sd_card_t *pSD = &sd_card[sd_index];

   error_state_update(ERROR_STATE_SD_SPI_0_NOT_MOUNTED, true);

  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Unmounting SD SPI %u.\n", sd_index);
  FRESULT fr = f_unmount(pSD->pcName);
  if(FR_OK == fr)
  {
    ret_val = true;
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "SD SPI error (%u) unmounting card %u - %s.\n", fr, sd_index, FRESULT_str(fr));
  }
  pSD->m_Status |= STA_NOINIT;

  return ret_val;
}

#define SEISMOMETER_ZLIB_CHUNK_SIZE        16384
#define SEISMOMETER_ZLIB_COMPRESSION_LEVEL 2
/* deflate memory usage (bytes) = (1 << (windowBits+2)) + (1 << (memLevel+9)) + 6 KB */
/* From zlib manual:
    The windowBits parameter is the base two logarithm of the window size (the size of the history buffer). It should be in the range 8..15 for this version of the library. Larger values of this parameter result in better compression at the expense of memory usage. The default value is 15 if deflateInit is used instead.
    For the current implementation of deflate(), a windowBits value of 8 (a window size of 256 bytes) is not supported. As a result, a request for 8 will result in 9 (a 512-byte window). In that case, providing 8 to inflateInit2() will result in an error when the zlib header with 9 is checked against the initialization of inflate(). The remedy is to not use 8 with deflateInit2() with this initialization, or at least in that case use 9 with inflateInit2(). */
#define SEISMOMETER_ZLIB_WINDOW_BITS       13
#define SEISMOMETER_ZLIB_MEM_LEVEL         (SEISMOMETER_ZLIB_WINDOW_BITS-7)
bool compress_file_zlib(FIL *source_file, FIL *compressed_file)
{
  bool ret_val = true;

  assert(nullptr != source_file);
  assert(nullptr != compressed_file);

  /* allocate deflate state */
  Bytef in[SEISMOMETER_ZLIB_CHUNK_SIZE];
  Bytef out[SEISMOMETER_ZLIB_CHUNK_SIZE];
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  int ret = deflateInit2(&strm, SEISMOMETER_ZLIB_COMPRESSION_LEVEL, Z_DEFLATED, 9, 2, Z_DEFAULT_STRATEGY);
  if(Z_OK != ret)
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%d) initializing deflate stream.\n", ret);
    assert(0);
  }

  unsigned int input_bytes = 0, output_bytes = 0;
  do
  {
    FRESULT fr = f_read(source_file, in, SEISMOMETER_ZLIB_CHUNK_SIZE, &strm.avail_in);
    input_bytes += strm.avail_in;
    if(FR_OK != fr)
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) reading from source file - %s.\n", fr, FRESULT_str(fr));
      ret_val = false;
      break;
    }
    else
    {
      strm.next_in = in;
      int flush = f_eof(source_file) ? Z_FINISH : Z_NO_FLUSH;
        
      do
      {
        strm.avail_out = SEISMOMETER_ZLIB_CHUNK_SIZE;
        strm.next_out  = out;

        ret = deflate(&strm, flush);    /* no bad return value */
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

        unsigned have = SEISMOMETER_ZLIB_CHUNK_SIZE - strm.avail_out;
        output_bytes += have;
        unsigned br;
        fr = f_write(compressed_file, out, have, &br);

        if((FR_OK != fr) || (have != br))
        {
          SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) writing to compressed file (%u/%u bytes written) - %s.\n", fr,  br, have, FRESULT_str(fr));
          ret_val = false;
          break;
        }
        SEISMOMETER_PRINTF(SEISMOMETER_LOG_DEBUG, ".");

      } while(strm.avail_out == 0);
      if(true == ret_val)
      {
        assert(strm.avail_in == 0);
      }
    }

  } while(ret_val && (0 == f_eof(source_file)));
  if(true == ret_val)
  {
    assert(ret == Z_STREAM_END);
  }

  deflateEnd(&strm);
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_DEBUG, "\n");
  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Done compressing (%u bytes in, %u bytes out).\n", input_bytes, output_bytes);

  return ret_val;
}

#define COMPRESSED_FILE_FILENAME_MAX_LENGTH       256
#define COMPRESSED_FILE_FILENAME_EXTENSION_LENGTH 3 /* includes '.' seperator */
#define COMPRESSED_FILE_FILENAME_FORMAT           "%s.gz"
bool sd_card_spi_compress_file(const char* source_filename)
{
  bool ret_val = true;
  assert(source_filename != nullptr);

  char compressed_file_filename[COMPRESSED_FILE_FILENAME_MAX_LENGTH] = {'\0'};
  assert(snprintf(compressed_file_filename, COMPRESSED_FILE_FILENAME_MAX_LENGTH, COMPRESSED_FILE_FILENAME_FORMAT, source_filename) > COMPRESSED_FILE_FILENAME_EXTENSION_LENGTH);

  SEISMOMETER_PRINTF(SEISMOMETER_LOG_INFO, "Compressing file '%s' to '%s'.\n", source_filename, compressed_file_filename);

  FRESULT fr = FR_INVALID_PARAMETER;
  FIL source_file;
  fr = f_open(&source_file, source_filename, FA_READ);
  if(FR_OK == fr)
  {
    FIL compressed_file;
    fr = f_open(&compressed_file, compressed_file_filename, (FA_CREATE_ALWAYS | FA_WRITE));

    if(FR_OK == fr)
    {
      assert(true == ret_val);
      ret_val = compress_file_zlib(&source_file, &compressed_file);

      if(FR_OK != f_close(&compressed_file))
      {
        SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) closing compressed file '%s' - %s.\n", fr, compressed_file_filename, FRESULT_str(fr));
        ret_val = false;
      }
    }
    else
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) opening compressed file '%s' - %s.\n", fr, compressed_file_filename, FRESULT_str(fr));
      ret_val = false;
    }

    if(FR_OK != f_close(&source_file))
    {
      SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) closing source file '%s' - %s.\n", fr, source_filename, FRESULT_str(fr));
      ret_val = false;
    }
  }
  else
  {
    SEISMOMETER_PRINTF(SEISMOMETER_LOG_ERROR, "Error (%u) opening source file '%s' - %s.\n", fr, source_filename, FRESULT_str(fr));
    ret_val = false;
  }

  return ret_val;
}