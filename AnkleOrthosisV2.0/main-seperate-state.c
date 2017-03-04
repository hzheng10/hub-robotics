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
enum states state = ST_SW_EXTENSION;
float a=0.85;
int16_t main(void) {

    //unsigned int sample;
    double moment_arm = 0.0;
    double valve_command = 0.0;
    double force_load_cell = 0.0;
    double desired_force = 0.0;
    double test =0.0;
    //parameters
    double heel_sensor_level_foot=0.4;
    double heel_sensor_toe_off=0.8;
    double swing_flexion_angle=45.0;
    double count = 0;
    double u1,u2,u3,y1,y2,y3;
 
    /* Configure the oscillator for the device */
    ConfigureOscillator(); // Must be called at beginning of main routine
    ConfigureAnalog(); // Must be called before while loop to use ReadAnalogSample(...)
    DAConfig(); // Must be called before setVoltage(...)
    
    TRISAbits.TRISA2 = 0; // Set pin 9 (RA2) as ouput
    LATAbits.LATA2 = 0; // Set pin 9 (RA2) to digital/logic low
    TRISAbits.TRISA4 = 0; // Set pin 12 (RA4) as ouput
    LATAbits.LATA4 = 0; // Set pin 12 (RA4) to digital/logic low

    AD1PCFGLbits.PCFG5=1;   // Set pin 7 RB3.AN5 as digital pins.
    TRISBbits.TRISB3 = 1; //Set pin 7 RB3/AN5 as input
    y1 = Samp2Voltage(ReadAnalogSample(0)); //read voltage from channel 0 (AN2) PIN2
    y2 = Samp2Voltage(ReadAnalogSample(1));  //read voltage froom channel 1 (AN3) PIN3
    y3 = Samp2Voltage(ReadAnalogSample(4));    // read voltage from channel 4 (AN4) PIN6

    while (1) {
        // Main program goes here
        // Read analog signal  u1- heel load cell.   u2 - knee angle.  u3 - cylinder load cell.
        u1= Samp2Voltage(ReadAnalogSample(0)); //read voltage from channel 0 (AN2) PIN2
        // Filtered heel_sensor
        y1 = a*y1+(1-a)*u1; //0.855 0.145
        u2 = Samp2Voltage(ReadAnalogSample(1));  //read voltage froom channel 1 (AN3) PIN3
        // Filtered knee angle change this gain.
        y2 = 0.975*y2+0.025*u2;
        u3 = Samp2Voltage(ReadAnalogSample(4));    // read voltage from channel 4 (AN4) PIN6
        // Filtered force load cell
        y3 = a*y3+(1-a)*u3;
        force_load_cell = 3.03*y3-5;       // from 0 to 3.3 volt to -5 to 5 volt.
        force_load_cell = 100*force_load_cell*4.448+20; // convert -5 to 5 volt to force in N.
        knee_angle = KneeAngle(y2); //calculate the knee_angle from the potentiometer volage
        heel_sensor = y1;
        moment_arm = MomentArm(knee_angle); //calculate the moment_arm from the current knee_angle
        
        if (PORTBbits.RB3 )          // if the pin is high
        {
            //setVoltage(0,Voltage2Samp(2.5));
            LATAbits.LATA2 = 0; //led1 off
            LATAbits.LATA4 = 0; // led 2 off
            /*state = ST_SW_EXTENSION;*/
            switch (state)
            {
                case ST_EARLY_STANCE:
                        state = ST_PRE_SWING;
                        break;
                case ST_PRE_SWING:
                        state = ST_SW_FLEXION;
                        break;
                case ST_SW_FLEXION:
                        state = ST_SW_EXTENSION;
                        break;
                case ST_SW_EXTENSION:
                        state = ST_EARLY_STANCE;
                        break;
            }
        }
        else 
        {
            switch (state)
            {
                // state 0
                case ST_EARLY_STANCE:
                    LATAbits.LATA2 = 0; //led1 off
                    LATAbits.LATA4 = 0; // led 2 off

                    if (count>=2000) // delay 0.86 sec  (2000 for 1 sec)1680 for 0.86 sec
                    {
                        //state = ST_PRE_SWING;
                        count=0;
                        break;
                    }
                    //knee_angle = 20;
                    desired_force = DesiredForce(Impedance(knee_angle, 0.0, 2.0, 0.01, 2.0), moment_arm);
                    
                    valve_command = PIDController(desired_force, force_load_cell, 0.004); // calculate the valve_command from the PID controller
                    count+=1;
                    //valve_command = 2.76;
                    setVoltage(0,Voltage2Samp(valve_command));       // output the valve command
   
                    break;
                //state 1
                case ST_PRE_SWING:
                    LATAbits.LATA2 = 1;
                    LATAbits.LATA4 = 0;
                    /*if (heel_sensor <heel_sensor_toe_off)
                        state = ST_SW_FLEXION;*/
                    desired_force = DesiredForce(Impedance(knee_angle, 0, 1, 0, 30), moment_arm);
                    valve_command = PIDController(DesiredForce(Impedance(knee_angle, 0, 1, 0, 30), moment_arm), force_load_cell, 0.004); // calculate the valve_command from the PID controller
                    setVoltage(0,Voltage2Samp(valve_command));       // output the valve command

                    count=0;
                    break;
                //state 2
                case ST_SW_FLEXION:
                    LATAbits.LATA2 = 0;
                    LATAbits.LATA4 = 1;
                    /*if (knee_angle >=swing_flexion_angle)
                        state = ST_SW_EXTENSION;*/
                    desired_force = DesiredForce(Impedance(knee_angle, 0, 0.36, 0.01, 75), moment_arm);
                    valve_command = PIDController(DesiredForce(Impedance(knee_angle, 0, 0.36, 0.01, 75), moment_arm), force_load_cell, 0.011); // calculate the valve_command from the PID controller
                    // test
                    //valve_command = 5;
                    setVoltage(0,Voltage2Samp(valve_command));       // output the valve command
                  
                    count=0;
                    break;
                  //state 3
                  case ST_SW_EXTENSION:
                    LATAbits.LATA2 = 1;
                    LATAbits.LATA4 = 1;
                    /*if (heel_sensor >=heel_sensor_toe_off)
                        state = ST_EARLY_STANCE;*/
                    desired_force = DesiredForce(Impedance(knee_angle, 0, 0.4, 0.005, 0), moment_arm);
                    valve_command = PIDController(DesiredForce(Impedance(knee_angle, 0, 0.4, 0.005, 0), moment_arm), force_load_cell, 0.008); // calculate the valve_command from the PID controller
                    setVoltage(0,Voltage2Samp(valve_command));       // output the valve command
                    count=0;
                    break;
            }
        }
    }
}


