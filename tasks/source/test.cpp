#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

#define NORMAL_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define DOUBLE_CLICK_TIMEOUT_MS 300
#define DOUBLE_CLICK_TIMEOUT_TICKS pdMS_TO_TICKS(DOUBLE_CLICK_TIMEOUT_MS)
#define DEBOUNCE_DELAY_MS 50

#define TASK_NAME_SWITCHES "switches"
#define TASK_NAME_ALL_ON "all_on"
#define TASK_NAME_ALL_OFF "all_off"

bool ptc9_prev_state = true;
bool ptc10_prev_state = true;

TaskHandle_t l_handle_led_all_on = NULL;
TaskHandle_t l_handle_led_all_off = NULL;

void task_all_on(void *t_arg) {
    while (1) {

        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
        }
        vTaskSuspend(NULL);
    }
}

void task_all_off(void *t_arg) {
    while (1) {
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
        }
        vTaskSuspend(NULL);
    }
}

void task_switches(void *t_arg) {
    TickType_t last_ptc9_click_time = 0;
    uint8_t ptc9_click_count = 0;
    TickType_t last_ptc10_click_time = 0;
    uint8_t ptc10_click_count = 0;

    while (1) {
        bool ptc9_current_state = (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0);
        if (ptc9_current_state && !ptc9_prev_state) {
            TickType_t current_time = xTaskGetTickCount();
            if (ptc9_click_count == 0) {
                ptc9_click_count = 1;
                last_ptc9_click_time = current_time;
            } else if (ptc9_click_count == 1) {
                if ((current_time - last_ptc9_click_time) <= DOUBLE_CLICK_TIMEOUT_TICKS) {
                    ptc9_click_count = 0;
                    if (l_handle_led_all_on) {
                        vTaskResume(l_handle_led_all_on);
                    }
                } else {
                    ptc9_click_count = 1;
                    last_ptc9_click_time = current_time;
                }
            }
        }
        ptc9_prev_state = ptc9_current_state;

        bool ptc10_current_state = (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0);
        if (ptc10_current_state && !ptc10_prev_state) {
            TickType_t current_time = xTaskGetTickCount();
            if (ptc10_click_count == 0) {
                ptc10_click_count = 1;
                last_ptc10_click_time = current_time;
            } else if (ptc10_click_count == 1) {
                if ((current_time - last_ptc10_click_time) <= DOUBLE_CLICK_TIMEOUT_TICKS) {
                    ptc10_click_count = 0;
                    if (l_handle_led_all_off) {
                        vTaskResume(l_handle_led_all_off);
                    }
                } else {
                    ptc10_click_count = 1;
                    last_ptc10_click_time = current_time;
                }
            }
        }
        ptc10_prev_state = ptc10_current_state;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("FreeRTOS task demo program with double-click control.\r\n");

    if (xTaskCreate(task_all_on, TASK_NAME_ALL_ON, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_ALL_ON);
    }

    if (xTaskCreate(task_all_off, TASK_NAME_ALL_OFF, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_ALL_OFF);
    }

    if (xTaskCreate(task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES);
    }

    vTaskStartScheduler();

    while (1);

    return 0;
}