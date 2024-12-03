// **************************************************************************
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 02/2022
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         Task control demo program.
//
// **************************************************************************

// FreeRTOS kernel includes.
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

// System includes.
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

// Task priorities.
#define LOW_TASK_PRIORITY 		(configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY 	(configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY 		(configMAX_PRIORITIES)

#define TASK_NAME_SWITCHES		"switches"
#define TASK_NAME_LED_PTA		"led_pta"
#define TASK_NAME_LED_SNAKE_L	"led_snake_l"
#define TASK_NAME_LED_SNAKE_R	"led_snake_r"
#define TASK_NAME_LED_BRIGHTNESS "led_brightness"

#define TASK_NAME_TWO_HEAD "snake_two_head"

#define LED_PTA_NUM 	2
#define LED_PTC_NUM		8
#define LED_PTB_NUM		9

#define MAX_BRIGHTNESS 100
#define BRIGHTNESS_STEP 20

// pair of GPIO port and LED pin.
struct LED_Data
{
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};

// all PTAx LEDs in array
LED_Data g_led_pta[LED_PTA_NUM] =
        {
                {LED_PTA1_PIN, LED_PTA1_GPIO},
                {LED_PTA2_PIN, LED_PTA2_GPIO},
        };

// all PTCx LEDs in array
LED_Data g_led_ptc[LED_PTC_NUM] =
        {
                {LED_PTC0_PIN, LED_PTC0_GPIO},
                {LED_PTC1_PIN, LED_PTC1_GPIO},
                {LED_PTC2_PIN, LED_PTC2_GPIO},
                {LED_PTC3_PIN, LED_PTC3_GPIO},
                {LED_PTC4_PIN, LED_PTC4_GPIO},
                {LED_PTC5_PIN, LED_PTC5_GPIO},
                {LED_PTC7_PIN, LED_PTC7_GPIO},
                {LED_PTC8_PIN, LED_PTC8_GPIO},
        };

LED_Data g_led_rgb[LED_PTB_NUM] =
        {
                {LED_PTB2_PIN, LED_PTB2_GPIO},
                {LED_PTB3_PIN, LED_PTB3_GPIO},
                {LED_PTB9_PIN, LED_PTB9_GPIO},
                {LED_PTB10_PIN, LED_PTB10_GPIO},
                {LED_PTB11_PIN, LED_PTB11_GPIO},
                {LED_PTB18_PIN, LED_PTB18_GPIO},
                {LED_PTB19_PIN, LED_PTB19_GPIO},
                {LED_PTB20_PIN, LED_PTB20_GPIO},
                {LED_PTB23_PIN, LED_PTB23_GPIO},
        };

// Brightness levels for the three RGB lights
uint8_t brightness_levels[3] = {MAX_BRIGHTNESS, 0, 0};

// Function to update LED brightness
void update_led_brightness()
{
    for (int i = 0; i < 3; i++)
    {
        GPIO_PinWrite(g_led_rgb[i * 3].m_led_gpio, g_led_rgb[i * 3].m_led_pin, brightness_levels[i] > 0);
        GPIO_PinWrite(g_led_rgb[i * 3 + 1].m_led_gpio, g_led_rgb[i * 3 + 1].m_led_pin, brightness_levels[i] > 50);
        GPIO_PinWrite(g_led_rgb[i * 3 + 2].m_led_gpio, g_led_rgb[i * 3 + 2].m_led_pin, brightness_levels[i] == MAX_BRIGHTNESS);
    }
}

// Task to control LED brightness
void task_led_brightness(void *t_arg)
{
    while (1)
    {
        update_led_brightness();
        vTaskSuspend(NULL); // Suspend itself until resumed by another task
    }
}

// Task to blink PTA LEDs
void task_led_pta_blink(void *t_arg)
{
    while (1)
    {
        for (int i = 0; i < LED_PTA_NUM; i++)
        {
            GPIO_PinToggle(g_led_pta[i].m_led_gpio, g_led_pta[i].m_led_pin);
            vTaskDelay(500);
        }
    }
}

// Task to control snake effect from left
void task_snake_left(void *t_arg)
{
    int type = 0;

    if(type = 7){

        while (1)
        {
            for (int i = 0; i < LED_PTC_NUM; i++)
            {
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 1);
                GPIO_PinWrite(g_led_ptc[i+1].m_led_gpio, g_led_ptc[i+1].m_led_pin, 1);
                vTaskDelay(100);
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 0);
            }
            vTaskSuspend(NULL);
        }
    }

}

