//--------------------------------------------------------------------
// Name:            Chris Coulston
// Date:            Fall 2020
// Purp:            inLab08
//
// Assisted:        The entire class of EENG 383
// Assisted by:     Microchips 18F26K22 Tech Docs 
//-
//- Academic Integrity Statement: I certify that, while others may have
//- assisted me in brain storming, debugging and validating this program,
//- the program itself is my own work. I understand that submitting code
//- which is the work of other individuals is a violation of the course
//- Academic Integrity Policy and may result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board. I also understand that if I knowingly give my original work to
//- another individual that it could also result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board.
//------------------------------------------------------------------------
#include <pic18f26k22.h>

#include "mcc_generated_files/mcc.h"
#pragma warning disable 520     // warning: (520) function "xyz" is never called  3
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)

#define MIC_THRESHOLD  128

#define     NUM_SAMPLES     256

uint8_t fillBuffer = false;

uint8_t samplesCollected = false;

uint8_t adc_reading[NUM_SAMPLES];

uint16_t thresholdRange = 10;

void INIT_PIC(void);
void myTMR0ISR(void);

//----------------------------------------------
// Main "function"
//----------------------------------------------

void main(void) {
    //Current roadblock:
    //Frequency always too high, even accounting for a missing fudge value.
    //Questions:
    //Why are periods so short? 
    //How do you count the cycles for the fudge value?
    //Are periods *this* short normal? It seems like they're too short, missing fudge value or no. 
    

    uint16_t i;
    char cmd;

    SYSTEM_Initialize();

    // BEFORE enabling interrupts, otherwise that while loop becomes an
    // infinite loop.  Doing this to give EUSART1's baud rate generator time
    // to stablize - this will make the splash screen looks better
    TMR0_WriteTimer(0x0000);
    INTCONbits.TMR0IF = 0;
    while (INTCONbits.TMR0IF == 0);

    TMR0_SetInterruptHandler(myTMR0ISR);
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    printf("inLab 08\r\n");
    printf("Microphone experiments\r\n");
    printf("Dev'21\r\n");
    printf("> "); // print a nice command prompt

    //Get something into the buffer to make the ISR work. 
    //While loop only runs once as part of setup.
    ADCON0bits.GO_NOT_DONE = 1;
    while (ADCON0bits.GO_NOT_DONE == 1);
            
    
    for (;;) {
        if(samplesCollected){
            samplesCollected = false;
            printf("The last %d ADC samples from the microphone are: \r\n", NUM_SAMPLES);
            //Analyze samples here.
            for(uint16_t i = 0;i<NUM_SAMPLES;i++){
                if(i % 16 == 0){
                    printf("\r\nS[%d] ", i);
                }
                printf("%d ", adc_reading[i]);
            }
            printf("\r\nThe sound wave crossed at the following indices:\r\n");
            uint8_t crossings[NUM_SAMPLES/2];
            uint16_t crIdx = 0;
            for(uint16_t i = 1;i<NUM_SAMPLES;i++){
                if(adc_reading[i-1] <= 128 && adc_reading[i] > 128){
                    crossings[crIdx] = i - 1;
                    crIdx += 1;
                }
            }
            for(uint16_t i = 0;i<crIdx;i++){
                printf("%d ", crossings[i]);
            }
            printf("\r\nThe sound wave had %d periods:\r\n", crIdx);
            uint16_t periodSum = 0;
            for(uint16_t i = 1;i<crIdx;i++){
                printf("%d - %d = %d\r\n", crossings[i], crossings[i-1], crossings[i] - crossings[i-1]);
                periodSum += crossings[i] - crossings[i-1];
            }
            uint16_t avgPeriod = periodSum/crIdx;
            uint16_t avgPeriodUs = avgPeriod * 25;
            printf("\r\naverage period = %d us\r\n", avgPeriodUs);
            printf("\r\average frequendy = %d Hz\r\n", 1000000/avgPeriodUs);
        }

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                case '?':
                    printf("------------------------------\r\n");
                    printf("?: Help menu\r\n");
                    printf("o: k\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("T/t: Increase/decrease threshold 138-118\r\n");
                    printf("f: gather %d samples from the microphone and calculate the frequency\r\n", NUM_SAMPLES);
//                    printf("s: gather %d samples from ADC\r\n", NUM_SAMPLES);
                    printf("------------------------------\r\n");
                    break;

                    //--------------------------------------------
                    // Reply with "k", used for PC to PIC test
                    //--------------------------------------------    
                case 'o':
                    printf(" k\r\n>");
                    break;

                    //--------------------------------------------
                    // Reset the processor after clearing the terminal
                    //--------------------------------------------                      
                case 'Z':
                    for (i = 0; i < 40; i++) printf("\n");
                    RESET();
                    break;

                    //--------------------------------------------
                    // Clear the terminal
                    //--------------------------------------------                      
                case 'z':
                    for (i = 0; i < 40; i++) printf("\n>");
                    break;
                    
                case 'T':
                    thresholdRange += 5;
                    printf("Volume range: %d - %d\r\n", MIC_THRESHOLD - thresholdRange, MIC_THRESHOLD + thresholdRange);
                    break;
                case 't':
                    if(thresholdRange > 0){
                        thresholdRange -= 5;
                    printf("Volume range: %d - %d\r\n", MIC_THRESHOLD - thresholdRange, MIC_THRESHOLD + thresholdRange);
                    }
                    else{
                        printf("Volume range: %d - %d\r\n", MIC_THRESHOLD - thresholdRange, MIC_THRESHOLD + thresholdRange);
                        printf("Threshold at minimum\r\n");
                    }
                    
                    break;
                case 'f':
                    
                    fillBuffer = true;
                    break;
/*              
                    //--------------------------------------------
                    // Continue to collect samples until the user
                    // presses a key on the keyboard
                    //--------------------------------------------                                          
                case's':
                    for (i = 0; i < NUM_SAMPLES; i++) {
                        while (NEW_SAMPLE == false);
                        NEW_SAMPLE = false;
                        adc_reading[i] = ADRESH;
                    } // end while

                    printf("The last %d ADC samples from the microphone are:\r\n", NUM_SAMPLES);
                    for (i = 0; i < NUM_SAMPLES; i++) // print-out samples
                        printf("%d ", adc_reading[i]);
                    printf("\r\n");
                    break;
 */
/*
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    ADC_SelectChannel(cmd - '0');
                    printf("The ADC is now sampling AN%c\r\n", cmd);
                    break;
*/
                    //--------------------------------------------
                    // If something unknown is hit, tell user
                    //--------------------------------------------
                default:
                    printf("Unknown key %c\r\n", cmd);
                    break;
            } // end switch
        } // end if
    } // end while 

} // end main

