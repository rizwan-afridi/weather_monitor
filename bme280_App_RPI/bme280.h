#ifndef BME280
#define BME280

#include <bcm2835.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define I2C_SENSOR_ADDR     0x76
#define MAX_LEN             32

// REGISTER ADDRESSES
#define ID                  0xD0
#define RESET               0xE0
#define CTRL_HUM            0xF2
#define STATUS              0xF3
#define CTRL_MEAS           0xF4
#define CONFIG              0xF5
#define PRESS_MSB           0xF7
#define PRESS_LSB           0xF8
#define PRESS_XLSB          0xF9
#define TEMP_MSB            0xFA
#define TEMP_LSB            0xFB
#define TEMP_XLSB           0xFC
#define HUM_MSB             0xFD
#define HUM_LSB             0xFE
#define CALIB00             0x88
#define CALIB26             0xE1

typedef struct {
    int32_t temp;
    uint32_t press;
    uint32_t hum;
} Bme280_sensor_t;

// Function prototypes
int initializeBme280(void);
int configureBme280(uint8_t press, uint8_t temp, uint8_t hum, uint8_t mode);
int32_t bme280_compensate_temp(int32_t adc_T);
uint32_t bme280_compensate_press(int32_t adc_P);
uint32_t bme280_compensate_hum(int32_t adc_H);
int bme280_read_sensor(Bme280_sensor_t *sensor_read);

#endif  // BME280
