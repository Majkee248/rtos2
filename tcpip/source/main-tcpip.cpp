// **************************************************************************
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 12/2023
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         FreeRTOS TCP/IP Demo with Left/Right LED Control Feature
//
// **************************************************************************
//
// All mapped LEDs and switches and their PINs and GPIOs:
// See schema in APPS syllabus.
//
// Switches:
// Name        PIN                GPIO
// PTC9        SW_PTC9_PIN        SW_PTC9_GPIO
// PTC10    SW_PTC10_PIN    SW_PTC10_GPIO
// PTC11    SW_PTC11_PIN    SW_PTC11_GPIO
// PTC12    SW_PTC12_PIN    SW_PTC12_GPIO
//
// LEDs:
// Name        PIN                GPIO
// PTA1        LED_PTA1_PIN   LED_PTA1_GPIO
// PTA2        LED_PTA2_PIN   LED_PTA2_GPIO
//
// PTC0        LED_PTC0_PIN   LED_PTC0_GPIO
// PTC1        LED_PTC1_PIN   LED_PTC1_GPIO
// PTC2        LED_PTC2_PIN   LED_PTC2_GPIO
// PTC3        LED_PTC3_PIN   LED_PTC3_GPIO
// PTC4        LED_PTC4_PIN   LED_PTC4_GPIO
// PTC5        LED_PTC5_PIN   LED_PTC5_GPIO
// PTC7        LED_PTC7_PIN   LED_PTC7_GPIO
// PTC8        LED_PTC8_PIN   LED_PTC8_GPIO
//
// PTB2        LED_PTB2_PIN   LED_PTB2_GPIO
// PTB3        LED_PTB3_PIN   LED_PTB3_GPIO
// PTB9        LED_PTB9_PIN   LED_PTB9_GPIO
// PTB10    LED_PTB10_PIN  LED_PTB10_GPIO
// PTB11    LED_PTB11_PIN  LED_PTB11_GPIO
// PTB18    LED_PTB18_PIN  LED_PTB18_GPIO
// PTB19    LED_PTB19_PIN  LED_PTB19_GPIO
// PTB20    LED_PTB20_PIN  LED_PTB20_GPIO
// PTB23    LED_PTB23_PIN  LED_PTB23_GPIO

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_sysmpu.h"
#include <cstdio>
#include <cstring> // Added for string manipulation

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// Task priorities.
#define LOW_TASK_PRIORITY         (configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY     (configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY         (configMAX_PRIORITIES)

// Task names.
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
#define LED_PTC_NUM        8
#define LED_PTB_NUM        9

struct LED_Data
{
    uint32_t pin;
    GPIO_Type *gpio;
};

LED_Data pta[ LED_PTA_NUM ] =
        {
                { LED_PTA1_PIN, LED_PTA1_GPIO },
                { LED_PTA2_PIN, LED_PTA2_GPIO },
        };

LED_Data ptc[ LED_PTC_NUM ] =
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

struct CUSTOM_LED {
    bool state;
    uint32_t pin;
    GPIO_Type *gpio;
};

