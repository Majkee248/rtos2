// **************************************************************************
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 02/2022
// Modified by:  [Your Name], [Your Email], [Date]
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         Task control demo program with RGB brightness control.
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


#define LOW_TASK_PRIORITY 		(configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY 	(configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY 		(configMAX_PRIORITIES)


#define TASK_NAME_SWITCHES			"switches"
#define TASK_NAME_LED_PTA			"led_pta"
#define TASK_NAME_LED_SNAKE_L		"led_snake_l"
#define TASK_NAME_LED_SNAKE_R		"led_snake_r"
#define TASK_NAME_LED_BOTH_SNAKES	"led_both_snakes"
#define TASK_NAME_ALL_ON			    "all_on"
#define TASK_NAME_ALL_OFF			"all_off"
#define TASK_NAME_RGB_BRIGHTNESS	"rgb_brightness"


#define LED_PTA_NUM 	2
#define LED_PTC_NUM		8
#define LED_PTB_NUM		9


#define BRIGHTNESS_STEP 1
#define BRIGHTNESS_MAX 10
#define BRIGHTNESS_MIN 0


#define DOUBLE_CLICK_TIMEOUT_MS   300
#define DOUBLE_CLICK_TIMEOUT_TICKS pdMS_TO_TICKS(DOUBLE_CLICK_TIMEOUT_MS)

#define DEBOUNCE_DELAY_MS 50


bool ptc9_prev_state = true;
bool ptc10_prev_state = true;
bool ptc12_prev_state = true;

TaskHandle_t l_handle_led_all_on = NULL;
TaskHandle_t l_handle_led_all_off = NULL;


volatile uint8_t brightness1 = 5;
volatile uint8_t brightness2 = 5;

uint32_t site = 0;

struct LED_Data
{
    uint32_t m_led_pin;
    GPIO_Type *m_led_gpio;
};

LED_Data g_led_pta[ LED_PTA_NUM ] =
        {
                { LED_PTA1_PIN, LED_PTA1_GPIO },
                { LED_PTA2_PIN, LED_PTA2_GPIO },
        };

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

