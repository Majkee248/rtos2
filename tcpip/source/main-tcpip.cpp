// **************************************************************************
//
//               Demo program for OSY labs
//
// Subject:      Operating systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 12/2023
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         FreeRTOS TCP/IP Demo with Button Handling
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
// PTB2		LED_PTB2_PIN   LED_PTB2_GPIO
// PTB3		LED_PTB3_PIN   LED_PTB3_GPIO
// PTB9		LED_PTB9_PIN   LED_PTB9_GPIO
// PTB10	LED_PTB10_PIN  LED_PTB10_GPIO
// PTB11	LED_PTB11_PIN  LED_PTB11_GPIO
// PTB18	LED_PTB18_PIN  LED_PTB18_GPIO
// PTB19	LED_PTB19_PIN  LED_PTB19_GPIO
// PTB20	LED_PTB20_PIN  LED_PTB20_GPIO
// PTB23	LED_PTB23_PIN  LED_PTB23_GPIO

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_sysmpu.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// Task priorities.
#define LOW_TASK_PRIORITY 		(configMAX_PRIORITIES - 2)
#define NORMAL_TASK_PRIORITY 	(configMAX_PRIORITIES - 1)
#define HIGH_TASK_PRIORITY 		(configMAX_PRIORITIES)

// Task names.
#define TASK_NAME_LED_PTA			"led_pta"
#define TASK_NAME_SOCKET_SRV		"socket_srv"
#define TASK_NAME_SOCKET_CLI		"socket_cli"
#define TASK_NAME_SET_ONOFF          "set_onoff"
#define TASK_NAME_BUTTON            "button"

// Socket configuration.
#define SOCKET_SRV_TOUT			1000
#define SOCKET_SRV_BUF_SIZE		256
#define SOCKET_SRV_PORT			3333

#define SOCKET_CLI_PORT			3333

#define BUT_NUM 	    4
#define LED_PTA_NUM 	2
#define LED_PTC_NUM		8
#define LED_PTB_NUM		9

// Pair of GPIO port and LED pin.
struct LED_Data
{
    uint32_t pin;
    GPIO_Type *gpio;
};

// All PTAx LEDs in array.
LED_Data pta[ LED_PTA_NUM ] =
        {
                { LED_PTA1_PIN, LED_PTA1_GPIO },
                { LED_PTA2_PIN, LED_PTA2_GPIO },
        };

// All PTCx LEDs in array.
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

// Custom LED structure to hold state.
struct CUSTOM_LED {
    bool state;
    uint32_t pin;
    GPIO_Type *gpio;
};

// Initialize PTC LEDs with default off state.
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

// Custom Button structure to hold state.
struct CUSTOM_BUT {
    bool state;
    bool change;
    bool released;
    unsigned int pin;
    GPIO_Type *gpio;
};

// Initialize buttons.
CUSTOM_BUT but_bool[BUT_NUM] = {
        {false, false, false, SW_PTC9_PIN, SW_PTC9_GPIO},
        {false, false, false, SW_PTC10_PIN, SW_PTC10_GPIO},
        {false, false, false, SW_PTC11_PIN, SW_PTC11_GPIO},
        {false, false, false, SW_PTC12_PIN, SW_PTC12_GPIO}
};

// Global socket client variable.
xSocket_t l_sock_client = FREERTOS_INVALID_SOCKET;

// Random number generator for TCP/IP stack.
BaseType_t xApplicationGetRandomNumber( uint32_t * tp_pul_number ) { return uxRand(); }

// Stack overflow hook.
void vApplicationStackOverflowHook( xTaskHandle *tp_task_handle, signed portCHAR *tp_task_name )
{
PRINTF( "STACK PROBLEM %s.\r\n", tp_task_name );
taskDISABLE_INTERRUPTS();
for( ;; );
}

// This task blinks PTA LEDs alternately.
void task_led_pta_blink( void *t_arg )
{
    uint32_t l_inx = 0;

    while ( 1 )
    {
        // Switch LED on.
        GPIO_PinWrite( pta[ l_inx ].gpio, pta[ l_inx ].pin, 1 );
        vTaskDelay( 200 / portTICK_PERIOD_MS );
        // Switch LED off.
        GPIO_PinWrite( pta[ l_inx ].gpio, pta[ l_inx ].pin, 0 );
        // Next LED.
        l_inx++;
        l_inx %= LED_PTA_NUM;
    }
}

// This task sets the state of PTC LEDs based on ptc_bool array.
void task_set_onoff( void *tp_arg  ){
    while(1) {
        for(int i = 0; i < LED_PTC_NUM; i++) {
            GPIO_PinWrite( ptc_bool[ i ].gpio, ptc_bool[ i ].pin, ptc_bool[i].state );
        }
        vTaskDelay( 50 / portTICK_PERIOD_MS );
    }
}

