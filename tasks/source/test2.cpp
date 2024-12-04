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

#define LOW_TASK_PRIORITY        (configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY     (configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY       (configMAX_PRIORITIES)

#define TASK_NAME_SWITCHES       "switches"
#define TASK_NAME_LED_PTA        "led_pta"
#define TASK_NAME_LED_SNAKE_L    "led_snake_l"
#define TASK_NAME_LED_SNAKE_R    "led_snake_r"
#define TASK_NAME_LED_SNAKE_BACK "led_snake_back"

#define LED_PTA_NUM 2
#define LED_PTC_NUM 8
#define LED_PTB_NUM 9

uint32_t site = 0;

struct LED_Data {
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};

LED_Data g_led_pta[LED_PTA_NUM] = {
    { LED_PTA1_PIN, LED_PTA1_GPIO },
    { LED_PTA2_PIN, LED_PTA2_GPIO },
};

LED_Data g_led_ptc[LED_PTC_NUM] = {
    { LED_PTC0_PIN, LED_PTC0_GPIO },
    { LED_PTC1_PIN, LED_PTC1_GPIO },
    { LED_PTC2_PIN, LED_PTC2_GPIO },
    { LED_PTC3_PIN, LED_PTC3_GPIO },
    { LED_PTC4_PIN, LED_PTC4_GPIO },
    { LED_PTC5_PIN, LED_PTC5_GPIO },
    { LED_PTC7_PIN, LED_PTC7_GPIO },
    { LED_PTC8_PIN, LED_PTC8_GPIO },
};

bool is_task_snake_l_running = false;
bool is_task_snake_r_running = false;
bool is_task_snake_back_running = false;

void task_led_pta_blink(void *t_arg) {
    uint32_t l_inx = 0;
    while (1) {
        GPIO_PinWrite(g_led_pta[l_inx].m_led_gpio, g_led_pta[l_inx].m_led_pin, 1);
        vTaskDelay(200);
        GPIO_PinWrite(g_led_pta[l_inx].m_led_gpio, g_led_pta[l_inx].m_led_pin, 0);
        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

void task_snake_left(void *t_arg) {
    is_task_snake_l_running = true;
    while (1) {
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
        }
        vTaskSuspend(NULL);
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        for (int inx = LED_PTC_NUM - 1; inx >= 0; inx--) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    is_task_snake_l_running = false;
}

void task_snake_right(void *t_arg) {
    is_task_snake_r_running = true;
    while (1) {
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
        }
        vTaskSuspend(NULL);
        for (int inx = LED_PTC_NUM - 1; inx >= 0; inx--) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    is_task_snake_r_running = false;
}

void task_snake_back(void *t_arg) {
    is_task_snake_back_running = true;
    while (1) {
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
        }
        vTaskSuspend(NULL);
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        for (int inx = LED_PTC_NUM - 1; inx >= 0; inx--) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        for (int inx = LED_PTC_NUM - 1; inx >= 0; inx--) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        for (int inx = 0; inx < LED_PTC_NUM; inx++) {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    is_task_snake_back_running = false;
}

void task_switches(void *t_arg) {
    TaskHandle_t l_handle_led_snake_l = xTaskGetHandle(TASK_NAME_LED_SNAKE_L);
    TaskHandle_t l_handle_led_snake_r = xTaskGetHandle(TASK_NAME_LED_SNAKE_R);
    TaskHandle_t l_handle_led_snake_back = xTaskGetHandle(TASK_NAME_LED_SNAKE_BACK);

    while (1) {
        if (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0 && !is_task_snake_l_running) {
            if (l_handle_led_snake_l) {
                vTaskResume(l_handle_led_snake_l);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        if (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0 && !is_task_snake_r_running) {
            if (l_handle_led_snake_r) {
                vTaskResume(l_handle_led_snake_r);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        if (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0 && !is_task_snake_back_running) {
            if (l_handle_led_snake_back) {
                vTaskResume(l_handle_led_snake_back);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("FreeRTOS task demo program.\r\n");
    PRINTF("Switches PTC9 and PTC10 will stop and run PTAx LEDs blinking.\r\n");
    PRINTF("Switches PTC11 and PTC12 will start snake on red LEDS from the left and right side.\r\n");

    if (xTaskCreate(task_led_pta_blink, TASK_NAME_LED_PTA, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_PTA);
    }

    if (xTaskCreate(task_snake_left, TASK_NAME_LED_SNAKE_L, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_L);
    }

    if (xTaskCreate(task_snake_right, TASK_NAME_LED_SNAKE_R, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R);
    }

    if (xTaskCreate(task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES);
    }

    if (xTaskCreate(task_snake_back, TASK_NAME_LED_SNAKE_BACK, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS) {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES);
    }

    vTaskStartScheduler();

    while (1);

    return 0;
}