CUSTOM_LED ptc_bool[ LED_PTC_NUM ] =
        {
                { false, LED_PTC0_PIN, LED_PTC0_GPIO },
                { false, LED_PTC1_PIN, LED_PTC1_GPIO },
                { false, LED_PTC2_PIN, LED_PTC2_GPIO },
                { false, LED_PTC3_PIN, LED_PTC3_GPIO },
                { false, LED_PTC4_PIN, LED_PTC4_GPIO },
                { false, LED_PTC5_PIN, LED_PTC5_GPIO },
                { false, LED_PTC7_PIN, LED_PTC7_GPIO },
                { false, LED_PTC8_PIN, LED_PTC8_GPIO },
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

void task_led_pta_blink( void *t_arg );
void task_socket_srv( void *tp_arg );
void task_socket_cli( void *tp_arg );
void task_set_onoff( void *tp_arg );
void task_monitor_buttons(void *tp_arg);
void task_print_buttons(void *tp_arg);

BaseType_t xApplicationGetRandomNumber( uint32_t * tp_pul_number ) { return uxRand(); }

void vApplicationStackOverflowHook( xTaskHandle *tp_task_handle, signed portCHAR *tp_task_name )
{
PRINTF( "STACK PROBLEM %s.\r\n", tp_task_name );
}

void task_led_pta_blink( void *t_arg )
{
    uint32_t l_inx = 0;

    while ( 1 )
    {
        GPIO_PinWrite( pta[ l_inx ].gpio, pta[ l_inx ].pin, 1 );

        vTaskDelay( 200 / portTICK_PERIOD_MS );

        GPIO_PinWrite( pta[ l_inx ].gpio, pta[ l_inx ].pin, 0 );

        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

bool parse_led_command(const char* input, Direction_t* dir, int* num) {

    char led_str[] = "LED";
    int i = 0;


    while (input[i] == ' ') i++;


    int j = 0;
    while (led_str[j] != '\0') {
        if (input[i] != led_str[j]) {
            return false;
        }
        i++;
        j++;
    }


    while (input[i] == ' ') i++;


    if (input[i] == 'L' || input[i] == 'l') {
        *dir = LEFT;
    } else if (input[i] == 'R' || input[i] == 'r') {
        *dir = RIGHT;
    } else {
        return false;
    }
    i++;

    while (input[i] == ' ') i++;

    if (input[i] >= '0' && input[i] <= '9') {
        *num = 0;
        while (input[i] >= '0' && input[i] <= '9') {
            *num = (*num) * 10 + (input[i] - '0');
            i++;
        }
    } else {
        return false;
    }

    return true;
}

void task_socket_srv( void *tp_arg )
{
    PRINTF( "Task socket server started.\r\n" );
    TickType_t l_receive_tout = 25000 / portTICK_PERIOD_MS;

    int l_port = ( int ) tp_arg;
    struct freertos_sockaddr l_srv_address;

    l_srv_address.sin_port = FreeRTOS_htons( l_port );
    l_srv_address.sin_addr = FreeRTOS_inet_addr_quick( 0, 0, 0, 0 );

    xSocket_t l_sock_listen;
    xWinProperties_t l_win_props;
    struct freertos_sockaddr from;
    socklen_t fromSize = sizeof from;
    BaseType_t l_bind_result;
    int8_t l_rx_buf[ SOCKET_SRV_BUF_SIZE + 1 ];

    l_sock_listen = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( l_sock_listen != FREERTOS_INVALID_SOCKET );

    l_bind_result = FreeRTOS_bind( l_sock_listen, &l_srv_address, sizeof l_srv_address );
    configASSERT( l_bind_result == 0 );

    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_RCVTIMEO, &l_receive_tout, sizeof( l_receive_tout ) );
    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_SNDTIMEO, &l_receive_tout, sizeof( l_receive_tout ) );

    memset( &l_win_props, '\0', sizeof l_win_props );

    l_win_props.lTxBufSize   = SOCKET_SRV_BUF_SIZE;
    l_win_props.lTxWinSize   = 2;
    l_win_props.lRxBufSize   = SOCKET_SRV_BUF_SIZE;
    l_win_props.lRxWinSize   = 2;

    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &l_win_props, sizeof( l_win_props ) );

    FreeRTOS_listen( l_sock_listen, 2 );

    PRINTF( "Socket server started, listening on port %u.\r\n", FreeRTOS_ntohs( l_srv_address.sin_port ) );

    for( ;; )
    {
        uint32_t l_reply_count = 0;

        do {
            l_sock_client = FreeRTOS_accept( l_sock_listen, &from, &fromSize );
            vTaskDelay( SOCKET_SRV_TOUT / portTICK_PERIOD_MS );
        } while ( l_sock_client == FREERTOS_INVALID_SOCKET );

        if ( l_sock_client == FREERTOS_INVALID_SOCKET )
        {
            PRINTF( "Invalid socket from ACCEPT!\r\n" );
            continue;
        }

        vTaskDelay( 100 / portTICK_PERIOD_MS );

        for ( l_reply_count = 0; pdTRUE; l_reply_count++ )
        {
            BaseType_t l_len;

            l_len = FreeRTOS_recv(    l_sock_client, l_rx_buf, SOCKET_SRV_BUF_SIZE, 0 );

            if( l_len > 0 )
            {
                l_rx_buf[ l_len ] = '\0';

                PRINTF( "Received: %s\r\n", l_rx_buf );

                Direction_t direction;
                int num_leds;

                if (parse_led_command((char *)l_rx_buf, &direction, &num_leds)) {
                    PRINTF("Parsed command: Direction=%s, Number=%d\r\n", direction == LEFT ? "LEFT" : "RIGHT", num_leds);

                    if (num_leds >= 0 && num_leds <= LED_PTC_NUM) {
                        if (direction == LEFT) {
                            for (int i = 0; i < num_leds; i++) {
                                ptc_bool[i].state = true;
                                PRINTF("LED PTC%d ON\n", i);
                            }
                            for (int i = num_leds; i < LED_PTC_NUM; i++) {
                                ptc_bool[i].state = false;
                            }
                        } else if (direction == RIGHT) {
                            for (int i = 0; i < num_leds; i++) {
                                ptc_bool[LED_PTC_NUM - 1 - i].state = true;
                                PRINTF("LED PTC%d ON\n", LED_PTC_NUM - 1 - i);
                            }
                            for (int i = 0; i < LED_PTC_NUM - num_leds; i++) {
                                ptc_bool[i].state = false;
                            }
                        }
                    } else {
                        PRINTF("Invalid number of LEDs: %d\n", num_leds);
                    }
                } else {
                    PRINTF("Invalid command format\n");
                }

                l_len = FreeRTOS_send( l_sock_client, ( void * ) l_rx_buf, strlen((char*)l_rx_buf), 0 );

                PRINTF( "Server forwarded %d bytes.\r\n", l_len );
            }
            if ( l_len < 0 )
            {
                // Error occurred
                PRINTF( "FreeRTOS_recv returned error: %ld.\r\n", l_len );
                break;
            }
            if ( l_len == 0 )
            {
                PRINTF( "Recv timeout.\r\n" );
                break;
            }
        }
        PRINTF( "Socket server replied %d times.\r\n", l_reply_count );


        vTaskDelay( SOCKET_SRV_TOUT / portTICK_PERIOD_MS );


        FreeRTOS_closesocket( l_sock_client );
        l_sock_client = FREERTOS_INVALID_SOCKET;
    }
}


