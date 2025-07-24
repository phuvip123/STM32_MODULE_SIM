#include "main.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/* Extern UART */
extern UART_HandleTypeDef huart2;
extern char rxBuffer[256];
extern const char *TB_ACCESS_TOKEN;

/* Gửi AT command */
void SendAT(const char *cmd)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\r\n", cmd);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
}

/* Chờ OK hoặc ERROR */
bool WaitForOKorError(uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    uint16_t idx = 0;
    memset(rxBuffer, 0, sizeof(rxBuffer));

    while ((HAL_GetTick() - start) < timeout && idx < sizeof(rxBuffer) - 1) {
        if (HAL_UART_Receive(&huart2, (uint8_t *)&rxBuffer[idx], 1, 100) == HAL_OK) {
            if (strstr(rxBuffer, "OK")) return true;
            if (strstr(rxBuffer, "ERROR")) return false;
            idx++;
        }
    }
    return false;
}

/* Chờ ký tự đặc biệt '>' */
bool WaitForChar(char c, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    char ch;
    while ((HAL_GetTick() - start) < timeout) {
        if (HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, 100) == HAL_OK) {
            if (ch == c) return true;
        }
    }
    return false;
}

/* Khởi tạo GPRS và MQTT stack */
void MQTT_Init()
{
    SendAT("AT");
    WaitForOKorError(2000);

    SendAT("AT+CGATT=1");      // GPRS attach
    WaitForOKorError(5000);

    SendAT("AT+CSTT=\"internet\""); // APN (sửa theo nhà mạng)
    WaitForOKorError(5000);

    SendAT("AT+CIICR");        // Bring up wireless connection
    WaitForOKorError(5000);

    SendAT("AT+CIFSR");        // Get IP address
    WaitForOKorError(5000);

    SendAT("AT+CMQTTSTART");   // Start MQTT service
    WaitForOKorError(5000);

    SendAT("AT+CMQTTACCQ=0,\"client1\""); // Create MQTT client
    WaitForOKorError(3000);
}

/* Kết nối MQTT broker */
bool MQTT_Connect()
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "AT+CMQTTCONNECT=0,\"tcp://demo.thingsboard.io:1883\",60,1,\"%s\",\"\"",
             TB_ACCESS_TOKEN);
    SendAT(cmd);
    return WaitForOKorError(8000);
}

/* Publish dữ liệu JSON */
bool MQTT_Publish(const char *topic, const char *payload)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,%d", (int)strlen(topic));
    SendAT(cmd);
    if (!WaitForChar('>', 3000)) return false;
    HAL_UART_Transmit(&huart2, (uint8_t *)topic, strlen(topic), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);
    if (!WaitForOKorError(3000)) return false;

    snprintf(cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%d", (int)strlen(payload));
    SendAT(cmd);
    if (!WaitForChar('>', 3000)) return false;
    HAL_UART_Transmit(&huart2, (uint8_t *)payload, strlen(payload), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);
    if (!WaitForOKorError(3000)) return false;

    SendAT("AT+CMQTTPUB=0,1,60"); // QoS=1, Retain=0
    return WaitForOKorError(5000);
}
