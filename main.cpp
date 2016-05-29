//*******************************************************************
//                           Lab2-cyclic executive
//             a simple cyclic executive for the MBED unit
//
// Description
//
// Design, build and test a simple Cyclic Executive for the MBED unit using an interval of 20ms. 
// There are 8 actions:
//  - Measure the frequency of a 3.3v square wave signal once every second
//  - Read ONE digital input every 300mS 
//  - Output a watchdog pulse every 300mS
//  - Read TWO analogue inputs every 400mS
//  - Display the values on the LCD display every 2 seconds
//  - Check error code every 800mS
//  - Log the values every 5 seconds (in comma delimited format) onto a uSD card
//  - Use other slots to check a shutdown switch
// 
// Given parameters are: 
// Data logging : 1=log to uSD card
// Length of watchdog pulse Range : 20 mS
// Display of error condition: 2=display as flashing blue LED in MBED board 
// Show execution time pulse for one of the actions: 1 
//
//
// Version
//    Roshenac Mitchell  March 2016
//*******************************************************************   

#include "mbed.h"
#include "SDFileSystem.h"
#include "WattBob_TextLCD.h"
#include <vector> 

/*---------------- INPUTS AND OUTPUT ----------*/

// LED used to show error 
DigitalOut errorLED(LED1);

//watchdog pulse
DigitalOut watchdog(p23);

//execution time pulse
DigitalOut executionPulse(p22);

//used to read in frequency pulse
DigitalIn wave(p11);

//digital input
DigitalIn digIn(p12);

// shutdown switch 
DigitalIn shutDown(p21);

// Analogue inputs
AnalogIn input1(p17);
AnalogIn input2(p18);

/*---------------- VARIABLES ----------*/

// float to hold period and frequency
float period = 0;
int frequency =0; 

// Digital input 
int switch1 =0;

// Analogue input readings
float current_analogue_in_1;
float current_analogue_in_2;

// Average analogue input 
float average_analogue_in_1 =0; 
float average_analogue_in_2 =0; 

// vector to hold last 4 analogue readings 
// set to 0 initially 
vector<float> analog1 (4,0); 
vector<float> analog2 (4,0);

// variable to hold error code
int errorCode =0; 

// pointer to 16-bit parallel I/O object
MCP23017 *par_port; 

// pointer to 2*16 chacater LCD object 
WattBob_TextLCD *lcd; 

// Ticker for cyclic executive
Ticker ticker;

// Variables for cyclic executive program
int ticks = 0; // Used to define what task to call in the cyclic executive program

// SD Card setup 
SDFileSystem sd(p5, p6, p7, p8, "sd");
FILE *fp;

/*---------------- TASKS----------*/

//1. Measure the frequency of a 3.3v square wave 
// execution pulse created to show how long the task takes
void measureFrequency()
{
    //Show execution time pulse
    executionPulse = 1;
    
    // create a timer
    Timer timer;
    timer.reset();
    
     // if wave is low, wait till high, start timer, wait till low, stop timer
    if(wave ==0)
    {
        while(wave ==0){}
        timer.start();
        while(wave==1){}
        timer.stop();     
    }else{
        // if wave is high, wait till low, start timer, wait till high, stop timer
        while(wave ==1){}   
        timer.start();
        while(wave==0){}
        timer.stop();
    }
    
    // period is the time it takes from low->high (or vice versa) * 2
    period = timer.read_us()*2;
    
    // frequency is 1/T (period) 
    frequency = 1000000/period; 
    
    // stop execution pulse 
    executionPulse = 0;
}


//2. Read ONE digital input 
void readDigitalInput()
{
    switch1 = digIn.read();
}


//3. Output a watchdog pulse
void outputWatchdog()
{
    watchdog = 1;
    //length of watchdog pulse Range : 20 mS
    wait_ms(20);
    watchdog = 0;
}