void task_socket_cli( void *tp_arg )
{
    PRINTF( "Task socket client started. \r\n" );

    vTaskDelay( 500 / portTICK_PERIOD_MS );

    Socket_t l_sock_server = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    struct freertos_sockaddr *lp_sever_addr = ( freertos_sockaddr * ) tp_arg;
    BaseType_t l_res = FreeRTOS_connect( l_sock_server, lp_sever_addr, sizeof( freertos_sockaddr ) );

    if ( l_res == 0 )
    {
        PRINTF( "Connected to server.\r\n" );

        for ( int i = 0; i < 10; i++ )
        {
            l_res = FreeRTOS_send( l_sock_server, "Hello\n", 6, 0 );
            PRINTF( "Sent %d bytes.\r\n", l_res );
            if ( l_res < 0 ) break;
            vTaskDelay( 500 / portTICK_PERIOD_MS );
        }
    }
    else
    {
        PRINTF( "Unable to connect to server!\r\n" );
    }

    FreeRTOS_closesocket( l_sock_server );

    vTaskDelete( NULL );
}

// Callback from TCP stack - interface state changed
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t t_network_event )
{
    static BaseType_t s_task_already_created = pdFALSE;
    static struct freertos_sockaddr s_server_addr;
    s_server_addr.sin_port = FreeRTOS_htons( SOCKET_CLI_PORT );
    s_server_addr.sin_addr = FreeRTOS_inet_addr_quick( 10, 0, 0, 1 );

    // Handle network up event
    if ( t_network_event == eNetworkUp )
    {
        PRINTF( "Network interface UP.\r\n" );
        // Create the tasks that use the TCP/IP stack if they have not already been created.
        if ( s_task_already_created == pdFALSE )
        {
            // Create socket server task
            if ( xTaskCreate( task_socket_srv, TASK_NAME_SOCKET_SRV, configMINIMAL_STACK_SIZE + 1024,
                              ( void * ) SOCKET_SRV_PORT, configMAX_PRIORITIES - 1, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task %s.\r\n", TASK_NAME_SOCKET_SRV );
            }

            // Optionally, create socket client task
            /*
            if ( xTaskCreate( task_socket_cli, TASK_NAME_SOCKET_CLI, configMINIMAL_STACK_SIZE + 1024,
                    &s_server_addr, configMAX_PRIORITIES - 1, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task %s.\r\n", TASK_NAME_SOCKET_CLI );
            }
            */

            s_task_already_created = pdTRUE;
        }
    }
}

void task_set_onoff( void *tp_arg ){
    while(1) {
        for(int i = 0; i < LED_PTC_NUM; i++) {
            GPIO_PinWrite( ptc_bool[ i ].gpio, ptc_bool[ i ].pin, ptc_bool[i].state );
        }

        vTaskDelay( 5 / portTICK_PERIOD_MS );
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
            if(l_sock_client != FREERTOS_INVALID_SOCKET){
                FreeRTOS_send(l_sock_client, (void *)msg, strlen(msg) + 1, 0);
                PRINTF("Sent Button States: %s", msg);
            }
            enter = false;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();

    SYSMPU_Enable(SYSMPU, false);

    PRINTF("FreeRTOS+TCP with Left/Right LED Control started.\r\n");

    // SET CORRECTLY MAC ADDRESS FOR USAGE IN LAB!
    //
    // Computer in lab use IP address 158.196.XXX.YYY.
    // Set MAC to 5A FE C0 DE XXX YYY+21
    // IP address will be configured from DHCP
    //
    uint8_t ucMAC[ipMAC_ADDRESS_LENGTH_BYTES] = { 0x5A, 0xFE, 0xC0, 0xDE, 142, 100 };

    uint8_t ucIPAddress[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 10 };
    uint8_t ucIPMask[ipIP_ADDRESS_LENGTH_BYTES] = { 255, 255, 255, 0 };
    uint8_t ucIPGW[ipIP_ADDRESS_LENGTH_BYTES] = { 10, 0, 0, 1 };

    FreeRTOS_IPInit(ucIPAddress, ucIPMask, ucIPGW, NULL, ucMAC);

    // Create existing tasks
    if (xTaskCreate(
            task_led_pta_blink,
            TASK_NAME_LED_PTA,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS )
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_LED_PTA );
    }

    if (xTaskCreate(
            task_set_onoff,
            TASK_NAME_SET_ONOFF,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS )
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_SET_ONOFF );
    }

    if (xTaskCreate(
            task_monitor_buttons,
            TASK_NAME_MONITOR_BUTTONS,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS )
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_MONITOR_BUTTONS );
    }

    if (xTaskCreate(
            task_print_buttons,
            TASK_NAME_PRINT_BUTTONS,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL) != pdPASS )
    {
        PRINTF("Unable to create task '%s'.\r\n", TASK_NAME_PRINT_BUTTONS );
    }

    vTaskStartScheduler();

    while (1);

    return 0 ;
}