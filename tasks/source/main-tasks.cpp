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
//
// All mapped LEDs and switches and their PINs and GPIOs:
// See schema in APPS syllabus.
//
// Switches:
// Name		PIN				GPIO
// PTC9		SW_PTC9_PIN		SW_PTC9_GPIO
// PTC10	SW_PTC10_PIN	SW_PTC10_GPIO
// PTC11	SW_PTC11_PIN	SW_PTC11_GPIO
// PTC12	SW_PTC12_PIN	SW_PTC12_GPIO
//
// LEDs:
// Name		PIN				GPIO
// PTA1		LED_PTA1_PIN   LED_PTA1_GPIO
// PTA2		LED_PTA2_PIN   LED_PTA2_GPIO
//
// PTC0		LED_PTC0_PIN   LED_PTC0_GPIO
// PTC1		LED_PTC1_PIN   LED_PTC1_GPIO
// PTC2		LED_PTC2_PIN   LED_PTC2_GPIO
// PTC3		LED_PTC3_PIN   LED_PTC3_GPIO
// PTC4		LED_PTC4_PIN   LED_PTC4_GPIO
// PTC5		LED_PTC5_PIN   LED_PTC5_GPIO
// PTC7		LED_PTC7_PIN   LED_PTC7_GPIO
// PTC8		LED_PTC8_PIN   LED_PTC8_GPIO
//
// PTB2		{LED_PTB2_PIN,   LED_PTB2_GPIO},
// PTB3		{LED_PTB3_PIN,  LED_PTB3_GPIO},
// PTB9		{LED_PTB9_PIN,   LED_PTB9_GPIO},
// PTB10	{LED_PTB10_PIN,  LED_PTB10_GPIO},
// PTB11	{LED_PTB11_PIN,  LED_PTB11_GPIO},
// PTB18	{LED_PTB18_PIN,  LED_PTB18_GPIO},
// PTB19	{LED_PTB19_PIN,  LED_PTB19_GPIO},
// PTB20	{LED_PTB20_PIN,  LED_PTB20_GPIO},
// PTB23	{LED_PTB23_PIN,  LED_PTB23_GPIO},


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
#define TASK_NAME_LIGHT  	 	"light_all"
#define TASK_NAME_LIGHT_OFF		"light_all_off"

#define LED_PTA_NUM 	2
#define LED_PTC_NUM		8
#define LED_PTB_NUM		9

// pair of GPIO port and LED pin.
struct LED_Data
{
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};

// all PTAx LEDs in array
LED_Data g_led_pta[ LED_PTA_NUM ] =
        {
                { LED_PTA1_PIN, LED_PTA1_GPIO },
                { LED_PTA2_PIN, LED_PTA2_GPIO },
        };

// all PTCx LEDs in array
LED_Data g_led_ptc[ LED_PTC_NUM ] =
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

LED_Data g_led_ptb[ LED_PTB_NUM ] =
        {
                {LED_PTB2_PIN,   LED_PTB2_GPIO},
                {LED_PTB3_PIN,  LED_PTB3_GPIO},
                {LED_PTB9_PIN,   LED_PTB9_GPIO},
                {LED_PTB10_PIN,  LED_PTB10_GPIO},
                {LED_PTB11_PIN,  LED_PTB11_GPIO},
                {LED_PTB18_PIN,  LED_PTB18_GPIO},
                {LED_PTB19_PIN,  LED_PTB19_GPIO},
                {LED_PTB20_PIN,  LED_PTB20_GPIO},
                {LED_PTB23_PIN,  LED_PTB23_GPIO},
        };

typedef struct {
    GPIO_Type* red_gpio;
    uint32_t red_pin;
    GPIO_Type* green_gpio;
    uint32_t green_pin;
    GPIO_Type* blue_gpio;
    uint32_t blue_pin;
} RGB_LED_Data;

