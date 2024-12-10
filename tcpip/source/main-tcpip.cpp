// **************************************************************************
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 12/2023
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         FreeRTOS TCP/IP Demo with LED Control
//
// **************************************************************************
//
// All mapped LEDs and switches and their PINs and GPIOs:
// See schema in APPS syllabus.
//
// Switches:
// Name        PIN             GPIO
// PTC9        SW_PTC9_PIN     SW_PTC9_GPIO
// PTC10       SW_PTC10_PIN    SW_PTC10_GPIO
// PTC11       SW_PTC11_PIN    SW_PTC11_GPIO
// PTC12       SW_PTC12_PIN    SW_PTC12_GPIO
//
// LEDs:
// Name        PIN             GPIO
// PTA1        LED_PTA1_PIN    LED_PTA1_GPIO
// PTA2        LED_PTA2_PIN    LED_PTA2_GPIO
//
// PTC0        LED_PTC0_PIN    LED_PTC0_GPIO
// PTC1        LED_PTC1_PIN    LED_PTC1_GPIO
// PTC2        LED_PTC2_PIN    LED_PTC2_GPIO
// PTC3        LED_PTC3_PIN    LED_PTC3_GPIO
// PTC4        LED_PTC4_PIN    LED_PTC4_GPIO
// PTC5        LED_PTC5_PIN    LED_PTC5_GPIO
// PTC7        LED_PTC7_PIN    LED_PTC7_GPIO
// PTC8        LED_PTC8_PIN    LED_PTC8_GPIO
//
// PTB2        LED_PTB2_PIN    LED_PTB2_GPIO
// PTB3        LED_PTB3_PIN    LED_PTB3_GPIO
// PTB9        LED_PTB9_PIN    LED_PTB9_GPIO
// PTB10       LED_PTB10_PIN   LED_PTB10_GPIO
// PTB11       LED_PTB11_PIN   LED_PTB11_GPIO
// PTB18       LED_PTB18_PIN   LED_PTB18_GPIO
// PTB19       LED_PTB19_PIN   LED_PTB19_GPIO
// PTB20       LED_PTB20_PIN   LED_PTB20_GPIO
// PTB23       LED_PTB23_PIN   LED_PTB23_GPIO

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_sysmpu.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// Standard C libraries
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Task priorities.
#define LOW_TASK_PRIORITY        (configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY     (configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY       (configMAX_PRIORITIES)

// Task names
#define TASK_NAME_LED_PTA        "led_pta"
#define TASK_NAME_LED_CONTROL    "led_control"
#define TASK_NAME_SOCKET_SRV     "socket_srv"
#define TASK_NAME_SOCKET_CLI     "socket_cli"

// Socket configurations
#define SOCKET_SRV_TOUT          1000
#define SOCKET_SRV_BUF_SIZE      256
#define SOCKET_SRV_PORT          3333
#define SOCKET_CLI_PORT          3333

// LED counts
#define LED_PTA_NUM              2
#define LED_PTC_NUM              8
#define LED_PTB_NUM              9

// Maximum number allowed for LED control (8 bits)
#define MAX_LED_NUMBER           255

// Pair of GPIO port and LED pin.
struct LED_Data
{
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};

// All PTAx LEDs in array
LED_Data g_led_pta[LED_PTA_NUM] =
        {
                { LED_PTA1_PIN, LED_PTA1_GPIO },
                { LED_PTA2_PIN, LED_PTA2_GPIO },
        };

// All PTCx LEDs in array
LED_Data g_led_ptc[LED_PTC_NUM] =
        {
                { LED_PTC0_PIN, LED_PTC0_GPIO },
                { LED_PTC1_PIN, LED_PTC1_GPIO },
                { LED_PTC2_PIN, LED_PTC2_GPIO },
                { LED_PTC3_PIN, LED_PTC3_GPIO },
                { LED_PTC4_PIN, LED_PTC4_GPIO },
                { LED_PTC5_PIN, LED_PTC5_GPIO },
                { LED_PTC7_PIN, LED_PTC7_GPIO },
                { LED_PTC8_PIN, LED_PTC8_GPIO },
        };

// Global variable for LED number control
volatile uint32_t g_led_number = 0;

// Random number generator for TCP/IP stack
BaseType_t xApplicationGetRandomNumber(uint32_t *tp_pul_number) { return uxRand(); }

// Some task stack overflow
void vApplicationStackOverflowHook(xTaskHandle *tp_task_handle, signed portCHAR *tp_task_name)
{
PRINTF("STACK PROBLEM %s.\r\n", tp_task_name);
}

