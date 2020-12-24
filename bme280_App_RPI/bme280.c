#include <bcm2835.h>
#include "bme280.h"

uint8_t rbuf[MAX_LEN] = { 0 };
uint8_t wbuf[MAX_LEN] = { 0 };
uint8_t regAddr = 0;

// Calibration registers
uint8_t  _dig_H1, _dig_H3, _dig_H6;
uint16_t _dig_T1, _dig_P1, _dig_H4, _dig_H5;
int16_t  _dig_T2, _dig_T3, _dig_P2, _dig_P3, _dig_P4, _dig_P5, _dig_P6, _dig_P7, _dig_P8, _dig_P9, _dig_H2;

int32_t  _t_fine;

int initializeBme280(void)
{
    uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_2500;
    uint8_t buf = 0;
    
    if (!bcm2835_init())
    {
        printf("bcm2835_init failed. Are you running as root??\n");
        return -1;
    }

    if (!bcm2835_i2c_begin())
    {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
        return -1;
    }

    bcm2835_i2c_setSlaveAddress(I2C_SENSOR_ADDR);
    bcm2835_i2c_setClockDivider(clk_div);

    // Reset sensor
    wbuf[0] = RESET;
    wbuf[1] = 0xB6;     // Power on reset
    if (bcm2835_i2c_write(wbuf, 2) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C write\n");
        return -2;
    }

    usleep(200000);     // 200ms delay for power-up
    
    // Read back humidity sensor chip ID
    regAddr = ID;
    if (bcm2835_i2c_read_register_rs(&regAddr, &buf, 1) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read\n");
        return -2;
    }
    // Compare chip ID
    if (buf != 0x60)
    {
        printf("Chip ID wrong!\n");
        return -3;
    }
    else
        printf("Success! chip ID read OK\n");

    // Read calibration data in global variables
    uint8_t calib[26] = { 0 };
    regAddr = CALIB00;
    if (bcm2835_i2c_read_register_rs(&regAddr, calib, 26) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read of calibration 00\n");
        return -2;
    }
    _dig_T1 = (uint16_t)(((uint16_t) calib[1] << 8) | calib[0]);
    _dig_T2 = ( int16_t)((( int16_t) calib[3] << 8) | calib[2]);
    _dig_T3 = ( int16_t)((( int16_t) calib[5] << 8) | calib[4]);
    _dig_P1 = (uint16_t)(((uint16_t) calib[7] << 8) | calib[6]);
    _dig_P2 = ( int16_t)((( int16_t) calib[9] << 8) | calib[8]);
    _dig_P3 = ( int16_t)((( int16_t) calib[11] << 8) | calib[10]);
    _dig_P4 = ( int16_t)((( int16_t) calib[13] << 8) | calib[12]);
    _dig_P5 = ( int16_t)((( int16_t) calib[15] << 8) | calib[14]);
    _dig_P6 = ( int16_t)((( int16_t) calib[17] << 8) | calib[16]);
    _dig_P7 = ( int16_t)((( int16_t) calib[19] << 8) | calib[18]);
    _dig_P8 = ( int16_t)((( int16_t) calib[21] << 8) | calib[20]);
    _dig_P9 = ( int16_t)((( int16_t) calib[23] << 8) | calib[22]);
    _dig_H1 = calib[25];

    regAddr = CALIB26;
    if (bcm2835_i2c_read_register_rs(&regAddr, calib, 7) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read of calibration 26\n");
        return -2;
    }
    _dig_H2 = ( int16_t)((( int16_t) calib[1] << 8) | calib[0]);
    _dig_H3 = calib[2];
    _dig_H4 = ( int16_t)(((( int16_t) calib[3] << 8) | (0x0F & calib[4]) << 4) >> 4);
    _dig_H5 = ( int16_t)(((( int16_t) calib[5] << 8) | (0xF0 & calib[4]) ) >> 4 );
    _dig_H6 = calib[6];

    return 0;
}

