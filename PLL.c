// PLL.c
// Runs on TM4C1294
// A software function to change the bus frequency using the PLL.
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
 
 
#include <stdint.h>
#include "PLL.h"

// The #define statement PSYSDIV in PLL.h
// initializes the PLL to the desired frequency.

// ---------------- EDIT MADE ----------------
// Original comment was for 480 MHz VCO and 120 MHz output.
// We are now targeting 26 MHz:
// SysClk = 520 MHz / (19 + 1) = 26 MHz
// -------------------------------------------

#define SYSCTL_RIS_R            (*((volatile uint32_t *)0x400FE050))
#define SYSCTL_RIS_MOSCPUPRIS   0x00000100
#define SYSCTL_MOSCCTL_R        (*((volatile uint32_t *)0x400FE07C))
#define SYSCTL_MOSCCTL_PWRDN    0x00000008
#define SYSCTL_MOSCCTL_NOXTAL   0x00000004
#define SYSCTL_RSCLKCFG_R       (*((volatile uint32_t *)0x400FE0B0))
#define SYSCTL_RSCLKCFG_MEMTIMU 0x80000000
#define SYSCTL_RSCLKCFG_NEWFREQ 0x40000000
#define SYSCTL_RSCLKCFG_USEPLL  0x10000000
#define SYSCTL_RSCLKCFG_PLLSRC_M   0x0F000000
#define SYSCTL_RSCLKCFG_PLLSRC_MOSC 0x03000000
#define SYSCTL_RSCLKCFG_OSCSRC_M   0x00F00000
#define SYSCTL_RSCLKCFG_OSCSRC_MOSC 0x00300000
#define SYSCTL_RSCLKCFG_PSYSDIV_M  0x000003FF
#define SYSCTL_MEMTIM0_R        (*((volatile uint32_t *)0x400FE0C0))
#define SYSCTL_DSCLKCFG_R       (*((volatile uint32_t *)0x400FE144))
#define SYSCTL_DSCLKCFG_DSOSCSRC_M    0x00F00000
#define SYSCTL_DSCLKCFG_DSOSCSRC_MOSC 0x00300000
#define SYSCTL_PLLFREQ0_R       (*((volatile uint32_t *)0x400FE160))
#define SYSCTL_PLLFREQ0_PLLPWR  0x00800000
#define SYSCTL_PLLFREQ0_MFRAC_M 0x000FFC00
#define SYSCTL_PLLFREQ0_MINT_M  0x000003FF
#define SYSCTL_PLLFREQ0_MFRAC_S 10
#define SYSCTL_PLLFREQ0_MINT_S  0
#define SYSCTL_PLLFREQ1_R       (*((volatile uint32_t *)0x400FE164))
#define SYSCTL_PLLFREQ1_Q_M     0x00001F00
#define SYSCTL_PLLFREQ1_N_M     0x0000001F
#define SYSCTL_PLLFREQ1_Q_S     8
#define SYSCTL_PLLFREQ1_N_S     0
#define SYSCTL_PLLSTAT_R        (*((volatile uint32_t *)0x400FE168))
#define SYSCTL_PLLSTAT_LOCK     0x00000001

