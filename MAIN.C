#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* UART handle (ví dụ UART2) */
extern UART_HandleTypeDef huart2;

/* Access Token của thiết bị ThingsBoard */
const char *TB_ACCESS_TOKEN = "YOUR_ACCESS_TOKEN";

/* Buffer UART RX */
char rxBuffer[256];

/* Hàm hỗ trợ */
bool WaitForOKorError(uint32_t timeout);
bool WaitForChar(char c, uint32_t timeout);
void SendAT(const char *cmd);
void MQTT_Init();
bool MQTT_Connect();
bool MQTT_Publish(const char *topic, const char *payload);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    HAL_Delay(3000);
    printf("Starting SIM A7680C MQTT Demo...\r\n");

    /* Khởi động MQTT */
    MQTT_Init();

    if (MQTT_Connect()) {
        printf("MQTT connected to ThingsBoard!\r\n");

        /* Publish data loop */
        while (1) {
            MQTT_Publish("v1/devices/me/telemetry", "{\"temperature\":25,\"humidity\":70}");
            HAL_Delay(5000);
        }
    } else {
        printf("MQTT connection failed.\r\n");
    }

    while (1);
}