//-----------------------------------------------------------------------------
// Start an analog to digital conversion every 100uS.  Toggle RC1 so that users
// can check how fast conversions are being performed.
//-----------------------------------------------------------------------------
typedef enum  {MIC_IDLE, MIC_WAIT_FOR_TRIGGER, MIC_ACQUIRE} myTMR0states_t;
myTMR0states_t timerState = MIC_IDLE;

uint16_t bufferIdx = 0;
void myTMR0ISR(void) {
    //Ensure that there is always something in the buffer. 
//    while(ADCON0bits.GO_NOT_DONE == 1);
    uint8_t micReading = ADRESH;
    //Always need something in the buffer
    //make CERTAIN that you give this as much time as you possibly can to work.
    //It should always be the second line of this ISR
    //I tried to put it at the end and it broke everything. 
    ADCON0bits.GO_NOT_DONE = 1; 
    //Each ISR will tell the ADC to take the sample now. By the next ISR, the sample should be through the ADC. 
    //This means that the data an ISR places into the buffer at a given interrupt is the data it got during the last interrupt.
    //I believe this doesn't actually cause problems?
    switch(timerState){
        
        
        case MIC_IDLE:
            if(fillBuffer){
                timerState = MIC_WAIT_FOR_TRIGGER;
                bufferIdx = 0;
                fillBuffer = false;
                
            }
            break;
        case MIC_WAIT_FOR_TRIGGER:
            //Start sampling when sound in threshold
            if(micReading <= MIC_THRESHOLD + thresholdRange && micReading >= MIC_THRESHOLD - thresholdRange){
                adc_reading[bufferIdx] = micReading;
                bufferIdx += 1;
                timerState = MIC_ACQUIRE;
            }
            break;
        case MIC_ACQUIRE:
            adc_reading[bufferIdx] = micReading;
            bufferIdx += 1;
            if(bufferIdx >= NUM_SAMPLES){
                //Read buffer full, stop reading and notify main
                samplesCollected = true;
                timerState = MIC_IDLE;
                
                
            }
            break;
        
        
        
        //Need to add fudge value. I have no idea how to count the cycles.
        //We'll probably need to ask a TA.
        
        TMR0_WriteTimer(0x10000 - 400);
        INTCONbits.TMR0IF = 0;
        
    }
    

    
    

} // end myTMR0ISR