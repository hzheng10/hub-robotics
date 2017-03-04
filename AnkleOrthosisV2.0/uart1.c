#include <xc.h>
#include <stdint.h>
#if __XC16_VERSION < 1011
#warning "Please upgrade to XC16 v1.11 or newer."
#endif
#include "uart1.h"

//UART1 communicates with PC

#define FCY 40000000
#define BAUDRATE 115200
#define BRGVAL ((FCY/BAUDRATE)/16)-1

/******************************************************************************
 * Function:  void __attribute__ ((interrupt, no_auto_psv)) _U1RXInterrupt(void)
 *
 * PreCondition: UART Module must be Initialized with receive interrupt enabled.
 *
 * Input:        None
 *
 * Output:       None
 *
 * Side Effects: None
 *
 * Overview:     UART receive interrupt service routine called whenever a byte
 *               of data received in UART Rx buffer.
 *****************************************************************************/
void __attribute__ ( (interrupt, no_auto_psv) ) _U1RXInterrupt( void )
{
    LATA = U1RXREG;
    IFS0bits.U1RXIF = 0;
}

/******************************************************************************
 * Function:   void __attribute__ ((interrupt, no_auto_psv)) _U1TXInterrupt(void)
 *
 * PreCondition: UART Module must be Initialized with transmit interrupt enabled.
 *
 * Input:        None
 *
 * Output:       None
 *
 * Side Effects: None
 *
 * Overview:     UART transmit interrupt service routine called whenever a data
 *               is sent from UART Tx buffer
 *****************************************************************************/
void __attribute__ ( (interrupt, no_auto_psv) ) _U1TXInterrupt( void )
{
    IFS0bits.U1TXIF = 0;
}

/******************************************************************************
 * Function:        void InitUART2()
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function configures UART module
 *****************************************************************************/
void InitUART1( void )
{
    // This is an EXAMPLE, so brutal typing goes into explaining all bit sets
    // The HPC16 board has a DB9 connector wired to UART2, so we will
    // be configuring this port only
    // configure U1MODE
    U1MODEbits.UARTEN = 0;  // Bit15 TX, RX DISABLED, ENABLE at end of func

    //U1MODEbits.notimplemented;// Bit14
    U1MODEbits.USIDL = 0;   // Bit13 Continue in Idle
    U1MODEbits.IREN = 0;    // Bit12 No IR translation
    U1MODEbits.RTSMD = 0;   // Bit11 Simplex Mode

    //U1MODEbits.notimplemented;// Bit10
    U1MODEbits.UEN = 0;     // Bits8,9 TX,RX enabled, CTS,RTS not
    U1MODEbits.WAKE = 0;    // Bit7 No Wake up (since we don't sleep here)
    U1MODEbits.LPBACK = 0;  // Bit6 No Loop Back
    U1MODEbits.ABAUD = 0;   // Bit5 No Autobaud (would require sending '55')
    U1MODEbits.BRGH = 0;    // Bit3 16 clocks per bit period
    U1MODEbits.PDSEL = 0;   // Bits1,2 8bit, No Parity
    U1MODEbits.STSEL = 0;   // Bit0 One Stop Bit

    // Load a value into Baud Rate Generator.  Example is for 9600.
    // See section 19.3.1 of datasheet.
    //  U1BRG = (Fcy/(16*BaudRate))-1
    //  U1BRG = (37M/(16*9600))-1
    //  U1BRG = 240
    //U1BRG = 389;            // 60Mhz osc, 9600 Baud

    // Load a value into Baud Rate Generator.  Example is for 9600.
    // See section 19.3.1 of datasheet.
    //  U1BRG = (Fcy/(16*BaudRate))-1
    //  U1BRG = (37M/(16*9600))-1
    //  U1BRG = 240
    //U1BRG = 389;            // 60Mhz osc, 9600 Baud

    U1BRG = BRGVAL;            // BAUD Rate Setting for 9600
    // Load all values in for U1STA SFR
    U1STAbits.UTXISEL1 = 0; //Bit15 Int when Char is transferred (1/2 config!)
    U1STAbits.UTXINV = 0;   //Bit14 N/A, IRDA config
    U1STAbits.UTXISEL0 = 0; //Bit13 Other half of Bit15

    //U1STAbits.notimplemented = 0;//Bit12
    U1STAbits.UTXBRK = 0;   //Bit11 Disabled
    U1STAbits.UTXEN = 0;    //Bit10 TX pins controlled by periph
    U1STAbits.UTXBF = 0;    //Bit9 *Read Only Bit*
    U1STAbits.TRMT = 0;     //Bit8 *Read Only bit*
    U1STAbits.URXISEL = 0;  //Bits6,7 Int. on character recieved
    U1STAbits.ADDEN = 0;    //Bit5 Address Detect Disabled
    U1STAbits.RIDLE = 0;    //Bit4 *Read Only Bit*
    U1STAbits.PERR = 0;     //Bit3 *Read Only Bit*
    U1STAbits.FERR = 0;     //Bit2 *Read Only Bit*
    U1STAbits.OERR = 0;     //Bit1 *Read Only Bit*
    U1STAbits.URXDA = 0;    //Bit0 *Read Only Bit*
    IPC7 = 0x4400;          // Mid Range Interrupt Priority level, no urgent reason
    IFS0bits.U1TXIF = 0;    // Clear the Transmit Interrupt Flag
    IEC0bits.U1TXIE = 1;    // Enable Transmit Interrupts
    IFS0bits.U1RXIF = 0;    // Clear the Recieve Interrupt Flag
    IEC0bits.U1RXIE = 0;    // Disable Recieve Interrupts
    //RPOR1bits.RP36R = 1;    //RB4 as U1TX
    //RPINR18bits.U1RXR = 24; //RA8 as U1RX

    RPOR5bits.RP11R = 3;    //RB11 as U1TX
    //RPINR18bits.U1RXR = 10; //RB10 as U1RX
    RPINR18bits.U1RXR = 14; //RB14 as U1RX


    
    
    U1MODEbits.UARTEN = 1;  // And turn the peripheral on
    U1STAbits.UTXEN = 1;
}

