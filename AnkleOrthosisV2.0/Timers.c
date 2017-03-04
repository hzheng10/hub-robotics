#include <p33FJ128MC802.h>
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

/* Use the following loop to catch interrupt
 * while(IFS0bits.T1IF==0){
 *   //Pause until interrupt occurs
 * }
*/


void TimerConfig()
{

    T1CONbits.TON = 0;// Disable Timer
    T1CONbits.TCS = 0;// Select internal instruction cycle clock
    T1CONbits.TGATE = 0;// Disable Gated Timer mode
    T1CONbits.TCKPS = 0b11;// Select 1:256 Prescaler (156250Hz)
    TMR1 = 0x00; // Clear timer register
    PR1 = 5000; // Load the period value (about 1s)
    IPC0bits.T1IP = 0x01;// Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0;// Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1;// Enable Timer1 interrupt
    T1CONbits.TON = 1;// Start Timer

}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void){
        /*void __attri Interrupt Service Routine code goes here */
        IFS0bits.T1IF = 0; // Clear Timer1 Interrupt Flag
}

