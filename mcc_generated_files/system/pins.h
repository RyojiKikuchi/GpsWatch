/**
 * Generated Pins header File
 * 
 * @file pins.h
 * 
 * @defgroup  pinsdriver Pins Driver
 * 
 * @brief This is generated driver header for pins. 
 *        This header file provides APIs for all pins selected in the GUI.
 *
 * @version Driver Version  3.0.0
*/

/*
? [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#ifndef PINS_H
#define PINS_H

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set IO_RA0 aliases
#define UART_TX_TRIS                 TRISAbits.TRISA0
#define UART_TX_LAT                  LATAbits.LATA0
#define UART_TX_PORT                 PORTAbits.RA0
#define UART_TX_WPU                  WPUAbits.WPUA0
#define UART_TX_OD                   ODCONAbits.ODCA0
#define UART_TX_ANS                  ANSELAbits.ANSA0
#define UART_TX_SetHigh()            do { LATAbits.LATA0 = 1; } while(0)
#define UART_TX_SetLow()             do { LATAbits.LATA0 = 0; } while(0)
#define UART_TX_Toggle()             do { LATAbits.LATA0 = ~LATAbits.LATA0; } while(0)
#define UART_TX_GetValue()           PORTAbits.RA0
#define UART_TX_SetDigitalInput()    do { TRISAbits.TRISA0 = 1; } while(0)
#define UART_TX_SetDigitalOutput()   do { TRISAbits.TRISA0 = 0; } while(0)
#define UART_TX_SetPullup()          do { WPUAbits.WPUA0 = 1; } while(0)
#define UART_TX_ResetPullup()        do { WPUAbits.WPUA0 = 0; } while(0)
#define UART_TX_SetPushPull()        do { ODCONAbits.ODCA0 = 0; } while(0)
#define UART_TX_SetOpenDrain()       do { ODCONAbits.ODCA0 = 1; } while(0)
#define UART_TX_SetAnalogMode()      do { ANSELAbits.ANSA0 = 1; } while(0)
#define UART_TX_SetDigitalMode()     do { ANSELAbits.ANSA0 = 0; } while(0)
// get/set IO_RA1 aliases
#define UART_RX_TRIS                 TRISAbits.TRISA1
#define UART_RX_LAT                  LATAbits.LATA1
#define UART_RX_PORT                 PORTAbits.RA1
#define UART_RX_WPU                  WPUAbits.WPUA1
#define UART_RX_OD                   ODCONAbits.ODCA1
#define UART_RX_ANS                  ANSELAbits.ANSA1
#define UART_RX_SetHigh()            do { LATAbits.LATA1 = 1; } while(0)
#define UART_RX_SetLow()             do { LATAbits.LATA1 = 0; } while(0)
#define UART_RX_Toggle()             do { LATAbits.LATA1 = ~LATAbits.LATA1; } while(0)
#define UART_RX_GetValue()           PORTAbits.RA1
#define UART_RX_SetDigitalInput()    do { TRISAbits.TRISA1 = 1; } while(0)
#define UART_RX_SetDigitalOutput()   do { TRISAbits.TRISA1 = 0; } while(0)
#define UART_RX_SetPullup()          do { WPUAbits.WPUA1 = 1; } while(0)
#define UART_RX_ResetPullup()        do { WPUAbits.WPUA1 = 0; } while(0)
#define UART_RX_SetPushPull()        do { ODCONAbits.ODCA1 = 0; } while(0)
#define UART_RX_SetOpenDrain()       do { ODCONAbits.ODCA1 = 1; } while(0)
#define UART_RX_SetAnalogMode()      do { ANSELAbits.ANSA1 = 1; } while(0)
#define UART_RX_SetDigitalMode()     do { ANSELAbits.ANSA1 = 0; } while(0)
// get/set IO_RA2 aliases
#define LED_TRIS                 TRISAbits.TRISA2
#define LED_LAT                  LATAbits.LATA2
#define LED_PORT                 PORTAbits.RA2
#define LED_WPU                  WPUAbits.WPUA2
#define LED_OD                   ODCONAbits.ODCA2
#define LED_ANS                  ANSELAbits.ANSA2
#define LED_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define LED_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define LED_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define LED_GetValue()           PORTAbits.RA2
#define LED_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define LED_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define LED_SetPullup()          do { WPUAbits.WPUA2 = 1; } while(0)
#define LED_ResetPullup()        do { WPUAbits.WPUA2 = 0; } while(0)
#define LED_SetPushPull()        do { ODCONAbits.ODCA2 = 0; } while(0)
#define LED_SetOpenDrain()       do { ODCONAbits.ODCA2 = 1; } while(0)
#define LED_SetAnalogMode()      do { ANSELAbits.ANSA2 = 1; } while(0)
#define LED_SetDigitalMode()     do { ANSELAbits.ANSA2 = 0; } while(0)
// get/set IO_RA4 aliases
#define SCL_TRIS                 TRISAbits.TRISA4
#define SCL_LAT                  LATAbits.LATA4
#define SCL_PORT                 PORTAbits.RA4
#define SCL_WPU                  WPUAbits.WPUA4
#define SCL_OD                   ODCONAbits.ODCA4
#define SCL_ANS                  ANSELAbits.ANSA4
#define SCL_SetHigh()            do { LATAbits.LATA4 = 1; } while(0)
#define SCL_SetLow()             do { LATAbits.LATA4 = 0; } while(0)
#define SCL_Toggle()             do { LATAbits.LATA4 = ~LATAbits.LATA4; } while(0)
#define SCL_GetValue()           PORTAbits.RA4
#define SCL_SetDigitalInput()    do { TRISAbits.TRISA4 = 1; } while(0)
#define SCL_SetDigitalOutput()   do { TRISAbits.TRISA4 = 0; } while(0)
#define SCL_SetPullup()          do { WPUAbits.WPUA4 = 1; } while(0)
#define SCL_ResetPullup()        do { WPUAbits.WPUA4 = 0; } while(0)
#define SCL_SetPushPull()        do { ODCONAbits.ODCA4 = 0; } while(0)
#define SCL_SetOpenDrain()       do { ODCONAbits.ODCA4 = 1; } while(0)
#define SCL_SetAnalogMode()      do { ANSELAbits.ANSA4 = 1; } while(0)
#define SCL_SetDigitalMode()     do { ANSELAbits.ANSA4 = 0; } while(0)
// get/set IO_RA5 aliases
#define SDA_TRIS                 TRISAbits.TRISA5
#define SDA_LAT                  LATAbits.LATA5
#define SDA_PORT                 PORTAbits.RA5
#define SDA_WPU                  WPUAbits.WPUA5
#define SDA_OD                   ODCONAbits.ODCA5
#define SDA_ANS                  ANSELAbits.ANSA5
#define SDA_SetHigh()            do { LATAbits.LATA5 = 1; } while(0)
#define SDA_SetLow()             do { LATAbits.LATA5 = 0; } while(0)
#define SDA_Toggle()             do { LATAbits.LATA5 = ~LATAbits.LATA5; } while(0)
#define SDA_GetValue()           PORTAbits.RA5
#define SDA_SetDigitalInput()    do { TRISAbits.TRISA5 = 1; } while(0)
#define SDA_SetDigitalOutput()   do { TRISAbits.TRISA5 = 0; } while(0)
#define SDA_SetPullup()          do { WPUAbits.WPUA5 = 1; } while(0)
#define SDA_ResetPullup()        do { WPUAbits.WPUA5 = 0; } while(0)
#define SDA_SetPushPull()        do { ODCONAbits.ODCA5 = 0; } while(0)
#define SDA_SetOpenDrain()       do { ODCONAbits.ODCA5 = 1; } while(0)
#define SDA_SetAnalogMode()      do { ANSELAbits.ANSA5 = 1; } while(0)
#define SDA_SetDigitalMode()     do { ANSELAbits.ANSA5 = 0; } while(0)
/**
 * @ingroup  pinsdriver
 * @brief GPIO and peripheral I/O initialization
 * @param none
 * @return none
 */
void PIN_MANAGER_Initialize (void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt on Change Handling routine
 * @param none
 * @return none
 */
void PIN_MANAGER_IOC(void);


#endif // PINS_H
/**
 End of File
*/