void InitUART1_sd( void )
{
    // This is an EXAMPLE, so brutal typing goes into explaining all bit sets
    // The HPC16 board has a DB9 connector wired to UART2, so we will
    // be configuring this port only
    // configure U1MODE
    U1MODEbits.UARTEN = 0;  // Bit15 TX, RX DISABLED, ENABLE at end of func

    //U1MODEbits.notimplemented;// Bit14
    U1MODEbits.USIDL = 0;   // Bit13 Continue in Idle
    U1MODEbits.IREN = 0;    // Bit12 No IR translation
    U1MODEbits.RTSMD = 0;   // Bit11 Simplex Mode

    //U1MODEbits.notimplemented;// Bit10
    U1MODEbits.UEN = 0;     // Bits8,9 TX,RX enabled, CTS,RTS not
    U1MODEbits.WAKE = 0;    // Bit7 No Wake up (since we don't sleep here)
    U1MODEbits.LPBACK = 0;  // Bit6 No Loop Back
    U1MODEbits.ABAUD = 0;   // Bit5 No Autobaud (would require sending '55')
    U1MODEbits.BRGH = 0;    // Bit3 16 clocks per bit period
    U1MODEbits.PDSEL = 0;   // Bits1,2 8bit, No Parity
    U1MODEbits.STSEL = 0;   // Bit0 One Stop Bit

    // Load a value into Baud Rate Generator.  Example is for 9600.
    // See section 19.3.1 of datasheet.
    //  U1BRG = (Fcy/(16*BaudRate))-1
    //  U1BRG = (37M/(16*9600))-1
    //  U1BRG = 240
    //U1BRG = 389;            // 60Mhz osc, 9600 Baud

    // Load a value into Baud Rate Generator.  Example is for 9600.
    // See section 19.3.1 of datasheet.
    //  U1BRG = (Fcy/(16*BaudRate))-1
    //  U1BRG = (37M/(16*9600))-1
    //  U1BRG = 240
    //U1BRG = 389;            // 60Mhz osc, 9600 Baud

    U1BRG = BRGVAL;            // BAUD Rate Setting for 9600
    // Load all values in for U1STA SFR
    U1STAbits.UTXISEL1 = 0; //Bit15 Int when Char is transferred (1/2 config!)
    U1STAbits.UTXINV = 0;   //Bit14 N/A, IRDA config
    U1STAbits.UTXISEL0 = 0; //Bit13 Other half of Bit15

    //U1STAbits.notimplemented = 0;//Bit12
    U1STAbits.UTXBRK = 0;   //Bit11 Disabled
    U1STAbits.UTXEN = 0;    //Bit10 TX pins controlled by periph
    U1STAbits.UTXBF = 0;    //Bit9 *Read Only Bit*
    U1STAbits.TRMT = 0;     //Bit8 *Read Only bit*
    U1STAbits.URXISEL = 0;  //Bits6,7 Int. on character recieved
    U1STAbits.ADDEN = 0;    //Bit5 Address Detect Disabled
    U1STAbits.RIDLE = 0;    //Bit4 *Read Only Bit*
    U1STAbits.PERR = 0;     //Bit3 *Read Only Bit*
    U1STAbits.FERR = 0;     //Bit2 *Read Only Bit*
    U1STAbits.OERR = 0;     //Bit1 *Read Only Bit*
    U1STAbits.URXDA = 0;    //Bit0 *Read Only Bit*
    IPC7 = 0x4400;          // Mid Range Interrupt Priority level, no urgent reason
    IFS0bits.U1TXIF = 0;    // Clear the Transmit Interrupt Flag
    IEC0bits.U1TXIE = 1;    // Enable Transmit Interrupts
    IFS0bits.U1RXIF = 0;    // Clear the Recieve Interrupt Flag
    IEC0bits.U1RXIE = 0;    // Disable Recieve Interrupts
    //RPOR1bits.RP36R = 1;    //RB4 as U1TX
    //RPINR18bits.U1RXR = 24; //RA8 as U1RX

    RPOR6bits.RP13R = 3;    //RB13 as U1TX
    RPINR18bits.U1RXR = 12; //Assign U1Rx To Pin RP12

    U1MODEbits.UARTEN = 1;  // And turn the peripheral on
    U1STAbits.UTXEN = 1;
}

