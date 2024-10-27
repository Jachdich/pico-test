#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "sgtl5000.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define I2C_ADDR 0x0A

uint32_t sgtl5000_read(uint16_t reg) {
  uint8_t packets[] = {reg >> 8, reg & 0xFF};
  uint8_t bytes_written = i2c_write_blocking(i2c0, I2C_ADDR, packets, 2, true);
  if (bytes_written != 2) {
    return 0;
  }

  uint8_t buf[2];
  uint32_t bytes_read = i2c_read_blocking(i2c0, I2C_ADDR, buf, 2, false) << 8;
  if (bytes_read != 2) {
    return 0;
  }
  return buf[0] << 8 | buf[1];
}

size_t sgtl5000_write(uint16_t reg, uint16_t val) {
  uint8_t packets[] = {reg >> 8, reg & 0xFF, val >> 8, val & 0xFF};
  uint8_t bytes = i2c_write_blocking(i2c0, I2C_ADDR, packets, 4, false);
  if ((int8_t)bytes == PICO_ERROR_GENERIC) {
    return 0;
  }
  return bytes;
}

bool sgtl5000_setup(void) {
  i2c_init(i2c0, 1000); // TODO speed?
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  printf("i2c init\n");
  sleep_ms(5);
  size_t r = sgtl5000_write(CHIP_ANA_POWER, ANA_POWER_REFTOP_POWERUP |
                                              ANA_POWER_ADC_MONO |
                                              ANA_POWER_DAC_MONO);
  printf("wrote first value, ret: %d\n", r);
  if (!r)
    return false;
  sgtl5000_write(CHIP_LINREG_CTRL, LINREG_VDDC_VOLTAGE(0x0C /*1.0V*/) |
                                       LINREG_VDDC_FROM_VDDIO |
                                       LINREG_VDDC_SOURCE_OVERRIDE);
  printf("write linreg control\n");
  sgtl5000_write(CHIP_REF_CTRL, REF_VAG_VOLTAGE(0x1F /*1.575V*/) |
                                    REF_BIAS_CTRL(0x1 /*+12.5%*/));
  sgtl5000_write(CHIP_LINE_OUT_CTRL,
                 LINE_OUT_CTRL_VAG_VOLTAGE(0x22 /*1.65V*/) |
                     LINE_OUT_CTRL_OUT_CURRENT(0x3C /*0.54mA*/));
  sgtl5000_write(CHIP_SHORT_CTRL, 0x4446); // fuck it, I cba to work this out
  sgtl5000_write(CHIP_ANA_CTRL, 0x0137);   // enable zero cross detectors

  // TODO: Handle case if SGTL5000 is the I2S master. For now, assume slave
  sgtl5000_write(CHIP_ANA_POWER,
                 ANA_POWER_CAPLESS_HEADPHONE_POWERUP | ANA_POWER_DAC_POWERUP |
                     ANA_POWER_HEADPHONE_POWERUP | ANA_POWER_DAC_MONO |
                     ANA_POWER_VAG_POWERUP | ANA_POWER_REFTOP_POWERUP);
  sgtl5000_write(CHIP_DIG_POWER, DIG_POWER_DAC | DIG_POWER_I2S_IN);
  sleep_ms(400);
  sgtl5000_write(CHIP_CLK_CTRL, 0x0004); // 44.1 kHz, 256*Fs
  sgtl5000_write(CHIP_I2S_CTRL, 0x0030); // SCLK=64*Fs, 16bit, I2S format
  // default signal routing is ok?
  sgtl5000_write(CHIP_SSS_CTRL, 0x0010);    // ADC->I2S, I2S->DAC
  sgtl5000_write(CHIP_ADCDAC_CTRL, 0x0000); // disable dac mute
  sgtl5000_write(CHIP_DAC_VOL, 0x3C3C);     // digital gain, 0dB
  sgtl5000_write(CHIP_ANA_HP_CTRL, 0x7F7F); // set volume (lowest level)
  sgtl5000_write(CHIP_ANA_CTRL, 0x0036);    // enable zero cross detectors
  return true;
}

const static int AUDIO_SAMPLE_RATE_EXACT = 44100.0f;

void configure_i2s(void) {

}

void audio_test(void) {
  while (1) {
  bool success = sgtl5000_setup();
  printf("Powered up success? %d\n", success);
  sleep_ms(500);
  }
}
