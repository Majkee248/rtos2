******
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 02/2022
// Modified by:  [Your Name], [Your Email], [Date]
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         Task control demo program with brightness control.
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
#define LOW_TASK_PRIORITY          (configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY         (configMAX_PRIORITIES)

// Task names.
#define TASK_NAME_SWITCHES         "switches"
#define TASK_NAME_LED_PTA          "led_pta"
#define TASK_NAME_LED_SNAKE_L      "led_snake_l"
#define TASK_NAME_LED_SNAKE_R      "led_snake_r"
#define TASK_NAME_LED_BRIGHTNESS   "led_brightness"

// LED counts.
#define LED_PTA_NUM                2
#define LED_PTC_NUM                8
#define LED_PTB_NUM                9

// Brightness control.
#define MAX_BRIGHTNESS             100
#define BRIGHTNESS_STEP            10

// Define indices for RGB LEDs.
#define FIRST_RGB_LED_INDEX        0
#define THIRD_RGB_LED_INDEX        2

// Pair of GPIO port and LED pin.
struct LED_Data
{
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};


uint32_t pos;

// All PTAx LEDs in array.
LED_Data g_led_pta[LED_PTA_NUM] =
        {
                {LED_PTA1_PIN, LED_PTA1_GPIO},
                {LED_PTA2_PIN, LED_PTA2_GPIO},
        };

// All PTCx LEDs in array.
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

// All RGB LEDs in array (assuming 3 RGB LEDs, each with R, G, B pins).
LED_Data g_led_rgb[LED_PTB_NUM] =
        {
                // RGB LED 1
                {LED_PTB2_PIN, LED_PTB2_GPIO},    // Red
                {LED_PTB3_PIN, LED_PTB3_GPIO},    // Green
                {LED_PTB9_PIN, LED_PTB9_GPIO},    // Blue
                // RGB LED 2
                {LED_PTB10_PIN, LED_PTB10_GPIO},  // Red
                {LED_PTB11_PIN, LED_PTB11_GPIO},  // Green
                {LED_PTB18_PIN, LED_PTB18_GPIO},  // Blue
                // RGB LED 3
                {LED_PTB19_PIN, LED_PTB19_GPIO},  // Red
                {LED_PTB20_PIN, LED_PTB20_GPIO},  // Green
                {LED_PTB23_PIN, LED_PTB23_GPIO},  // Blue
        };

// Brightness levels for the first and third RGB LEDs.
uint8_t brightness_levels[2] = {MAX_BRIGHTNESS, MAX_BRIGHTNESS}; // [0] - First RGB, [1] - Third RGB

// Function to update LED brightness for the first and third RGB LEDs.
void update_led_brightness()
{
    // First RGB LED
    uint8_t brightness_first = brightness_levels[0];
    GPIO_PinWrite(g_led_rgb[FIRST_RGB_LED_INDEX * 3].m_led_gpio, g_led_rgb[FIRST_RGB_LED_INDEX * 3].m_led_pin, brightness_first > 0);
    GPIO_PinWrite(g_led_rgb[FIRST_RGB_LED_INDEX * 3 + 1].m_led_gpio, g_led_rgb[FIRST_RGB_LED_INDEX * 3 + 1].m_led_pin, brightness_first > 10);
    GPIO_PinWrite(g_led_rgb[FIRST_RGB_LED_INDEX * 3 + 2].m_led_gpio, g_led_rgb[FIRST_RGB_LED_INDEX * 3 + 2].m_led_pin, brightness_first == MAX_BRIGHTNESS);

    // Third RGB LED
    uint8_t brightness_third = brightness_levels[1];
    GPIO_PinWrite(g_led_rgb[THIRD_RGB_LED_INDEX * 3].m_led_gpio, g_led_rgb[THIRD_RGB_LED_INDEX * 3].m_led_pin, brightness_third > 0);
    GPIO_PinWrite(g_led_rgb[THIRD_RGB_LED_INDEX * 3 + 1].m_led_gpio, g_led_rgb[THIRD_RGB_LED_INDEX * 3 + 1].m_led_pin, brightness_third > 10);
    GPIO_PinWrite(g_led_rgb[THIRD_RGB_LED_INDEX * 3 + 2].m_led_gpio, g_led_rgb[THIRD_RGB_LED_INDEX * 3 + 2].m_led_pin, brightness_third == MAX_BRIGHTNESS);
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
            // Get current state of the pin
            uint32_t current_state = GPIO_PinRead(g_led_pta[i].m_led_gpio, g_led_pta[i].m_led_pin);
            // Toggle the pin state
            GPIO_PinWrite(g_led_pta[i].m_led_gpio, g_led_pta[i].m_led_pin, !current_state);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

// Task to control snake effect from left
void task_snake_left(void *t_arg)
{
    while (1)
    {
        vTaskSuspend(0); // Suspend task
        if(pos == 0){
            for (int i = 0; i < LED_PTC_NUM - 1; i++) // Ensure safe access to the array
            {
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 1);
                GPIO_PinWrite(g_led_ptc[i + 1].m_led_gpio, g_led_ptc[i + 1].m_led_pin, 1);
                vTaskDelay(100);
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 0);
            }
            // Last LED stays on
            GPIO_PinWrite(g_led_ptc[7].m_led_gpio, g_led_ptc[7].m_led_pin, 1);
        }
        pos = 7;


    }
}

