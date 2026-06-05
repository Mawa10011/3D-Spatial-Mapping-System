// PLL.h
// Runs on TM4C1294
// A software function to change the bus frequency using the PLL.
// Daniel Valvano
// March 27, 2014
// Edited by Mawa Hassan

/* This example accompanies the book
   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2014
   Program 4.6, Section 4.3
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Program 2.10, Figure 2.37

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
 */

// CHANGE MADE HERE!!
// SysClk = 520 MHz / 19 + 1 = 26
#define PSYSDIV 19

// configure the system to get its clock from the PLL
void PLL_Init(void);

/*
Original Valvano table for 480 MHz VCO:
PSYSDIV  SysClk (Hz)
  3     120,000,000
  4      96,000,000
  5      80,000,000
  7      60,000,000
  9      48,000,000
 15      30,000,000
 19      24,000,000
 29      16,000,000
 39      12,000,000
 79       6,000,000

For this project version:
- We changed the VCO in PLL.c from 480 MHz to 520 MHz
- With PSYSDIV = 19:
  SysClk = 520,000,000 / 20 = 26,000,000 Hz
*/