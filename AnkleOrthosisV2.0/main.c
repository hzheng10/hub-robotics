/******************************************************************************/
/* Full controller of the ankle orthosis
 * Updated 11/27/2016
 */
/******************************************************************************/

/* Device header file */
#include <p33FJ128MC802.h>
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
#include <stdio.h>
#include "system.h"        /* System funct/params, like osc/peripheral config */
#include <libpic30.h>
#include "AnalogIO.h"      /* ADC + DAC I/O Library */
#include <float.h>         // Floating point math definition
#include "StateFormulas.h" //Contains methods and variables for each walking state 1--4
#include "i2c_emem.h"
#include "uart1.h"
#include "uart2.h"
#include "MPU6050.h"
#include <math.h>
#include "timer1.h"      // Includes the micro() and millis()functions
#include "libas.h"
#include "pwm.h"
#include "controller.h"

// Configuration bits ---------------------------------------------
_FOSCSEL(FNOSC_FRCPLL); //set clock for internal OSC with PLL
_FOSC(OSCIOFNC_OFF & POSCMD_NONE); //no clock output, external OSC disabled
_FWDT(FWDTEN_OFF); //disable the watchdog timer
_FICD(JTAGEN_OFF); //disable JTAG, enable debugging on PGx1 pins
_FBS(BSS_NO_FLASH);

void InitQEI(void)
{
    //ADPCFG |= 0x0038; // Configure QEI pins as digital inputs

    QEICONbits.QEIM = 0; // Disable QEI Module
    QEICONbits.CNTERR = 0; // Clear any count errors
    QEICONbits.QEISIDL = 0; // Continue operation during sleep
    QEICONbits.SWPAB = 0; // QEA and QEB not swapped
    QEICONbits.PCDOUT = 0; // Normal I/O pin operation
    QEICONbits.POSRES = 1; // Index pulse resets position counter
    DFLTCONbits.CEID = 1; // Count error interrupts disabled
    DFLTCONbits.QEOUT = 1; // Digital filters output enabled for QEn pins
    DFLTCONbits.QECK = 5; // 1:64 clock divide for digital filter for QEn
    RPINR14bits.QEA1R = 6; //RB6 as QEI1 Phase A PIN15
    RPINR14bits.QEB1R = 7; //RB7 as QEI1 Phase B PIN16
    POSCNT = 0; // Reset position counter
    QEICONbits.QEIM = 6; // X4 mode with position counter reset by Index
    return;
}

int POSCNTcopy = 0;
float rev = 0, ankle_angle = 0;
int overflow =0;
float AngleFromQEnc(void)
{
    POSCNTcopy = (int)POSCNT;
    rev = overflow*30+POSCNTcopy/1000.0; // the turns of the motor
    if ( POSCNTcopy >= 30000)    // overflow over 5 turn in positive direction
    {
        overflow++;
        POSCNT = 0;// Reset position counter
    }
    else if ( POSCNTcopy <= -30000)  //// overflow over 5 turn in negtive direction
    {
        overflow--;
        POSCNT = 0;// Reset position counter
    }
    ankle_angle = -rev/200.0*360;    // the transmission ratio is 240
    return(ankle_angle);
}

