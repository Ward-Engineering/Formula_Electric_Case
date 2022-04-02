/*
 * File:   main.c
 * Author: Ward
 *
 * Created on 15 maart 2022, 23:36
 */


#include <xc.h>
#include <stdbool.h>


#define SERVO LATCbits.LATC7
/*
 * Prototypes
 */
void __interrupt (high_priority) high_ISR(void);   //high priority interrupt routine
void __interrupt (low_priority) low_ISR(void);  //low priority interrupt routine, not used in this example
void initChip(void);
void start_adc(void);
void stop_adc(void);
void enable_adc(void);
void init_state(void);
bool getONOFFToggle(void);


/*
 * Global Variables
 */
//volatile int  counter = 0;
bool readingThrottle = false;
bool ONOFFWasPressed = false;
char throttleInput = 0;
char brakeInput = 0;
int brakeCounter = 0;
int brakeOnTime = 0;
static enum{FSM_ready_to_drive,FSM_not_ready_to_drive}car_state;
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
        //adc interrupt
        if(readingThrottle)
        {
            throttleInput = ADRESH;
        }
        else
        {
            brakeInput = ADRESH;
        }
        
        if((brakeInput > 7 && throttleInput > 7))
            throttleInput = 0;
        CCPR2L = throttleInput;
        
        PIR1bits.ADIF = 0;
        readingThrottle = !readingThrottle;
        start_adc();
    }
    if(INTCONbits.TMR0IF == 1)
    {
        if(LATCbits.LATC6 == 1)
        {
            LATCbits.LATC6 = 0;
        }
        else 
            LATCbits.LATC6 = 1;
        //apply brake
        //total period is 20000 us, range is 575 - 2460 us, interrupt gets called every 20 us
        if(brakeCounter > brakeOnTime)
        {
            SERVO = 0;
        }
        
        brakeCounter ++;
        if(brakeCounter > 1000)
        {
            brakeCounter = 0;
            brakeOnTime = brakeInput *0.09 + 29;
            SERVO = 1;
        }
        TMR0L = 16;
        INTCONbits.TMR0IF = 0;
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
    init_state();
    
    while(1)
    {       
        //keep running
        switch(car_state)
        {
            case FSM_not_ready_to_drive:
                if(getONOFFToggle())
                {
                    car_state = FSM_ready_to_drive;
                    enable_adc();
                    start_adc();
                }
                LATAbits.LATA7 = 0;
                
                brakeInput = 0;
                throttleInput = 0;
                break;
                
            case FSM_ready_to_drive:
                if(getONOFFToggle())
                {
                    car_state = FSM_not_ready_to_drive;
                    stop_adc();
                }
                LATAbits.LATA7 = 1;
                
                
                
                break;
                
        }
        
    }

}

bool getONOFFToggle(void)
{
    bool ONOFFPressed = (PORTCbits.RC0 == 0);
    bool toggled = (ONOFFPressed && !ONOFFWasPressed);
    ONOFFWasPressed = ONOFFPressed;
    return toggled;
}


void enable_adc(void)
{
    T1CONbits.TMR1ON = 1;
    ADCON0bits.ADON = 1;
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
}
void stop_adc(void)
{
    T1CONbits.TMR1ON = 1;
    ADCON0bits.ADON = 0;
    
}

void init_state(void)
{
    car_state = FSM_not_ready_to_drive;
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
    TRISA = 0b01111111; //Define PORTA as input
    ADCON1 = 0x00; //AD voltage reference
    ANSELA = 0b00000001; // define analog or digital
    CM1CON0 = 0x00; //Turn off Comparator
    LATB = 0x00; //Initial PORTB
    TRISB = 0x00; //Define PORTB as output
    LATC = 0x00; //Initial PORTC
    TRISC = 0b00000001; //Define PORTC as output, with the exception of rc0
    
    //enable adc 
    ADCON0bits.ADON = 1;
    ANSELAbits.ANSA0 = 1;
    ANSELAbits.ANSA1 = 1;
    ADCON2 = 0b00100101;
    //adc interrupts & timer0 interrupts
    INTCON = 0b11100000;
    INTCON2bits.TMR0IP = 1;
    IPR1bits.ADIP = 1;
    PIE1bits.ADIE = 1;
    //timer 2 for pwm, for the dc motor
    T2CON = 0b1111111;
    PR2 = 0xFF;
    CCP2CONbits.CCP2M = 0b1111;
    //timer 0
    T0CONbits.TMR0ON = 1;
    T0CONbits.T08BIT = 1;
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 0b000;
    
    
    
    
}