// Task to blink PTA LEDs alternatively
void task_led_pta_blink(void *t_arg)
{
    uint32_t l_inx = 0;

    while (1)
    {
        // Switch LED on
        GPIO_PinWrite(g_led_pta[l_inx].m_led_gpio, g_led_pta[l_inx].m_led_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        // Switch LED off
        GPIO_PinWrite(g_led_pta[l_inx].m_led_gpio, g_led_pta[l_inx].m_led_pin, 0);
        // Next LED
        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

// Task to monitor and update PTC LEDs based on g_led_number
void task_led_control(void *pvParameters)
{
    uint32_t previous_value = 0;

    while (1)
    {
        // Get current value
        uint32_t current_value = g_led_number;

        // Check if the value has changed
        if (current_value != previous_value)
        {
            // Iterate through all PTC LEDs
            for (int i = 0; i < LED_PTC_NUM; i++)
            {
                // Check if the i-th bit is set
                if (current_value & (1 << i))
                {
                    // Turn on LED
                    GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 1);
                }
                else
                {
                    // Turn off LED
                    GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 0);
                }
            }

            // Update previous value
            previous_value = current_value;
        }

        // Delay before next check
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task socket server with LED command parsing
void task_socket_srv(void *tp_arg)
{
    PRINTF("Task socket server started.\r\n");
    TickType_t l_receive_tout = SOCKET_SRV_TOUT / portTICK_PERIOD_MS;

    int l_port = (int)tp_arg;
    struct freertos_sockaddr l_srv_address;

    // Set listening port
    l_srv_address.sin_port = FreeRTOS_htons(l_port);
    l_srv_address.sin_addr = FreeRTOS_inet_addr_quick(0, 0, 0, 0);

    xSocket_t l_sock_listen;
    xSocket_t l_sock_client;
    xWinProperties_t l_win_props;
    struct freertos_sockaddr from;
    socklen_t fromSize = sizeof(from);
    BaseType_t l_bind_result;
    int8_t l_rx_buf[SOCKET_SRV_BUF_SIZE + 1]; // +1 for null terminator

    /* Create a socket. */
    l_sock_listen = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    configASSERT(l_sock_listen != FREERTOS_INVALID_SOCKET);

    l_bind_result = FreeRTOS_bind(l_sock_listen, &l_srv_address, sizeof(l_srv_address));
    configASSERT(l_bind_result == 0);

    // Set timeouts
    FreeRTOS_setsockopt(l_sock_listen, 0, FREERTOS_SO_RCVTIMEO, &l_receive_tout, sizeof(l_receive_tout));
    FreeRTOS_setsockopt(l_sock_listen, 0, FREERTOS_SO_SNDTIMEO, &l_receive_tout, sizeof(l_receive_tout));

    memset(&l_win_props, '\0', sizeof(l_win_props));
    l_win_props.lTxBufSize = SOCKET_SRV_BUF_SIZE;
    l_win_props.lTxWinSize = 2;
    l_win_props.lRxBufSize = SOCKET_SRV_BUF_SIZE;
    l_win_props.lRxWinSize = 2;

    FreeRTOS_setsockopt(l_sock_listen, 0, FREERTOS_SO_WIN_PROPERTIES, (void *)&l_win_props, sizeof(l_win_props));

    FreeRTOS_listen(l_sock_listen, 2);

    PRINTF("Socket server started, listening on port %u.\r\n", FreeRTOS_ntohs(l_srv_address.sin_port));

    for (;;)
    {
        uint32_t l_reply_count = 0;

        // Wait for a client
        do
        {
            l_sock_client = FreeRTOS_accept(l_sock_listen, &from, &fromSize);
            vTaskDelay(pdMS_TO_TICKS(SOCKET_SRV_TOUT));
        } while (l_sock_client == NULL);

        if (l_sock_client == FREERTOS_INVALID_SOCKET)
        {
            PRINTF("Invalid socket from ACCEPT!\r\n");
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        // Handle client requests
        for (l_reply_count = 0; pdTRUE; l_reply_count++)
        {
            BaseType_t l_len;

            // Receive data
            l_len = FreeRTOS_recv(l_sock_client, l_rx_buf, SOCKET_SRV_BUF_SIZE, 0);

            if (l_len > 0)
            {
                // Null-terminate the received data
                if (l_len < SOCKET_SRV_BUF_SIZE)
                {
                    l_rx_buf[l_len] = '\0';
                }
                else
                {
                    l_rx_buf[SOCKET_SRV_BUF_SIZE] = '\0';
                }

                PRINTF("Received: %s\r\n", l_rx_buf);

                // Parse the command
                if (strncmp((char *)l_rx_buf, "LED ", 4) == 0)
                {
                    // Get the number after "LED "
                    uint32_t led_num = atoi((char *)&l_rx_buf[4]);

                    // Limit to maximum allowed value
                    if (led_num > MAX_LED_NUMBER)
                    {
                        led_num = MAX_LED_NUMBER;
                    }

                    // Update the global LED number
                    g_led_number = led_num;

                    // Send confirmation
                    char confirm_msg[50];
                    snprintf(confirm_msg, sizeof(confirm_msg), "LED set to %u\r\n", led_num);
                    FreeRTOS_send(l_sock_client, confirm_msg, strlen(confirm_msg), 0);
                }
                else
                {
                    // Echo back the received data
                    l_len = FreeRTOS_send(l_sock_client, (void *)l_rx_buf, strlen((char *)l_rx_buf), 0);
                    PRINTF("Server forwarded %d bytes.\r\n", l_len);
                }
            }

            if (l_len < 0)
            {
                // Error occurred
                break;
            }
            if (l_len == 0)
            {
                PRINTF("Recv timeout.\r\n");
                break;
            }
        }
        PRINTF("Socket server replied %d times.\r\n", l_reply_count);

        // Pause before handling next client
        vTaskDelay(pdMS_TO_TICKS(SOCKET_SRV_TOUT));

        // Close the client socket
        FreeRTOS_closesocket(l_sock_client);
        l_sock_client = NULL;
    }
}

void task_socket_cli(void *tp_arg)
{
    PRINTF("Task socket client started.\r\n");

    vTaskDelay(pdMS_TO_TICKS(500));

    Socket_t l_sock_server = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    struct freertos_sockaddr *lp_sever_addr = (struct freertos_sockaddr *)tp_arg;
    BaseType_t l_res = FreeRTOS_connect(l_sock_server, lp_sever_addr, sizeof(struct freertos_sockaddr));

    if (l_res == 0)
    {
        PRINTF("Connected to server.\r\n");

        for (int i = 0; i < 10; i++)
        {
            l_res = FreeRTOS_send(l_sock_server, "Hello\n", 6, 0);
            PRINTF("Sent %d bytes.\r\n", l_res);
            if (l_res < 0) break;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    else
    {
        PRINTF("Unable to connect to server!\r\n");
    }

    FreeRTOS_closesocket(l_sock_server);

    vTaskDelete(NULL);
}

// Callback from TCP stack - interface state changed
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t t_network_event)
{
    static BaseType_t s_task_already_created = pdFALSE;
    static struct freertos_sockaddr s_server_addr;
    s_server_addr.sin_port = FreeRTOS_htons(SOCKET_CLI_PORT);
    s_server_addr.sin_addr = FreeRTOS_inet_addr_quick(10, 0, 0, 1);

    // Both eNetworkUp and eNetworkDown events can be processed here.
    if (t_network_event == eNetworkUp)
    {
        PRINTF("Network interface UP.\r\n");
        // Create the tasks that use the TCP/IP stack if they have not already been created.
        if (s_task_already_created == pdFALSE)
        {
            // Create socket server task
            if (xTaskCreate(task_socket_srv, TASK_NAME_SOCKET_SRV, configMINIMAL_STACK_SIZE + 1024,
                            (void *)SOCKET_SRV_PORT, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
            {
                PRINTF("Unable to create task %s.\r\n", TASK_NAME_SOCKET_SRV);
            }
            // Create socket client task
            if (xTaskCreate(task_socket_cli, TASK_NAME_SOCKET_CLI, configMINIMAL_STACK_SIZE + 1024,
                            &s_server_addr, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
            {
                PRINTF("Unable to create task %s.\r\n", TASK_NAME_SOCKET_CLI);
            }
            s_task_already_created = pdTRUE;
        }
    }
}

/*
 * @brief   Application entry point.
 */
int main(void)
{
    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();

    SYSMPU_Enable(SYSMPU, false);

    PRINTF("FreeRTOS+TCP with LED Control started.\r\n");

    // SET CORRECTLY MAC ADDRESS FOR USAGE IN LAB!
    //
    // Computer in lab use IP address 158.196.XXX.YYY.
    // Set MAC to 5A FE C0 DE XXX YYY+21
    // IP address will be configured from DHCP
    //
    uint8_t ucMAC[ipMAC_ADDRESS_LENGTH_BYTES] = { 0x5A, 0xFE, 0xC0, 0xDE, 0x8E, 0x67 };

    uint8_t ucIPAddress[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 10 };
    uint8_t ucIPMask[ipIP_ADDRESS_LENGTH_BYTES] = { 255, 255, 255, 0 };
    uint8_t ucIPGW[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 1 };

    FreeRTOS_IPInit(ucIPAddress, ucIPMask, ucIPGW, NULL, ucMAC);

    // Create PTA LED blinking task
    if (xTaskCreate(
            task_led_pta_blink,
            TASK_NAME_LED_PTA,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_LED_PTA);
    }

    // Create LED control task
    if (xTaskCreate(
            task_led_control,
            TASK_NAME_LED_CONTROL,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_LED_CONTROL);
    }

    vTaskStartScheduler();

    while (1)
    {
    }

    return 0;
}