int main(void) {
    const float gear_ratio = 200.0;
    const float torque_const = 37.0;    //mNm/A
    const float pi = 3.14;
    double cv, cv_filter;   // motor current pin voltage and filtered motor current pin voltage
    float mc = 0,mc_sum = 0, mc_thres = 70;  // mc: motor current
    float percent = 0;
    float power = 0, torque = 0, power_filter = 0;    // output power and torque
    
    struct imu_data imuBase;    // imu data
    extern I2CEMEM_DRV i2cmem;
    int16_t AcY1 = 0;
    
    float a=0.85;   // parameter for digital filter
    int st;         // state number
    const int ST_LATE_SWING = 4;
    struct st_impedance my_st_impedance;
    bool odd = true;
    float impedance;
    
    /* Configure the oscillator for the device */
    ConfigureOscillator(); // Must be called at beginning of main routine
    ConfigureAnalog(); // Must be called before while loop to use ReadAnalogSample(...)
    
    /* For serial monitor */
    InitUART1();    // Initialize UART1 for 115200,8,N,1 TX/RX as serial monitor
    /* For SD card */
    //InitUART1_sd(); // Initialize UART1 for 115200,8,N,1 TX/RX as SD connection
    printf("Uart1 and Uart2 is ready\n");
    
    InitTimer1();   //Initialized Timer1 module
    printf("Timer is ready\n");
    InitEncoder(9, 7, 12);  //pin 9 (RA2) as ouput CLK; pin 12 (RA4) as ouput CS ;pin 7 RB3/AN5 as input DI
    printf("AS5145 is ready\n");
    init_DIR(); // Initialize the Direction Pin
    init_PWM(); // Initialize the PWM module
    printf("PWM module is ready\n");
    InitQEI();  // Initialize the quadrature encoder module
    printf("Quadrature Encoder is ready\n");
    Stop ();
    i2cmem.init( &i2cmem ); // Initialise I2C peripheral and Driver
    initImu();  // Initialized the IMU and test the I2C Communication
    printf("I2C and IMU is ready:\n");
    // calibrate the IMU data
    imuBase = calibrateImu ();
    // print base data
    printf("Base data:");
    printImuData (imuBase);

    float angle = 0, angle_old = 0,ankle_velocity;

    //SWITCH
    AD1PCFGLbits.PCFG1=1;   // Set pin 3 RA3.AN1 as digital pins.
    TRISAbits.TRISA1 = 1; //Set pin 3 RA1/AN1 as input
    cv_filter = Samp2Voltage(ReadAnalogSample(0)); //read voltage from channel 0 (AN0) PIN2  is the motor current
    angle_old = AngleFromQEnc();
    __delay_ms(100);
    IEC0bits.T1IE = 1;// Enable Timer1 interrupt
    double ankle_v_filter = 0, ankle_v_radian = 0;
    float t_old = 0;
    float t_now = 0;
    float dt =  2e-3,time_sum = 0, mc_factor = 1;        // control cycle time in s
    float time_threshold_us = 200000;    //500 ms

/*    
    while(1)
    {
        angle = AngleFromQEnc();
        printf("%.1f\n", angle);
    }
*/

    while(1)                 
    {
        //LATBbits.LATB15 = !LATBbits.LATB15;
        angle = AngleFromQEnc(); // angle is 0 deg when foot is flat. maximum plantarflexion is -25 deg
        angle = a*angle_old + (1-a)*angle;

        t_now = micros();
        dt = t_now - t_old;
        t_old = t_now;

        //printf("%.2f,",dt);
        //deg/sec
        ankle_velocity = (angle - angle_old)/dt*1000000;    //deg/s
        ankle_v_filter = a*ankle_v_filter + (1-a)*ankle_velocity;
        ankle_v_radian = ankle_v_filter*pi/180;
        //printf("%.2f\n",ankle_v_filter);
        angle_old = angle;

        cv = Samp2Voltage(ReadAnalogSample(0)); // read motor current voltage from AN0
        cv_filter = a*cv_filter+(1-a)*cv; //0.855 0.145
        mc = 6.4*cv_filter; // motor current
        //printf("%.2f\n",current);

        // This is to control what data to log in.
        if (odd == true)
        {
            printf("%.1f,",angle);
            printf("%.1f,",ankle_v_filter);
            printf("%.1f,",impedance);
            //printf("%.1f,",dt/1000);
            odd = false;
        }
        else
        {
            //printf("%.1f\n",impedance);
            printf("%d,",st);
            printf("%.1f,",torque);
            printf("%.1f\n",power);

            odd = true;
        }

        if (PORTAbits.RA1==0)          // Switch OFF if the pin is LOW which is close
        {
            Stop(); //stop the motor
            printf("%Please Turn on USER Switch\n");
            // LED is OFF
            //LATBbits.LATB15 = 0;
            //printf("Switch is OFF\n");
        }
        else
        {
            //"1" == LED is ON
            //LATBbits.LATB15 = 1;
            //printf("%d\n",st);
            if (st == ST_LATE_SWING)
            {
                AcY1 = getAccelY( );

            }
            
            // Current limiting
            if (time_sum < time_threshold_us)
            {
                time_sum = time_sum + dt;
                if (mc > 3)
                {
                    mc_sum = mc + mc_sum;
                }
                if (mc_sum > mc_thres)
                {
                    mc_factor = 0.5;
                }
                else
                {
                    mc_factor = 1;
                }
            }
            else
            {
                time_sum = 0;
                mc_sum = 0;
            }

            my_st_impedance = controller_impedance(angle,ankle_v_filter,AcY1,mc_factor);
            st = my_st_impedance.st;
            impedance = my_st_impedance.impedance;
            percent = my_st_impedance.percent_new;
            torque = 0.9*percent*20*gear_ratio*torque_const/1000;
            power = 0.9*percent*20*gear_ratio*torque_const*ankle_v_radian/1000;
            
        }
    }
}   