// Task to control snake effect from right
void task_snake_right(void *t_arg)
{
    while (1)
    {
        vTaskSuspend(0); // Suspend task
        if(pos == 7){
            for (int i = LED_PTC_NUM - 1; i > 0; i--) // Ensure safe access to the array
            {
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 1);
                GPIO_PinWrite(g_led_ptc[i - 1].m_led_gpio, g_led_ptc[i - 1].m_led_pin, 1);
                vTaskDelay(100);
                GPIO_PinWrite(g_led_ptc[i].m_led_gpio, g_led_ptc[i].m_led_pin, 0);
            }

            GPIO_PinWrite(g_led_ptc[0].m_led_gpio, g_led_ptc[0].m_led_pin, 1);
        }
        pos = 0;
    }
}

// Task to monitor switches and control LED tasks
void task_switches(void *t_arg)
{
    TaskHandle_t handle_led_snake_l = xTaskGetHandle(TASK_NAME_LED_SNAKE_L);
    TaskHandle_t handle_led_snake_r = xTaskGetHandle(TASK_NAME_LED_SNAKE_R);
    TaskHandle_t handle_led_brightness = xTaskGetHandle(TASK_NAME_LED_BRIGHTNESS);

    while (1)
    {
        // Decrease brightness on PTC9
        if (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0)
        {
            // Decrease brightness of first RGB LED
            if (brightness_levels[0] >= BRIGHTNESS_STEP)
            {
                brightness_levels[0] -= BRIGHTNESS_STEP;
            }
            else
            {
                brightness_levels[0] = 0;
            }

            // Increase brightness of third RGB LED
            if (brightness_levels[1] + BRIGHTNESS_STEP <= MAX_BRIGHTNESS)
            {
                brightness_levels[1] += BRIGHTNESS_STEP;
            }
            else
            {
                brightness_levels[1] = MAX_BRIGHTNESS;
            }

            // Resume brightness update task
            if (handle_led_brightness && eTaskGetState(handle_led_brightness) == eSuspended)
            {
                vTaskResume(handle_led_brightness);
            }

            vTaskDelay(300 / portTICK_PERIOD_MS); // Debounce delay
        }

        // Increase brightness on PTC10
        if (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0)
        {
            // Increase brightness of first RGB LED
            if (brightness_levels[0] + BRIGHTNESS_STEP <= MAX_BRIGHTNESS)
            {
                brightness_levels[0] += BRIGHTNESS_STEP;
            }
            else
            {
                brightness_levels[0] = MAX_BRIGHTNESS;
            }

            // Decrease brightness of third RGB LED
            if (brightness_levels[1] >= BRIGHTNESS_STEP)
            {
                brightness_levels[1] -= BRIGHTNESS_STEP;
            }
            else
            {
                brightness_levels[1] = 0;
            }

            // Resume brightness update task
            if (handle_led_brightness && eTaskGetState(handle_led_brightness) == eSuspended)
            {
                vTaskResume(handle_led_brightness);
            }

            vTaskDelay(300 / portTICK_PERIOD_MS); // Debounce delay
        }

        if (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0){
            if(handle_led_snake_l)
                vTaskResume(handle_led_snake_l);
        }

        if (GPIO_PinRead(SW_PTC12_GPIO, SW_PTC12_PIN) == 0){
            if(handle_led_snake_r)
                vTaskResume(handle_led_snake_r);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Short delay to reduce CPU usage
    }
}

// Start application
int main(void)
{
    // Initialize board hardware.
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("FreeRTOS task demo program with brightness control.\r\n");

    // Initialize LED brightness
    update_led_brightness();

    // Create PTA blink task
    if (xTaskCreate(task_led_pta_blink, TASK_NAME_LED_PTA, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_PTA);
    }

    // Create snake left task
    if (xTaskCreate(task_snake_left, TASK_NAME_LED_SNAKE_L, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_L);
    }

    // Create snake right task
    if (xTaskCreate(task_snake_right, TASK_NAME_LED_SNAKE_R, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R);
    }

    // Create brightness control task
    if (xTaskCreate(task_led_brightness, TASK_NAME_LED_BRIGHTNESS, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_LED_BRIGHTNESS);
    }

    // Create switches monitoring task
    if (xTaskCreate(task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS)
    {
        PRINTF("Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES);
    }

    // Start the scheduler
    vTaskStartScheduler();

    // Infinite loop if scheduler fails
    while (1)
        ;

    return 0;
}