#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_sysmpu.h"
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#define LOW_TASK_PRIORITY         (configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY     (configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY         (configMAX_PRIORITIES)

#define TASK_NAME_LED_PTA        "led_pta"
#define TASK_NAME_SOCKET_SRV    "socket_srv"
#define TASK_NAME_SOCKET_CLI    "socket_cli"
#define TASK_NAME_SET_ONOFF    "set_onoff"
#define TASK_NAME_MONITOR_BUTTONS "monitor_buttons"
#define TASK_NAME_PRINT_BUTTONS   "print_buttons"

typedef enum { LEFT, RIGHT } Direction_t;

xSocket_t l_sock_client;

#define SOCKET_SRV_TOUT            1000
#define SOCKET_SRV_BUF_SIZE        256
#define SOCKET_SRV_PORT            3333

#define SOCKET_CLI_PORT            3333

#define BUT_NUM         4
#define LED_PTA_NUM     2
#define LED_PTC_NUM     4
#define LED_PTB_NUM     9

struct LED_Data {
    uint32_t pin;
    GPIO_Type *gpio;
};

LED_Data pta[ LED_PTA_NUM ] = {
    { LED_PTA1_PIN, LED_PTA1_GPIO },
    { LED_PTA2_PIN, LED_PTA2_GPIO },
};

LED_Data ptc[ LED_PTC_NUM ] = {
    { LED_PTC0_PIN, LED_PTC0_GPIO },
    { LED_PTC1_PIN, LED_PTC1_GPIO },
    { LED_PTC2_PIN, LED_PTC2_GPIO },
    { LED_PTC3_PIN, LED_PTC3_GPIO },
};

struct CUSTOM_LED {
    bool state;
    uint32_t pin;
    GPIO_Type *gpio;
};

CUSTOM_LED ptc_bool[ LED_PTC_NUM ] = {
    { false, LED_PTC0_PIN, LED_PTC0_GPIO },
    { false, LED_PTC1_PIN, LED_PTC1_GPIO },
    { false, LED_PTC2_PIN, LED_PTC2_GPIO },
    { false, LED_PTC3_PIN, LED_PTC3_GPIO },
};

struct CUSTOM_BUT {
    bool state;
    bool change;
    bool released;
    unsigned int pin;
    GPIO_Type *gpio;
};

CUSTOM_BUT but_bool[BUT_NUM] = {
    {false, false, false, SW_PTC9_PIN, SW_PTC9_GPIO},
    {false, false, false, SW_PTC10_PIN, SW_PTC10_GPIO},
    {false, false, false, SW_PTC11_PIN, SW_PTC11_GPIO},
    {false, false, false, SW_PTC12_PIN, SW_PTC12_GPIO}
};

void task_led_pta_blink(void *t_arg);
void task_socket_srv(void *tp_arg);
void task_socket_cli(void *tp_arg);
void task_set_onoff(void *tp_arg);
void task_monitor_buttons(void *tp_arg);
void task_print_buttons(void *tp_arg);

BaseType_t xApplicationGetRandomNumber(uint32_t *tp_pul_number) { return uxRand(); }

void vApplicationStackOverflowHook(xTaskHandle *tp_task_handle, signed portCHAR *tp_task_name) {
    PRINTF("STACK PROBLEM %s.\r\n", tp_task_name);
}

void task_led_pta_blink(void *t_arg) {
    uint32_t l_inx = 0;

    while (1) {
        GPIO_PinWrite(pta[l_inx].gpio, pta[l_inx].pin, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        GPIO_PinWrite(pta[l_inx].gpio, pta[l_inx].pin, 0);
        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

void task_set_onoff(void *tp_arg) {
    while (1) {
        for (int i = 0; i < LED_PTC_NUM; i++) {
            GPIO_PinWrite(ptc_bool[i].gpio, ptc_bool[i].pin, ptc_bool[i].state);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void msg() {
    const char *commands[] = {
        "LED L 1",
        "LED L 2",
        "LED L 3",
        "LED L 4"
    };

    for (int i = 0; i < 4; i++) {
        PRINTF("%s\n", commands[i]);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void task_led_sequence(void *tp_arg) {
    msg();
    vTaskDelete(NULL);
}

void task_monitor_buttons(void *tp_arg) {
    for (int i = 0; i < BUT_NUM; i++) {
        but_bool[i].change = false;
        but_bool[i].released = true;
        but_bool[i].state = false;
    }

    while (1) {
        for (int i = 0; i < BUT_NUM; i++) {
            bool state = GPIO_PinRead(but_bool[i].gpio, but_bool[i].pin);
            bool prev_state = but_bool[i].state;

            but_bool[i].state = (state == 0);

            if (but_bool[i].state != prev_state) {
                but_bool[i].change = true;
                but_bool[i].released = !but_bool[i].state;

                if (i == 0 && but_bool[i].state) {
                    if (xTaskCreate(
                            task_led_sequence,
                            "led_sequence",
                            configMINIMAL_STACK_SIZE + 100,
                            NULL,
                            NORMAL_TASK_PRIORITY,
                            NULL) != pdPASS) {
                        PRINTF("Unable to create task 'led_sequence'.\r\n");
                    }
                }
            } else {
                but_bool[i].change = false;
            }
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void task_print_buttons(void *tp_arg) {
    bool enter = false;
    char msg[64];

    while (true) {
        strcpy(msg, "BTN ");
        for (int i = 0; i < BUT_NUM; ++i) {
            sprintf(msg + strlen(msg), "%d", but_bool[i].state ? 1 : 0);
            if (!enter && but_bool[i].change) {
                enter = true;
            }
            but_bool[i].change = false;
        }
        strcat(msg, "\n");

        if (enter) {
            if (l_sock_client != FREERTOS_INVALID_SOCKET) {
                FreeRTOS_send(l_sock_client, (void *)msg, strlen(msg) + 1, 0);
                PRINTF("Sent Button States: %s", msg);
            }
            enter = false;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    SYSMPU_Enable(SYSMPU, false);

    PRINTF("FreeRTOS+TCP with Left/Right LED Control started.\r\n");

    uint8_t ucMAC[ipMAC_ADDRESS_LENGTH_BYTES] = { 0x5A, 0xFE, 0xC0, 0xDE, 142, 100 };

    uint8_t ucIPAddress[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 10 };
    uint8_t ucIPMask[ipIP_ADDRESS_LENGTH_BYTES] = { 255, 255, 255, 0 };
    uint8_t ucIPGW[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 1 };

    FreeRTOS_IPInit(ucIPAddress, ucIPMask, ucIPGW, NULL, ucMAC);

    if (xTaskCreate(
            task_led_pta_blink,
            TASK_NAME_LED_PTA,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_LED_PTA);
    }

    if (xTaskCreate(
            task_set_onoff,
            TASK_NAME_SET_ONOFF,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_SET_ONOFF);
    }

    if (xTaskCreate(
            task_monitor_buttons,
            TASK_NAME_MONITOR_BUTTONS,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_MONITOR_BUTTONS);
    }

    if (xTaskCreate(
            task_print_buttons,
            TASK_NAME_PRINT_BUTTONS,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_PRINT_BUTTONS);
    }

    vTaskStartScheduler();

    while (1);

    return 0;
}