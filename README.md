# A simple cyclic executive for the MBED unit

## Description
Design, build and test a simple Cyclic Executive for the MBED unit using an interval of 20ms.
There are 8 actions:
* Measure the frequency of a 3.3v square wave signal once every second
* Read ONE digital input every 300mS
* Output a watchdog pulse every 300mS
* Read TWO analogue inputs every 400mS
* Display the values on the LCD display every 2 seconds
* Check error code every 800mS
* Log the values every 5 seconds (in comma delimited format) onto a uSD card
* Use other slots to check a shutdown switch
  
 Given parameters are:
  Data logging : 1=log to uSD card
  Length of watchdog pulse Range : 20 mS
  Display of error condition: 2=display as flashing blue LED in MBED board
  Show execution time pulse for one of the actions: 1

## Authors
  Roshenac Mitchell  March 2016

