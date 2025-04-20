/*
 * ESP8266_HAL.h
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */

#ifndef INC_ESP8266_HAL_H_
#define INC_ESP8266_HAL_H_


void ESP_Init (char *SSID, char *PASSWD);

void Server_Start (void);

int tcp_server_connect();

int ESP_TCP_Send(char *message);

char ESP_TCP_Receive();

#endif /* INC_ESP8266_HAL_H_ */