// This task monitors the buttons and updates their state.
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

            // Assuming active low buttons.
            but_bool[i].state = (state == 0);

            if (but_bool[i].state != prev_state) {
                but_bool[i].change = true;
                but_bool[i].released = !but_bool[i].state;
            } else {
                but_bool[i].change = false;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// This task handles button presses for PTC9 and PTC10 to send commands.
void task_button( void *t_arg ) {
    while (1) {
        // Check if PTC9 was pressed.
        if (but_bool[0].change && but_bool[0].state) { // PTC9 corresponds to index 0
            char snakeCmd[] = "SNAKE\n";
            PRINTF("Button PTC9 pressed. Sending SNAKE command.\n");
            if (l_sock_client != FREERTOS_INVALID_SOCKET) {
                BaseType_t l_len = FreeRTOS_send(l_sock_client, (void *)snakeCmd, strlen(snakeCmd), 0);
                if (l_len < 0) {
                    PRINTF("Failed to send SNAKE command.\n");
                }
            } else {
                PRINTF("No client connected. Cannot send SNAKE command.\n");
            }
        }

        // Check if PTC10 was pressed.
        if (but_bool[1].change && but_bool[1].state) { // PTC10 corresponds to index 1
            char bSnakeCmd[] = "B_SNAKE\n";
            PRINTF("Button PTC10 pressed. Sending B_SNAKE command.\n");
            if (l_sock_client != FREERTOS_INVALID_SOCKET) {
                BaseType_t l_len = FreeRTOS_send(l_sock_client, (void *)bSnakeCmd, strlen(bSnakeCmd), 0);
                if (l_len < 0) {
                    PRINTF("Failed to send B_SNAKE command.\n");
                }
            } else {
                PRINTF("No client connected. Cannot send B_SNAKE command.\n");
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Callback from TCP stack - interface state changed.
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t t_network_event )
{
    static BaseType_t s_task_already_created = pdFALSE;
    static struct freertos_sockaddr s_server_addr;

    // Both eNetworkUp and eNetworkDown events can be processed here.
    if ( t_network_event == eNetworkUp )
    {
        PRINTF( "Network interface UP.\r\n" );
        // Create the tasks that use the TCP/IP stack if they have not already been created.
        if ( s_task_already_created == pdFALSE )
        {
            // Create socket server task.
            if ( xTaskCreate( task_socket_srv, TASK_NAME_SOCKET_SRV, configMINIMAL_STACK_SIZE + 1024,
                              ( void * ) SOCKET_SRV_PORT, configMAX_PRIORITIES - 1, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task %s.\r\n", TASK_NAME_SOCKET_SRV );
            }

            // Create button handling tasks.
            if ( xTaskCreate( task_monitor_buttons, "monitor_buttons", configMINIMAL_STACK_SIZE + 200,
                              NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task 'monitor_buttons'.\r\n" );
            }

            if ( xTaskCreate( task_button, TASK_NAME_BUTTON, configMINIMAL_STACK_SIZE + 200,
                              NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task '%s'.\r\n", TASK_NAME_BUTTON );
            }

            // Create LED set task.
            if ( xTaskCreate( task_set_onoff, TASK_NAME_SET_ONOFF, configMINIMAL_STACK_SIZE + 200,
                              NULL, NORMAL_TASK_PRIORITY, NULL ) != pdPASS )
            {
                PRINTF( "Unable to create task '%s'.\r\n", TASK_NAME_SET_ONOFF );
            }

            s_task_already_created = pdTRUE;
        }
    }
}

// Task socket server.
void task_socket_srv( void *tp_arg )
{
    PRINTF( "Task socket server started.\r\n" );
    TickType_t l_receive_tout = 25000 / portTICK_PERIOD_MS;

    int l_port = ( int ) tp_arg;
    struct freertos_sockaddr l_srv_address;

    // Set listening port.
    l_srv_address.sin_port = FreeRTOS_htons( l_port );
    l_srv_address.sin_addr = FreeRTOS_inet_addr_quick( 0, 0, 0, 0 );

    xSocket_t l_sock_listen;
    struct freertos_sockaddr from;
    socklen_t fromSize = sizeof from;
    BaseType_t l_bind_result;
    int8_t l_rx_buf[ SOCKET_SRV_BUF_SIZE + 1 ]; // +1 for null terminator.

    /* Create a socket. */
    l_sock_listen = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( l_sock_listen != FREERTOS_INVALID_SOCKET );

    l_bind_result = FreeRTOS_bind( l_sock_listen, &l_srv_address, sizeof l_srv_address );
    configASSERT( l_bind_result == 0 );

    // Set receive and send timeouts.
    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_RCVTIMEO, &l_receive_tout, sizeof( l_receive_tout ) );
    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_SNDTIMEO, &l_receive_tout, sizeof( l_receive_tout ) );

    // Set window properties.
    xWinProperties_t l_win_props;
    memset( &l_win_props, '\0', sizeof l_win_props );
    l_win_props.lTxBufSize   = SOCKET_SRV_BUF_SIZE;
    l_win_props.lTxWinSize   = 2;
    l_win_props.lRxBufSize   = SOCKET_SRV_BUF_SIZE;
    l_win_props.lRxWinSize   = 2;

    FreeRTOS_setsockopt( l_sock_listen, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &l_win_props, sizeof( l_win_props ) );

    // Start listening.
    FreeRTOS_listen( l_sock_listen, 2 );

    PRINTF( "Socket server listening on port %u.\r\n", FreeRTOS_ntohs( l_srv_address.sin_port ) );

    // Server loop.
    for( ;; )
    {
        uint32_t l_reply_count = 0;
        // Wait for client.
        do {
            l_sock_client = FreeRTOS_accept( l_sock_listen, &from, &fromSize );
            vTaskDelay( SOCKET_SRV_TOUT / portTICK_PERIOD_MS );
            PRINTF("Waiting for client...\n");
        } while ( l_sock_client == FREERTOS_INVALID_SOCKET );

        if ( l_sock_client == FREERTOS_INVALID_SOCKET )
        {
            PRINTF( "Invalid socket from ACCEPT!\r\n" );
            continue;
        }

        PRINTF( "Client connected.\r\n" );

        // Handle echo requests.
        for ( l_reply_count = 0; pdTRUE; l_reply_count++ )
        {
            BaseType_t l_len;

            // Receive data.
            l_len = FreeRTOS_recv(	l_sock_client, l_rx_buf, SOCKET_SRV_BUF_SIZE, 0 );

            if( l_len > 0 )
            {
                l_rx_buf[ l_len ] = 0;	// Null-terminate for safe printing.
                PRINTF("Received: %s", l_rx_buf);

                // Example: Echo the received message back.
                l_len = FreeRTOS_send( l_sock_client, ( void * ) l_rx_buf, l_len, 0 );

                PRINTF( "Server forwarded %d bytes.\r\n", l_len );
            }
            else if ( l_len < 0 )
            {
                PRINTF( "FreeRTOS_recv returned error: %ld.\r\n", l_len );
                break;
            }
            else // l_len == 0
            {
                PRINTF( "Recv timeout or connection closed.\r\n" );
                break;
            }
        }

        PRINTF( "Socket server session ended after %d replies.\r\n", l_reply_count );

        // Pause before accepting next client.
        vTaskDelay( SOCKET_SRV_TOUT / portTICK_PERIOD_MS );

        // Close this socket before looping back to create another.
        FreeRTOS_closesocket( l_sock_client );
        l_sock_client = FREERTOS_INVALID_SOCKET;
    }
}

// Application entry point.
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();

    /* Disable MPU for simplicity. */
    SYSMPU_Enable( SYSMPU, false );

    PRINTF( "FreeRTOS+TCP with Button Handling started.\r\n" );

    // SET CORRECTLY MAC ADDRESS FOR USAGE IN LAB!
    //
    // Computer in lab use IP address 158.196.XXX.YYY.
    // Set MAC to 5A FE C0 DE XXX YYY+21
    // IP address will be configured from DHCP
    //
    uint8_t ucMAC[ ipMAC_ADDRESS_LENGTH_BYTES ] = { 0x5A, 0xFE, 0xC0, 0xDE, 0x8E, 0x69 };

    uint8_t ucIPAddress[ ipIP_ADDRESS_LENGTH_BYTES ] = { 10, 0, 0, 10 };
    uint8_t ucIPMask[ ipIP_ADDRESS_LENGTH_BYTES ] = { 255, 255, 255, 0 };
    uint8_t ucIPGW[ ipIP_ADDRESS_LENGTH_BYTES ] = { 10, 0, 0, 1 };

    FreeRTOS_IPInit( ucIPAddress,  ucIPMask,  ucIPGW,  NULL,  ucMAC );

    // Create PTA LED blink task.
    if ( xTaskCreate(
            task_led_pta_blink,
            TASK_NAME_LED_PTA,
            configMINIMAL_STACK_SIZE + 100,
            NULL,
            NORMAL_TASK_PRIORITY,
            NULL ) != pdPASS )
    {
        PRINTF( "Unable to create task '%s'.\r\n", TASK_NAME_LED_PTA );
    }

    // Start the scheduler.
    vTaskStartScheduler();

    // Should never reach here.
    while ( 1 );

    return 0 ;
}