void task_led_pta_blink( void *t_arg )
{
    uint32_t l_inx = 0;

    while ( 1 )
    {
        GPIO_PinWrite( g_led_pta[ l_inx ].m_led_gpio, g_led_pta[ l_inx ].m_led_pin, 1 );
        vTaskDelay( pdMS_TO_TICKS(200) );

        GPIO_PinWrite( g_led_pta[ l_inx ].m_led_gpio, g_led_pta[ l_inx ].m_led_pin, 0 );

        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

void task_all_on(void *t_arg){
    while (1)
    {
        for (int inx = 0; inx < LED_PTC_NUM; inx++)
        {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 1);
        }

        vTaskSuspend(NULL);
    }
}

void task_all_off(void *t_arg){
    while (1)
    {
        for (int inx = 0; inx < LED_PTC_NUM; inx++)
        {
            GPIO_PinWrite(g_led_ptc[inx].m_led_gpio, g_led_ptc[inx].m_led_pin, 0);
        }

        vTaskSuspend(NULL);
    }
}

void task_snake_left(void *t_arg) {
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
}

void task_snake_right(void *t_arg) {
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
}

void task_both_snakes(void *t_arg){
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
}

void task_switches(void *t_arg) {
    TaskHandle_t l_handle_led_snake_l = xTaskGetHandle(TASK_NAME_LED_SNAKE_L);
    TaskHandle_t l_handle_led_snake_r = xTaskGetHandle(TASK_NAME_LED_SNAKE_R);
    TaskHandle_t l_handle_led_all_on = xTaskGetHandle(TASK_NAME_ALL_ON);
    TaskHandle_t l_handle_led_all_off = xTaskGetHandle(TASK_NAME_ALL_OFF);
    TaskHandle_t l_handle_led_snake_back = xTaskGetHandle(TASK_NAME_LED_BOTH_SNAKES)
    TickType_t last_ptc9_click_time = 0;
    uint8_t ptc9_click_count = 0;
    TickType_t last_ptc10_click_time = 0;
    uint8_t ptc10_click_count = 0;


    while (1) {
        if (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0) {
            if (l_handle_led_snake_l) {
                vTaskResume(l_handle_led_snake_l);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        if (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0) {
            if (l_handle_led_snake_r) {
                vTaskResume(l_handle_led_snake_r);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        if (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0) {
            if (l_handle_led_snake_back) {
                vTaskResume(l_handle_led_snake_back);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*void task_switches(void *t_arg)
{
    TaskHandle_t l_handle_led_snake_l = xTaskGetHandle(TASK_NAME_LED_SNAKE_L);
    TaskHandle_t l_handle_led_snake_r = xTaskGetHandle(TASK_NAME_LED_SNAKE_R);
    TaskHandle_t l_handle_led_both_snakes = xTaskGetHandle(TASK_NAME_LED_BOTH_SNAKES);
    TaskHandle_t l_handle_led_all_on = xTaskGetHandle(TASK_NAME_ALL_ON);
    TaskHandle_t l_handle_led_all_off = xTaskGetHandle(TASK_NAME_ALL_OFF);

    TickType_t last_ptc9_click_time = 0;
    uint8_t ptc9_click_count = 0;
    TickType_t last_ptc10_click_time = 0;
    uint8_t ptc10_click_count = 0;

    while (1)
    {
        bool ptc9_current_state = (GPIO_PinRead(SW_PTC9_GPIO, SW_PTC9_PIN) == 0);
        if (ptc9_current_state && !ptc9_prev_state)
        {
            TickType_t current_time = xTaskGetTickCount();
            if (ptc9_click_count == 0)
            {
                ptc9_click_count = 1;
                last_ptc9_click_time = current_time;
            }
            else if (ptc9_click_count == 1)
            {
                if ((current_time - last_ptc9_click_time) <= DOUBLE_CLICK_TIMEOUT_TICKS)
                {
                    ptc9_click_count = 0;
                    if (l_handle_led_all_on)
                    {
                        vTaskResume(l_handle_led_all_on);
                    }
                }
                else
                {
                    ptc9_click_count = 1;
                    last_ptc9_click_time = current_time;
                }
            }
        }
        ptc9_prev_state = ptc9_current_state;

        bool ptc10_current_state = (GPIO_PinRead(SW_PTC10_GPIO, SW_PTC10_PIN) == 0);
        if (ptc10_current_state && !ptc10_prev_state)
        {
            TickType_t current_time = xTaskGetTickCount();
            if (ptc10_click_count == 0)
            {
                ptc10_click_count = 1;
                last_ptc10_click_time = current_time;
            }
            else if (ptc10_click_count == 1)
            {
                if ((current_time - last_ptc10_click_time) <= DOUBLE_CLICK_TIMEOUT_TICKS)
                {
                    ptc10_click_count = 0;
                    if (l_handle_led_all_off)
                    {
                        vTaskResume(l_handle_led_all_off);
                    }
                }
                else
                {
                    ptc10_click_count = 1;
                    last_ptc10_click_time = current_time;
                }
            }
        }
        ptc10_prev_state = ptc10_current_state;

        bool ptc11_current_state = (GPIO_PinRead(SW_PTC11_GPIO, SW_PTC11_PIN) == 0);
        if (ptc11_current_state && !ptc11_prev_state)
        {
            if (l_handle_led_snake_l)
            {
                vTaskResume(l_handle_led_snake_l);
            }
        }
        ptc11_prev_state = ptc11_current_state;

        bool ptc12_current_state = (GPIO_PinRead(SW_PTC12_GPIO, SW_PTC12_PIN) == 0);
        if (ptc12_current_state && !ptc12_prev_state)
        {
            if (l_handle_led_snake_r)
            {
                vTaskResume(l_handle_led_snake_r);
            }
        }
        ptc12_prev_state = ptc12_current_state;

        if (ptc11_current_state && ptc12_current_state)
        {
            if (l_handle_led_both_snakes)
            {
                vTaskResume(l_handle_led_both_snakes);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}*/


void task_rgb_brightness_control(void *t_arg)
{
    uint8_t counter = 0;

    while(1)
    {

        counter++;
        if(counter >= BRIGHTNESS_MAX)
        {
            counter = 0;
        }


        if(counter < brightness1)
        {
            GPIO_PinWrite(LED_PTB2_GPIO, LED_PTB2_PIN, 1);
            GPIO_PinWrite(LED_PTB3_GPIO, LED_PTB3_PIN, 1);
            GPIO_PinWrite(LED_PTB9_GPIO, LED_PTB9_PIN, 1);
        }
        else
        {
            GPIO_PinWrite(LED_PTB2_GPIO, LED_PTB2_PIN, 0);
            GPIO_PinWrite(LED_PTB3_GPIO, LED_PTB3_PIN, 0);
            GPIO_PinWrite(LED_PTB9_GPIO, LED_PTB9_PIN, 0);
        }


        if(counter < brightness2)
        {
            GPIO_PinWrite(LED_PTB19_GPIO, LED_PTB19_PIN, 1);
            GPIO_PinWrite(LED_PTB20_GPIO, LED_PTB20_PIN, 1);
            GPIO_PinWrite(LED_PTB23_GPIO, LED_PTB23_PIN, 1);
        }
        else
        {
            GPIO_PinWrite(LED_PTB10_GPIO, LED_PTB10_PIN, 0);
            GPIO_PinWrite(LED_PTB11_GPIO, LED_PTB11_PIN, 0);
            GPIO_PinWrite(LED_PTB18_GPIO, LED_PTB18_PIN, 0);
        }


        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int main(void) {


    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF( "FreeRTOS task demo program with RGB brightness control.\r\n" );
    PRINTF( "On startup, RGB0 is at 50%% brightness and RGB1 is at 50%% brightness.\r\n" );
    PRINTF( "Pressing PTC9 decreases RGB0 brightness by 10%% and increases RGB1 by 10%%.\r\n" );
    PRINTF( "Pressing PTC12 decreases RGB1 brightness by 10%% and increases RGB0 by 10%%.\r\n" );
    PRINTF( "Double-clicking PTC9 turns all LEDs on.\r\n" );
    PRINTF( "Double-clicking PTC10 turns all LEDs off.\r\n" );
    PRINTF( "Other buttons control LED animations as before.\r\n" );


    brightness1 = 5;
    brightness2 = 5;

    if ( xTaskCreate(
            task_led_pta_blink,
            TASK_NAME_LED_PTA,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_PTA );
    }

    if ( xTaskCreate( task_snake_left, TASK_NAME_LED_SNAKE_L, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_L );
    }

    if ( xTaskCreate( task_snake_right, TASK_NAME_LED_SNAKE_R, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS)
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_SNAKE_R );
    }

    if ( xTaskCreate( task_switches, TASK_NAME_SWITCHES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_SWITCHES );
    }

    if ( xTaskCreate( task_both_snakes, TASK_NAME_LED_BOTH_SNAKES, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_LED_BOTH_SNAKES );
    }

    if ( xTaskCreate( task_all_on, TASK_NAME_ALL_ON, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_ALL_ON );
    }

    if ( xTaskCreate( task_all_off, TASK_NAME_ALL_OFF, configMINIMAL_STACK_SIZE + 100, NULL, NORMAL_TASK_PRIORITY, NULL) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_ALL_OFF );
    }


    if ( xTaskCreate(
            task_rgb_brightness_control,
            TASK_NAME_RGB_BRIGHTNESS,
            configMINIMAL_STACK_SIZE + 200,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'!\r\n", TASK_NAME_RGB_BRIGHTNESS );
    }

    vTaskStartScheduler();


    while ( 1 );

    return 0 ;
}