RGB_LED_Data g_rgb_leds[3] = {
        // RGB0
        {
                .red_gpio = LED_PTB2_GPIO, .red_pin = LED_PTB2_PIN,
                .green_gpio = LED_PTB3_GPIO, .green_pin = LED_PTB3_PIN,
                .blue_gpio = LED_PTB9_GPIO, .blue_pin = LED_PTB9_PIN,
        },
        // RGB1
        {
                .red_gpio = LED_PTB10_GPIO, .red_pin = LED_PTB10_PIN,
                .green_gpio = LED_PTB11_GPIO, .green_pin = LED_PTB11_PIN,
                .blue_gpio = LED_PTB18_GPIO, .blue_pin = LED_PTB18_PIN,
        },
        // RGB2
        {
                .red_gpio = LED_PTB19_GPIO, .red_pin = LED_PTB19_PIN,
                .green_gpio = LED_PTB20_GPIO, .green_pin = LED_PTB20_PIN,
                .blue_gpio = LED_PTB23_GPIO, .blue_pin = LED_PTB23_PIN,
        }
};

// Global brightness variables for each RGB LED
volatile float brightness_rgb0 = 50.0f;
volatile float brightness_rgb1 = 50.0f;
volatile float brightness_rgb2 = 50.0f;

void set_rgb_brightness(uint8_t rgb_index, float brightness_percentage)
{
    // Ensure brightness is within 0 to 100%
    if (brightness_percentage > 100.0f)
        brightness_percentage = 100.0f;
    if (brightness_percentage < 0.0f)
        brightness_percentage = 0.0f;

    // Set the global brightness variable based on the rgb_index
    switch (rgb_index)
    {
        case 0:
            brightness_rgb0 = brightness_percentage;
            break;
        case 1:
            brightness_rgb1 = brightness_percentage;
            break;
        case 2:
            brightness_rgb2 = brightness_percentage;
            break;
        default:
            break;
    }
}

void task_rgb_control(void *t_arg)
{
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms period
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Calculate on times for each RGB LED
        TickType_t on_times[3];
        on_times[0] = (TickType_t)(brightness_rgb0 * xFrequency / 100.0f);
        on_times[1] = (TickType_t)(brightness_rgb1 * xFrequency / 100.0f);
        on_times[2] = (TickType_t)(brightness_rgb2 * xFrequency / 100.0f);

        // Turn on LEDs with non-zero brightness
        for (int i = 0; i < 3; i++)
        {
            if (on_times[i] > 0)
            {
                GPIO_PinWrite(g_rgb_leds[i].red_gpio, g_rgb_leds[i].red_pin, 1);
                GPIO_PinWrite(g_rgb_leds[i].green_gpio, g_rgb_leds[i].green_pin, 1);
                GPIO_PinWrite(g_rgb_leds[i].blue_gpio, g_rgb_leds[i].blue_pin, 1);
            }
            else
            {
                // Ensure the LEDs are turned off
                GPIO_PinWrite(g_rgb_leds[i].red_gpio, g_rgb_leds[i].red_pin, 0);
                GPIO_PinWrite(g_rgb_leds[i].green_gpio, g_rgb_leds[i].green_pin, 0);
                GPIO_PinWrite(g_rgb_leds[i].blue_gpio, g_rgb_leds[i].blue_pin, 0);
            }
        }

        // Handle the PWM duty cycles
        TickType_t elapsed_time = 0;

        while (elapsed_time < xFrequency)
        {
            TickType_t next_event = xFrequency - elapsed_time;

            // Find the next time when any LED should be turned off
            for (int i = 0; i < 3; i++)
            {
                if (on_times[i] > elapsed_time && (on_times[i] - elapsed_time) < next_event)
                {
                    next_event = on_times[i] - elapsed_time;
                }
            }

            if (next_event > 0)
            {
                // Delay until next event
                vTaskDelay(next_event);
                elapsed_time += next_event;

                // Turn off LEDs whose on_time has elapsed
                for (int i = 0; i < 3; i++)
                {
                    if (on_times[i] == elapsed_time)
                    {
                        GPIO_PinWrite(g_rgb_leds[i].red_gpio, g_rgb_leds[i].red_pin, 0);
                        GPIO_PinWrite(g_rgb_leds[i].green_gpio, g_rgb_leds[i].green_pin, 0);
                        GPIO_PinWrite(g_rgb_leds[i].blue_gpio, g_rgb_leds[i].blue_pin, 0);
                    }
                }
            }
            else
            {
                // No more events to process; break out of the loop
                break;
            }
        }

        // Wait until the end of the period
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}