void PLL_Init(void){ 
  uint32_t timeout;

  SYSCTL_RSCLKCFG_R &= ~SYSCTL_RSCLKCFG_USEPLL;

  SYSCTL_MOSCCTL_R &= ~(SYSCTL_MOSCCTL_NOXTAL|SYSCTL_MOSCCTL_PWRDN);

  while((SYSCTL_RIS_R&SYSCTL_RIS_MOSCPUPRIS)==0){};

  SYSCTL_RSCLKCFG_R = (SYSCTL_RSCLKCFG_R&~SYSCTL_RSCLKCFG_OSCSRC_M)+SYSCTL_RSCLKCFG_OSCSRC_MOSC;
  SYSCTL_RSCLKCFG_R = (SYSCTL_RSCLKCFG_R&~SYSCTL_RSCLKCFG_PLLSRC_M)+SYSCTL_RSCLKCFG_PLLSRC_MOSC;

  SYSCTL_DSCLKCFG_R = (SYSCTL_DSCLKCFG_R&~SYSCTL_DSCLKCFG_DSOSCSRC_M)+SYSCTL_DSCLKCFG_DSOSCSRC_MOSC;

  // ---------------- CHANGED ----------------
  // Original code used:
  // Q = 0, N = 4, MINT = 96, MFRAC = 0
  // That gives:
  // fVCO = (25 MHz / (0+1) / (4+1)) * 96 = 480 MHz
  //
  // For 26 MHz exactly, 480 MHz is not enough because 480/(PSYSDIV+1)
  // cannot equal 26 MHz for any integer PSYSDIV.
  //
  // So I changed VCO to 520 MHz:
  // fVCO = (25 MHz / 1 / 5) * 104 = 520 MHz
  //
  // Then with PSYSDIV = 19 in PLL.h:
  // SysClk = 520 MHz / (19 + 1) = 26 MHz
  // ---------------- END CHANGED ----------------
	
#define FXTAL 25000000
#define Q            0
#define N            4
#define MINT        104   // EDIT: was 96, now 104 for 520 MHz VCO
#define MFRAC        0

  // ---------------- EDIT MADE ----------------
  // This macro now evaluates to 26 MHz with the new settings.
  // -------------------------------------------
	
#define SYSCLK (FXTAL/(Q+1)/(N+1))*(MINT+MFRAC/1024)/(PSYSDIV+1)

  SYSCTL_PLLFREQ0_R = (SYSCTL_PLLFREQ0_R&~SYSCTL_PLLFREQ0_MFRAC_M)+(MFRAC<<SYSCTL_PLLFREQ0_MFRAC_S) |
                      (SYSCTL_PLLFREQ0_R&~SYSCTL_PLLFREQ0_MINT_M)+(MINT<<SYSCTL_PLLFREQ0_MINT_S);

  SYSCTL_PLLFREQ1_R = (SYSCTL_PLLFREQ1_R&~SYSCTL_PLLFREQ1_Q_M)+(Q<<SYSCTL_PLLFREQ1_Q_S) |
                      (SYSCTL_PLLFREQ1_R&~SYSCTL_PLLFREQ1_N_M)+(N<<SYSCTL_PLLFREQ1_N_S);

  SYSCTL_PLLFREQ0_R |= SYSCTL_PLLFREQ0_PLLPWR;
  SYSCTL_RSCLKCFG_R |= SYSCTL_RSCLKCFG_NEWFREQ;

  if(SYSCLK < 16000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x0<<22) + (0x0<<21) + (0x0<<16) + (0x0<<6) + (0x0<<5) + (0x0);
  } else if(SYSCLK == 16000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x0<<22) + (0x1<<21) + (0x0<<16) + (0x0<<6) + (0x1<<5) + (0x0);
  } else if(SYSCLK <= 40000000){
    // 26 MHz falls into this case
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x2<<22) + (0x0<<21) + (0x1<<16) + (0x2<<6) + (0x0<<5) + (0x1);
  } else if(SYSCLK <= 60000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x3<<22) + (0x0<<21) + (0x2<<16) + (0x3<<6) + (0x0<<5) + (0x2);
  } else if(SYSCLK <= 80000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x4<<22) + (0x0<<21) + (0x3<<16) + (0x4<<6) + (0x0<<5) + (0x3);
  } else if(SYSCLK <= 100000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x5<<22) + (0x0<<21) + (0x4<<16) + (0x5<<6) + (0x0<<5) + (0x4);
  } else if(SYSCLK <= 120000000){
    SYSCTL_MEMTIM0_R = (SYSCTL_MEMTIM0_R&~0x03EF03EF) + (0x6<<22) + (0x0<<21) + (0x5<<16) + (0x6<<6) + (0x0<<5) + (0x5);
  } else{
    return;
  }

  timeout = 0;
  while(((SYSCTL_PLLSTAT_R&SYSCTL_PLLSTAT_LOCK) == 0) && (timeout < 0xFFFF)){
    timeout = timeout + 1;
  }
  if(timeout == 0xFFFF){
    return;
  }

  SYSCTL_RSCLKCFG_R = (SYSCTL_RSCLKCFG_R&~SYSCTL_RSCLKCFG_PSYSDIV_M)+(PSYSDIV&SYSCTL_RSCLKCFG_PSYSDIV_M) |
                       SYSCTL_RSCLKCFG_MEMTIMU |
                       SYSCTL_RSCLKCFG_USEPLL;
}