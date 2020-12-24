#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "bme280.h"

int main()
{
    int err_code;
    Bme280_sensor_t weather = { 0 };
    
    err_code = initializeBme280();
    printf("Initialization code: %d\n", err_code);
    if (err_code)
        return 1;

    err_code = configureBme280(1, 1, 1);
    printf("Config code: %d\n", err_code);
    if (err_code)
        return 1;
    
    // Reading the temperature and humidity values
    while(1)
    {
        if (0 != bme280_force_measurement())
        {
            printf("Error occurred trying to force a measurement\n");
            return 1;
        }

        sleep(1);

        bme280_read_sensor(&weather);
        
        printf("\n\n");
        printf("Pressure: %d\n", weather.press / 256);
        printf("Temperature: %d\n", weather.temp);
        printf("Humidity: %d\n", weather.hum / 1024);
        printf("\n\n");
    }

    return 0;
}