// This task blink alternatively both PTAx LEDs
void task_led_pta_blink( void *t_arg )
{
    uint32_t l_inx = 0;

    while ( 1 )
    {
        // switch LED on
        GPIO_PinWrite( g_led_pta[ l_inx ].m_led_gpio, g_led_pta[ l_inx ].m_led_pin, 1 );
        vTaskDelay( 200 );
        // switch LED off
        GPIO_PinWrite( g_led_pta[ l_inx ].m_led_gpio, g_led_pta[ l_inx ].m_led_pin, 0 );
        // next LED
        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

// This task is snake animation from left side on red LEDs
void task_snake_left( void *t_arg )
{
    while ( 1 )
    {
        vTaskSuspend( 0 );

        for ( int inx = 0; inx < LED_PTC_NUM; inx++ )
        {
            // switch LED on
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 1 );
            vTaskDelay( 200 );
            // switch LED off
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 0 );
        }
    }
}

// This task is snake animation from right side on red LEDs
void task_snake_right( void *t_arg )
{
    while ( 1 )
    {
        vTaskSuspend( 0 );

        for ( int inx = LED_PTC_NUM - 1; inx >= 0; inx-- )
        {
            // switch LED on
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 1 );
            vTaskDelay( 200 );
            // switch LED off
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 0 );
        }
    }
}

void light_all (void *t_arg){
    while (1)
    {
        // Turn on all LEDs
        for ( int inx = 0; inx < LED_PTC_NUM; inx++ )
        {
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 1 );
        }

        vTaskSuspend(NULL);
    }
}

void light_all_off (void *t_arg){
    while (1)
    {
        for ( int inx = 0; inx < LED_PTC_NUM; inx++ )
        {
            GPIO_PinWrite( g_led_ptc[ inx ].m_led_gpio, g_led_ptc[ inx ].m_led_pin, 0 );
        }
        vTaskSuspend(NULL);
    }
}

void increase_brightness(volatile float* brightness)
{
    if (*brightness <= 90.0f)
    {
        *brightness += 10.0f;
    }
    else
    {
        *brightness = 100.0f;
    }
}

void decrease_brightness(volatile float* brightness)
{
    if (*brightness >= 10.0f)
    {
        *brightness -= 10.0f;
    }
    else
    {
        *brightness = 0.0f;
    }
}