// Task to control snake effect from right
void task_snake_right(void *t_arg)
{
    while (1)
    {
        for (int i = LED_PTC_NUM - 1; i >= 0; i--)
        {
            GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 1);
            vTaskDelay(100);
            GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 0);
        }
        vTaskSuspend(NULL);
    }
}

void task_snake_two_head(void *t_arg)
{
    uint8_t direction = 1; // 1 for right, -1 for left
    uint8_t head_position = 0;

    while (1)
    {
        if (direction == 1) // Moving right
        {
            for (head_position = 0; head_position < LED_PTC_NUM - 1; head_position++)
            {
                // Turn on two LEDs for the head
                GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 1);
                GPIO_PinWrite(g_led_ptc[head_position + 1].m_led_gpio, g_led_ptc[head_position + 1].m_led_pin, 1);

                vTaskDelay(100);

                // Turn off the tail LED
                GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 0);
            }

            // Leave one LED lit at the end
            GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 1);

            // Suspend task and wait for direction change
            vTaskSuspend(NULL);
        }
        else if (direction == -1) // Moving left
        {
            for (head_position = LED_PTC_NUM - 1; head_position > 0; head_position--)
            {
                // Turn on two LEDs for the head
                GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 1);
                GPIO_PinWrite(g_led_ptc[head_position - 1].m_led_gpio, g_led_ptc[head_position - 1].m_led_pin, 1);

                vTaskDelay(100);

                // Turn off the tail LED
                GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 0);
            }

            // Leave one LED lit at the start
            GPIO_PinWrite(g_led_ptc[head_position].m_led_gpio, g_led_ptc[head_position].m_led_pin, 1);

            // Suspend task and wait for direction change
            vTaskSuspend(NULL);
        }
    }
}

// This task monitors switches and controls others led_* tasks
void task_switches(void *t_arg)
{
    TaskHandle_t l_handle_led_pta = xTaskGetHandle(TASK_NAME_LED_PTA);
    TaskHandle_t l_handle_led_snake_l = xTaskGetHandle(TASK_NAME_LED_SNAKE_L);
    TaskHandle_t l_handle_led_snake_r = xTaskGetHandle(TASK_NAME_LED_SNAKE_R);
    TaskHandle_t l_handle_led_brightness = xTaskGetHandle(TASK_NAME_LED_BRIGHTNESS);
    TaskHandle_t l_handle_snake_two_head = xTaskGetHandle(TASK_NAME_TWO_HEAD);


    while (1)
    {
        // Adjust brightness on PTC9
        if (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0)
        {
            for (int i = 0; i < 3; i++)
            {
                if (brightness_levels[i] > 0)
                {
                    brightness_levels[i] -= BRIGHTNESS_STEP;
                    brightness_levels[(i + 1) % 3] += BRIGHTNESS_STEP;
                    break;
                }
            }
            if (l_handle_led_brightness)
                vTaskResume(l_handle_led_brightness);

            vTaskDelay(300);
        }

        // Resume other tasks based on switches
        // Move snake to the left
        if (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0)
        {
            if (l_handle_snake_two_head)
            {
                vTaskResume(l_handle_snake_two_head);
                vTaskDelay(300);
            }
        }

        // Move snake to the right
        if (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0)
        {
            if (l_handle_snake_two_head)
            {
                vTaskResume(l_handle_snake_two_head);
                vTaskDelay(300);
            }
        }

        if (GPIO_PinRead(SW_PTC12_GPIO, SW_PTC12_PIN) == 0 && l_handle_led_snake_r)
            vTaskResume(l_handle_led_snake_r);

        vTaskDelay(1);
    }
}

// Start application
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("FreeRTOS task demo program.\r\n");

    update_led_brightness();

    if (xTaskCreate(task_led_pta_blink, TASK_NAME_LED_PTA, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_PTA);

    if (xTaskCreate(task_snake_left, TASK_NAME_LED_SNAKE_L, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_L);

    if (xTaskCreate(task_snake_right, TASK_NAME_LED_SNAKE_R, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R);

    if (xTaskCreate(task_led_brightness, TASK_NAME_LED_BRIGHTNESS, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_BRIGHTNESS);

    if (xTaskCreate(task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES);

    if (xTaskCreate(task_snake_two_head, TASK_NAME_TWO_HEAD, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
        PRINTF("Unable to create task 'snake_two_head'!\r\n");

    vTaskStartScheduler();

    while (1)
        ;

    return 0;
}