int configureBme280(uint8_t press, uint8_t temp, uint8_t hum)
{
    
    uint8_t ctrl_meas_byte = 0;
    uint8_t ctrl_hum_byte = 0;

    if (hum)
    {
        ctrl_hum_byte = 0x01;
    }

    wbuf[0] = CTRL_HUM;         // Register address
    wbuf[1] = ctrl_hum_byte;    // Data to be written
    if (bcm2835_i2c_write(wbuf, 2) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C write\n");
        return -2;
    }

    if (press)
    {
        ctrl_meas_byte = (ctrl_meas_byte & 0xE3) | 0x04;
    }

    if (temp)
    {
        ctrl_meas_byte = (ctrl_meas_byte & 0x1F) | 0x20;
    }

    // Operating mode being set to normal (b'11)
    //ctrl_meas_byte = (ctrl_meas_byte & 0xFC) | mode;
    
    wbuf[0] = CTRL_MEAS;
    wbuf[1] = ctrl_meas_byte;
    if (bcm2835_i2c_write(wbuf, 2) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C write\n");
        return -2;
    }

    return 0;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of
// .5123. equals 51.23 DegC.
int32_t bme280_compensate_temp(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)_dig_T1 << 1))) * ((int32_t)_dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)_dig_T1)) * ((adc_T >> 4) - ((int32_t)_dig_T1))) >> 12) * ((int32_t)_dig_T3)) >> 14;
    _t_fine = var1 + var2;
    T = (_t_fine * 5 + 128) >> 8;
    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8
// fractional bits).
// Output value of .24674867. represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t bme280_compensate_press(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)_dig_P6;
    var2 = var2 + ((var1*(int64_t )_dig_P5)<<17);
    var2 = var2 + (((int64_t)_dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)_dig_P3)>>8) + ((var1 * (int64_t)_dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)_dig_P1)>>33;
    if(var1 == 0)
    {
        return 0;
        // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p<<31) - var2)*3125)/var1;
    var1 = (((int64_t)_dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)_dig_P8) * p)>> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)_dig_P7)<<4);
    return (uint32_t)p;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22integer and 10fractional bits).
// Output value of .47445.represents 47445/1024= 46.333%RH
uint32_t bme280_compensate_hum(int32_t adc_H)
{
    int32_t var;

    var = (_t_fine - ((int32_t)76800));
    var = (((((adc_H << 14) - (((int32_t)_dig_H4) << 20) - (((int32_t)_dig_H5) * var)) +
                    ((int32_t)16384)) >> 15) * (((((((var * ((int32_t)_dig_H6)) >> 10) * (((var *
                                            ((int32_t)_dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)_dig_H2) + 8192) >> 14));
    var = (var - (((((var >> 15) * (var >> 15)) >> 7) * ((int32_t)_dig_H1)) >> 4));
    var = (var < 0 ? 0 : var); 
    var = (var > 419430400 ? 419430400 : var);
    return(uint32_t)(var >> 12);
}

int bme280_read_sensor(Bme280_sensor_t *sensor_read)
{
    int32_t press_uncomp = 0;
    int32_t temp_uncomp = 0;
    int32_t hum_uncomp = 0;
    
    regAddr = PRESS_MSB;
    if (bcm2835_i2c_read_register_rs(&regAddr, rbuf, 8) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read\n");
        return -2;
    }

    press_uncomp = (((int32_t) rbuf[0] << 24 | (int32_t) rbuf[1] << 16 | (int32_t) rbuf[2] << 8) >> 12);
    temp_uncomp = (((int32_t) rbuf[3] << 24 | (int32_t) rbuf[4] << 16 | (int32_t) rbuf[5] << 8) >> 12);
    hum_uncomp = ((int32_t)rbuf[6] << 8) | rbuf[7];

    sensor_read->temp = bme280_compensate_temp(temp_uncomp);
    sensor_read->press = bme280_compensate_press(press_uncomp);
    sensor_read->hum = bme280_compensate_hum(hum_uncomp);

    return 0;
}

// 1 -> BUSY, 0 -> READY
int bme280_measurement_status(void)
{
    regAddr = STATUS;
    if (bcm2835_i2c_read_register_rs(&regAddr, rbuf, 1) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read\n");
        return -2;
    }

    return (int)(rbuf[0] & 0x08);
}

// Trigger measurement (foce mode). Assumes that measurements are alraedy enabled
// through configureBme280()
int bme280_force_measurement(void)
{
    regAddr = CTRL_MEAS;
    if (bcm2835_i2c_read_register_rs(&regAddr, rbuf, 1) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C read\n");
        return -2;
    }

    wbuf[0] = CTRL_MEAS;         // Register address
    wbuf[1] = (rbuf[0] & 0xFC) | 0x01;  // Setting mode to force mode
    if (bcm2835_i2c_write(wbuf, 2) != BCM2835_I2C_REASON_OK)
    {
        printf("Some shit went wrong with I2C write\n");
        return -2;
    }

    return 0;
}
