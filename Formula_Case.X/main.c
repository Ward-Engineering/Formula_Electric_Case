/*
 * File:   main.c
 * Author: Ward
 *
 * Created on 15 maart 2022, 23:36
 */


#include <xc.h>
#include <stdbool.h>
/*
 * Prototypes
 */
void __interrupt (high_priority) high_ISR(void);   //high priority interrupt routine
void __interrupt (low_priority) low_ISR(void);  //low priority interrupt routine, not used in this example
void initChip(void);
void start_adc(void);


/*
 * Global Variables
 */
//volatile int  counter = 0;
bool readingThrottle = false;
/*
 * Interrupt Service Routines
 */
/********************************************************* 
	Interrupt Handler
**********************************************************/
void __interrupt (high_priority) high_ISR(void)
{
    if(PIR1bits.ADIF == 1)
    {
        //we got an adc interrupt
        PORTB = ADRESH;
        PIR1bits.ADIF = 0;
        start_adc();
    }
    
}

/*
 * Functions
 */
 /*************************************************
			Main
**************************************************/
void main(void)
{
    initChip();
    start_adc();
    
    while(1)
    {
        //keep running
    }

}

void start_adc(void)
{
    if(readingThrottle)
    {
        //read throttle
        ADCON0bits.CHS = 0b00000;      
    }
    else
    {
        //read brake
        ADCON0bits.CHS = 0b00001;
    }
    ADCON0bits.GODONE = 1;
    readingThrottle = !readingThrottle;
}

/*************************************************
			Initialize the CHIP
**************************************************/
void initChip(void)
{
	//CLK settings, should be fine
	OSCTUNE = 0x80; //3X PLL ratio mode selected
	OSCCON = 0x70; //Switch to 16MHz HFINTOSC
	OSCCON2 = 0x10; //Enable PLL, SOSC, PRI OSC drivers turned off
	while(OSCCON2bits.PLLRDY != 1); //Wait for PLL lock
	ACTCON = 0x90; //Enable active clock tuning for USB operation

    LATA = 0x00; //Initial PORTA
    TRISA = 0xFF; //Define PORTA as input
    ADCON1 = 0x00; //AD voltage reference
    ANSELA = 0b00000001; // define analog or digital
    CM1CON0 = 0x00; //Turn off Comparator
    LATB = 0x00; //Initial PORTB
    TRISB = 0x00; //Define PORTB as output
    LATC = 0x00; //Initial PORTC
    TRISC = 0x00; //Define PORTC as output
    
    //enable adc 
    ADCON0bits.ADON = 1;
    ANSELAbits.ANSA0 = 1;
    ANSELAbits.ANSA1 = 1;
    ADCON2 = 0b00100101;
    //adc interrupts
    INTCON = 0b11000000;
    IPR1bits.ADIP = 1;
    PIE1bits.ADIE = 1;
}
