#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "bme280.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ARRAY_SIZE(arr)       ( sizeof(arr) / sizeof(arr[0]) )

// Global variables for socket management
int server_fd, client_fd;
struct sockaddr_in server_addr;
char sMsg[256] = { 0 };
char cMsg[256] = { 0 };

// Command strings
char *cmdStrings[] = {
    "Fetch temperature",
    "Fetch humidity",
    "Fetch pressure",
    "Disconnect"};

int client_handshake(void)
{
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12000);
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.193");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        printf("Error in server socket creation\n");
        return 1;
    }

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
        printf("Bind successful\n");
    else
    {
        printf("Bind failed\n");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == 0)
        printf("Listening...\n");
    else
    {
        printf("unable to listen\n");
        close(server_fd);
        return 1;
    }

    client_fd = accept(server_fd, NULL, NULL);

    // Waiting for start signal from client
    memset(cMsg, 0, sizeof(cMsg));
    recv(client_fd, cMsg, sizeof(cMsg), 0);
    if (strcmp(cMsg, "Start measurement") != 0)
    {
        printf("Invalid message received from client\n");
        printf("cMsg: %s\n", cMsg);
        close(server_fd);
        return 1;
    }
    else
        printf("Message received from client, starting measurement...\n");

    memset(sMsg, 0, sizeof(cMsg));
    strcpy(sMsg, "Starting measurement on RPi now");
    send(client_fd, sMsg, strlen(sMsg), 0);
    
    return 0;
}

int is_command_valid(char *cmd)
{
    for(int i=0; i<ARRAY_SIZE(cmdStrings); i++)
    {
        if (0 == strcmp(cmd, cmdStrings[i]))
            return i;
    }

    return -1;
}

int main()
{
    int errCode, cmdCode;
    ssize_t rbytes = 0;
    ssize_t sbytes = 0;

    if (client_handshake() != 0)
    {
        printf("Client handshaking failed\n");
        return 1;
    }

    Bme280_sensor_t weather = { 0 };

    errCode = initializeBme280();
    printf("Initialization code: %d\n", errCode);
    if (errCode)
        return 1;

    errCode = configureBme280(1, 1, 1);
    printf("Config code: %d\n", errCode);
    if (errCode)
        return 1;
    
    // Reading the temperature and humidity values
    while(1)
    {
        memset(cMsg, 0, sizeof(cMsg));
        rbytes = recv(client_fd, cMsg, sizeof(cMsg), 0);
        if (0 == rbytes)
        {
            printf("Client socket dropped possibly\n");
            close(server_fd);
            return 1;
        }

        cmdCode = is_command_valid(cMsg);
        if (cmdCode >= 0)
        {
            if (0 == strcmp(cmdStrings[cmdCode], "Disconnect"))
            {
                printf("Closing socket\n");
                close(server_fd);
                return 1;
            }

            if (0 != bme280_force_measurement())
            {
                printf("Error occurred trying to force a measurement\n");
                return 1;
            }

            usleep(200000);
            bme280_read_sensor(&weather);

            memset(sMsg, 0, sizeof(cMsg));
            if (0 == strcmp(cmdStrings[cmdCode], "Fetch temperature"))
            {
                sprintf(sMsg, "Temperature: %d", weather.temp);
            }
            else if (0 == strcmp(cmdStrings[cmdCode], "Fetch humidity"))
            {
                sprintf(sMsg, "Humidity: %d", weather.hum / 1024);
            }
            else if (0 == strcmp(cmdStrings[cmdCode], "Fetch pressure"))
            {
                sprintf(sMsg, "Pressure: %d", weather.press / 256);
            }

            sbytes = send(client_fd, sMsg, strlen(sMsg), 0);
/*
            printf("\n\n");
            printf("Pressure: %d\n", weather.press / 256);
            printf("Temperature: %d\n", weather.temp);
            printf("Humidity: %d\n", weather.hum / 1024);
            printf("\n\n");
*/            
        }

    }

    return 0;
}