//4. Read TWO analogue inputs 
void readAnalogueInput()
{
    // read analogue input
    current_analogue_in_1 = input1.read();
    current_analogue_in_2 = input2.read();
    
    // Produce a filtered analogue values by averaging the last 4 readings.  
    
    // add new reading to start of vector
    analog1.insert(analog1.begin(), current_analogue_in_1);
    // remove reading from end of vector 
    analog1.erase(analog1.end());
    
    analog2.insert(analog2.begin(), current_analogue_in_2);
    analog2.erase(analog2.end());
    
    float sum1 = 0;
    float sum2 =0;
    
    // add the last 4 analogue readings
    for(int i= 0; i< analog1.size(); i++)
    {
        sum1 += analog1[i]; 
        sum2 += analog2[i];
    }
 
    // convert the reading so it is in the range 0 -> 5
    average_analogue_in_1 = (sum1/4) *5;
    average_analogue_in_2 = (sum2/4) * 5;
}


//5. Display the following on the LCD display
void display()
{
    // clear display
    lcd->locate(0,0);

    //print frequency value and digital value
    lcd->printf("%i  , %i", frequency, switch1);
   
    // move cursor to new line
    lcd->locate(1,0); 
    
    // print analogue value
    lcd->printf("%.2f , %.2f", average_analogue_in_1, average_analogue_in_2);
}


//6. Check error code 
void errorCodes()
{
    if((switch1 == 1) && (average_analogue_in_1 > average_analogue_in_2))
    {
      errorCode = 3;
    }else{
         errorCode = 0;
    }
    
    // if the error code is set, flash led 
    if(errorCode == 3)
    {
            errorLED = 1;
            wait_ms(5);
            errorLED = 0;
     }
}


//7. Log the following current values 
void logToSDCard()
{
    if(fp == NULL) {
        error("Could not open file for write\n");
    }
    
    //print frequency value , digital input values, Filtered analogue values 
     fprintf(fp, "%i , %i , %.2f , %.2f \n", frequency, switch1, average_analogue_in_1, average_analogue_in_2);
}


//8. check a shutdown switch.
void checkShutdownSwitch()
{
    int shutdownSwitch =  shutDown.read();
    if(shutdownSwitch)
    {   
        //SD file should be closed 
        fclose(fp);
        
         // clear display
        lcd-> cls();
        lcd->locate(0,0);
        lcd->printf("Remove SD card");
        exit(0);
    }
}
  
/*---------------- CYCLIC EXECTUTIVE ----------*/
// using clock time of 20mS 
void CyclEx()
{   
    if(ticks % 50 == 1)         // occurs every 1s (50 clock cycles) with an offset of 1 clock cycles
    {
        measureFrequency(); 
    }    
    else if(ticks % 15 == 2)    // occurs every 300mS (15 clock cycles) with an offset of 2 clock cycles
    {       
        readDigitalInput();      
    }
    else if(ticks % 15 == 3)    // occurs every 300mS (15 clock cycles) with an offset of 3 clock cycles
    {
        outputWatchdog();    
    }
    else if(ticks % 20 == 4)    // occurs every 400mS (20 clock cycles) with an offset of 4 clock cycles
    {
        readAnalogueInput(); 
    }
    else if(ticks % 100 == 10)    // occurs every 2s (100 clock cycles) with an offset of 10 clock cycles
    {   
        display(); 
        ticks++;
        ticks++;
    }
    else if(ticks % 40 == 6)    // occurs every 800mS (40 clock cycles) with an offset of 6 clock cycles
    {
        errorCodes(); 
    }
    else if(ticks % 250 == 7)  // occurs every 5s (250 clock cycles) with an offset of 7 clock cycles
    {
        logToSDCard(); 
    }
    else 
    {  
        checkShutdownSwitch(); 
    }
    
    // increment ticks 
    ticks++;
}



int main() {
    // initialise 16-bit I/O chip
    par_port = new MCP23017(p9, p10, 0x40); 
    
    // setup SD card
    mkdir("/sd/mydir", 0777);
    fp = fopen("/sd/mydir/sdtest.csv", "w");
    fprintf(fp, "frequency , digintal input , analogue input 1 , analogue input 2 \n");
    
    // set up for the LCD
    lcd = new WattBob_TextLCD(par_port); // initialise 2*26 char display
    lcd->cls(); 
    par_port->write_bit(1,BL_BIT); // turn LCD backlight ON 
    
    //set timer
    // the address of the cyclic executive and the interval (20 mS)
    ticker.attach(&CyclEx, 0.02); 
}