void task_switches(void *t_arg)
{
    // Variables for PTC11 double press detection
    static TickType_t last_press_time_ptc11 = 0;
    static uint8_t ptc11_press_count = 0;
    const TickType_t DOUBLE_PRESS_TIMEOUT = pdMS_TO_TICKS(500); // 500 ms timeout
    static bool prev_ptc11_state = true; // Assuming button is not pressed initially
    TaskHandle_t l_handle_led_snake_l = xTaskGetHandle( TASK_NAME_LED_SNAKE_L );
    TaskHandle_t l_handle_led_snake_r = xTaskGetHandle( TASK_NAME_LED_SNAKE_R );

    // Variables to store previous button states
    bool prev_ptc9_state = true;
    bool prev_ptc10_state = true;
    bool prev_ptc12_state = true;

    while (1)
    {
        // Read current state of switches
        bool current_ptc9_state = (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0) ? false : true;
        bool current_ptc10_state = (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0) ? false : true;
        bool current_ptc11_state = (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0) ? false : true;
        bool current_ptc12_state = (GPIO_PinRead(SW_PTC12_GPIO, SW_PTC12_PIN) == 0) ? false : true;

        // Handle PTC9 button press
        if (current_ptc9_state == false && prev_ptc9_state == true)
        {
            // Increase brightness of rgb0 by 10%, decrease brightness of rgb2 by 10%
            increase_brightness(&brightness_rgb0);
            decrease_brightness(&brightness_rgb2);
            PRINTF("PTC9 Pressed: Brightness rgb0: %.0f%%, rgb2: %.0f%%\r\n", brightness_rgb0, brightness_rgb2);
        }
        prev_ptc9_state = current_ptc9_state;

        // Handle PTC12 button press
        if (current_ptc12_state == false && prev_ptc12_state == true)
        {
            // Decrease brightness of rgb0 by 10%, increase brightness of rgb2 by 10%
            decrease_brightness(&brightness_rgb0);
            increase_brightness(&brightness_rgb2);
            PRINTF("PTC12 Pressed: Brightness rgb0: %.0f%%, rgb2: %.0f%%\r\n", brightness_rgb0, brightness_rgb2);
            if ( l_handle_led_snake_l )
                vTaskResume( l_handle_led_snake_l );
        }
        prev_ptc12_state = current_ptc12_state;

        // Handle PTC10 button press
        if (current_ptc10_state == false && prev_ptc10_state == true)
        {
            // Increase brightness of rgb1 by 10%
            increase_brightness(&brightness_rgb1);
            PRINTF("PTC10 Pressed: Brightness rgb1: %.0f%%\r\n", brightness_rgb1);
        }
        prev_ptc10_state = current_ptc10_state;

        // Handle PTC11 button press (brightness decrease and double press detection)
        if (current_ptc11_state == false && prev_ptc11_state == true)
        {
            TickType_t current_time = xTaskGetTickCount();

            // Decrease brightness of rgb1 by 10%
            decrease_brightness(&brightness_rgb1);
            PRINTF("PTC11 Pressed: Brightness rgb1: %.0f%%\r\n", brightness_rgb1);
            if ( l_handle_led_snake_r )
                vTaskResume( l_handle_led_snake_r );

            if ((current_time - last_press_time_ptc11) <= DOUBLE_PRESS_TIMEOUT)
            {
                ptc11_press_count++;

                if (ptc11_press_count == 2)
                {
                    // Double press detected, toggle LEDs from 0 to 7
                    static bool leds_on = false;
                    leds_on = !leds_on;

                    for (int inx = 0; inx < LED_PTC_NUM; inx++)
                    {
                        GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, leds_on ? 1 : 0);
                    }

                    PRINTF("PTC11 Double Pressed: LEDs from 0 to 7 turned %s.\r\n", leds_on ? "ON" : "OFF");

                    // Reset press count after toggling
                    ptc11_press_count = 0;
                }
            }
            else
            {
                ptc11_press_count = 1;
            }

            last_press_time_ptc11 = current_time;
        }
        prev_ptc11_state = current_ptc11_state;

        vTaskDelay(50); // Delay to debounce and reduce CPU usage
    }
}

// Start application
int main(void) {

    // Init board hardware.
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF( "FreeRTOS task demo program.\r\n" );
    PRINTF( "Switches PTC9 and PTC10 will stop and run PTAx LEDs blinking.\r\n" );
    PRINTF( "Switches PTC11 will toggle all lights on/off with double press.\r\n" );
    PRINTF( "Switches PTC12 will start snake on red LEDs from the right side.\r\n");

    // Create tasks
//    if ( xTaskCreate(
//            task_led_pta_blink,
//            TASK_NAME_LED_PTA,
//            configMINIMAL_STACK_SIZE + 100,
//            NULL,
//            NORMAL_TASK_PRIORITY,
//            NULL ) != pdPASS )
//    {
//        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_PTA );
//    }

    if ( xTaskCreate( light_all, TASK_NAME_LIGHT, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LIGHT );
    }
    else
    {
        vTaskSuspend(xTaskGetHandle(TASK_NAME_LIGHT)); // Initially suspend
    }

    if ( xTaskCreate( light_all_off, TASK_NAME_LIGHT_OFF, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LIGHT_OFF );
    }
    else
    {
        vTaskSuspend(xTaskGetHandle(TASK_NAME_LIGHT_OFF)); // Initially suspend
    }

    if ( xTaskCreate( task_snake_right, TASK_NAME_LED_SNAKE_R, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS)
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R );
    }

    if ( xTaskCreate( task_snake_left, TASK_NAME_LED_SNAKE_L, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS)
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R );
    }

    if ( xTaskCreate( task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES );
    }

    if (xTaskCreate(
            task_rgb_control,
            "RGB_Control",
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS)
    {
        PRINTF("Unable to create task 'RGB_Control'!\r\n");
    }


    vTaskStartScheduler();

    while ( 1 );

    return 0 ;
}