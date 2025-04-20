/*
 * ESP8266_HAL.c
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */


#include "ESP8266_HAL.h"
#include "stdio.h"
#include "string.h"
#include "stm32f1xx_hal.h"    // or "stm32xxxx_hal.h" based on your MCU



extern UART_HandleTypeDef huart1;
#define wifi_uart &huart1


char buffer[20];

/*****************************************************************************************************************/
int check_response(const char *expected)
{
	char temp[100] = {0};
	int i = 0;
	uint8_t c;
	uint32_t start = HAL_GetTick();

	while ((HAL_GetTick() - start) < 5000) // 5-second timeout
	{
		// Try to receive 1 byte with 10ms timeout
		if (HAL_UART_Receive(wifi_uart, &c, 1, 10) == HAL_OK)
		{
			if (i < sizeof(temp) - 1)
			{
				temp[i++] = c;
				temp[i] = '\0';

				if (strstr(temp, expected)) return 1;
			}
		}
	}

	return 0;  // Timeout or response not matched
}



/*****************************************************************************************************************************************/
void Uart_flush(UART_HandleTypeDef *uart)
{
    uint8_t dummy;

    // As long as RXNE (Receive Not Empty) flag is set, read and discard
    while (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE))
    {
        HAL_UART_Receive(uart, &dummy, 1, 10);  // Read and discard
    }
}



void ESP_Init (char *SSID, char *PASSWD)
{
	char data[80];
	const char *cmd;

	cmd = "AT+RST\r\n";
	HAL_UART_Transmit(wifi_uart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
	for (int i = 0; i < 5; i++)
	{
		HAL_Delay(1000);
	}

	/********* TEST AT **********/

	Uart_flush(wifi_uart);
	cmd = "AT\r\n";
	HAL_UART_Transmit(wifi_uart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
	while (!check_response("OK\r\n"));

	/********* SET STATION MODE **********/
	Uart_flush(wifi_uart);
	cmd = "AT+CWMODE=1\r\n";
	HAL_UART_Transmit(wifi_uart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
	while (!check_response("OK\r\n"));

	/********* AT+CWJAP="SSID","PASSWD" **********/
	Uart_flush(wifi_uart);
	sprintf(data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PASSWD);
	HAL_UART_Transmit(wifi_uart, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
	while (!check_response("OK\r\n"));
}

int ESP_TCP_Connect(char *ServerIP, int Port)
{
	char data[100];
	/*********Connect to TCP Server **********/
	Uart_flush(wifi_uart);
	sprintf(data, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ServerIP, Port);
	HAL_UART_Transmit(wifi_uart, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
	// 3. Wait for connection response
	if (check_response("CONNECT\r\n"))
		return 1;  // Success

	return 0;  // Failed or timeout
}



int ESP_TCP_Send(char *message)
{
	char cmd[32];
	int len = strlen(message);

	Uart_flush(wifi_uart);
	sprintf(cmd, "AT+CIPSEND=%d\r\n", len);
	HAL_UART_Transmit(wifi_uart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);

	if (!check_response(">")) return 0;
		// Transmit actual message
		HAL_UART_Transmit(wifi_uart, (uint8_t *)message, len, HAL_MAX_DELAY);

	// Wait for confirmation
	if (!check_response("SEND OK\r\n")) return 0;

	return 1;  // Success
}

char ESP_TCP_Receive()
{
    char c = 0;
    char temp[6] = {0};  // To hold incoming stream and match "+IPD,"
    uint8_t ch;
    int match_index = 0;
    uint32_t start = HAL_GetTick();

    // 1. Wait for "+IPD,"
    while ((HAL_GetTick() - start) < 5000)
    {
        if (HAL_UART_Receive(wifi_uart, &ch, 1, 10) == HAL_OK)
        {
            // Shift into temp buffer
            temp[match_index++] = ch;
            if (match_index >= sizeof(temp)) match_index = 0;

            // Check for "+IPD,"
            if (strstr(temp, "+IPD,") != NULL)
                break;
        }
    }

    // 2. Skip bytes until ':' is received
    start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 2000)
    {
        if (HAL_UART_Receive(wifi_uart, &ch, 1, 10) == HAL_OK)
        {
            if (ch == ':') break;
        }
    }

    // 3. Receive and return one character of actual data
    if (HAL_UART_Receive(wifi_uart, (uint8_t *)&c, 1, 100) == HAL_OK)
    {
        return c;
    }

    return 0;  // Return null char if nothing received
}


