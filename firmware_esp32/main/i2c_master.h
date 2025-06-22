/**
 * I2C driver init
 *
 * It's not thread safety
 */
#ifndef MAIN_I2C_MASTER_H_
#define MAIN_I2C_MASTER_H_

#include "driver/i2c.h"
#include "esp_err.h"

#define I2C_ACK		true
#define I2C_NACK	false

extern esp_err_t i2c_master_init(i2c_port_t i2c_master_port, gpio_num_t sda_io_num,
                                    gpio_num_t scl_io_num,  gpio_pullup_t pull_up_en, bool fastMode);


extern esp_err_t i2c_master_close(i2c_port_t i2c_master_port);

#endif /* MAIN_I2C_MASTER_H_ */
