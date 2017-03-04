/******************************************************************************/
/*This file is to test finite state machine with sensor inputs.                                                   */
/******************************************************************************/

/* Device header file */
//#include <p33FJ128MC802.h>
#if defined(__XC16__)
#include <xc.h>
#elif defined(__C30__)
#if defined(__dsPIC33E__)
#include <p33Exxxx.h>
#elif defined(__dsPIC33F__)
#include <p33Fxxxx.h>
#endif
#endif


#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include "system.h"        /* System funct/params, like osc/peripheral config */
#include <libpic30.h>
#include "AnalogIO.h"      // ADC + DAC I/O Library
#include "SPI.h"
#include <float.h>         // Floating point math definition
#include "States.h"         //Contains methods and variables for each walking state 1--4

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

/* i.e. uint16_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/


// Configuration bits ---------------------------------------------
_FOSCSEL(FNOSC_FRCPLL); //set clock for internal OSC with PLL
_FOSC(OSCIOFNC_OFF & POSCMD_NONE); //no clock output, external OSC disabled
_FWDT(FWDTEN_OFF); //disable the watchdog timer
_FICD(JTAGEN_OFF); //disable JTAG, enable debugging on PGx1 pins

//void ConfigureAnalog();
//void DAConfig();
void setVoltage(unsigned int chan, unsigned int voltage_samp);
//unsigned int ReadAnalogSample(unsigned int input_pin);

enum states{
    ST_EARLY_STANCE,
    ST_PRE_SWING,
    ST_SW_FLEXION,
    ST_SW_EXTENSION
};
double heel_sensor = 0.0;
double knee_angle = 0.0;
double force_load_cell = 0.0;
enum states state = ST_EARLY_STANCE;
float a=0.85;
int16_t main(void) {

    double valve_command = 0.0;
    /* Configure the oscillator for the device */
    ConfigureOscillator(); // Must be called at beginning of main routine
    ConfigureAnalog(); // Must be called before while loop to use ReadAnalogSample(...)
    DAConfig(); // Must be called before setVoltage(...)
    valve_command = 0;
    int d =1;
    while (1) {
        if (d == 1){
            if (valve_command <5)
                    valve_command =valve_command+0.5;
            else
            {
                valve_command =5;
                d = 0;
            }
        }
        else
            if (valve_command >0)
                    valve_command =valve_command-0.5;
            else
            {
                valve_command =0;
                d = 1;
            }
        setVoltage(0,Voltage2Samp(valve_command));       // output the valve command
        __delay_us(1000000);
    }
}