/******************************************************************************
 * Function:        void SoftwareDebounce()
 *
 * PreCondition:    UART module should be initialized
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function checks which  button is pressed in Explorer 16
 *                  board.Depending on button pressed different data is written
 *                  to UART TX register to transfer.
 *****************************************************************************/

//void SoftwareDebounce( void )
void SoftwareDebounce( unsigned char Data )
{
    //U1TXREG = 'A';
    U1TXREG = Data;
}

/***************************************************************************
* Function Name     : putsUART1                                            *
* Description       : This function puts the data string to be transmitted *
*                     into the transmit buffer (till NULL character)       *
* Parameters        : unsigned int * address of the string buffer to be    *
*                     transmitted                                          *
* Return Value      : None                                                 *
***************************************************************************/

void putsUART1(unsigned char *buffer)
{
    char * temp_ptr = (char *) buffer;

    /* transmit till NULL character is encountered */

    if(U1MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer != '\0')
        {
            while(U1STAbits.UTXBF); /* wait if the buffer is full */
            U1TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
    else
    {
        while(*temp_ptr != '\0')
        {
            while(U1STAbits.UTXBF);  /* wait if the buffer is full */
            U1TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
}

/***************************************************************************
* Function Name     : ReadUART1                                            *
* Description       : This function returns the contents of UxRXREG buffer *
* Parameters        : None                                                 *
* Return Value      : unsigned int value from UxRXREG receive buffer       *
***************************************************************************/

unsigned int ReadUART1(void)
{
    if(U1MODEbits.PDSEL == 3)
        return (U1RXREG);
    else
        return (U1RXREG & 0xFF);
}

/*********************************************************************
* Function Name     : DataRdyUart1                                   *
* Description       : This function checks whether there is any data *
*                     that can be read from the input buffer, by     *
*                     checking URXDA bit                             *
* Parameters        : None                                           *
* Return Value      : char if any data available in buffer           *
*********************************************************************/

char DataRdyUART1(void)
{
    return(U1STAbits.URXDA);
}

/*********************************************************************
* Function Name     : WriteUART1                                     *
* Description       : This function writes data into the UxTXREG,    *
* Parameters        : unsigned int data the data to be written       *
* Return Value      : None                                           *
*********************************************************************/

void WriteUART1(unsigned int data)
{
    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}
