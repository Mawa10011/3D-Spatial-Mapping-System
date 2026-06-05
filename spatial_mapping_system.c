// Deliverable #2 
// Mawa Hassan
// Final Code

#include <stdint.h>
#include <stdio.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

#define I2C_MCS_ACK             0x00000008
#define I2C_MCS_DATACK          0x00000008
#define I2C_MCS_ADRACK          0x00000004
#define I2C_MCS_STOP            0x00000004
#define I2C_MCS_START           0x00000002
#define I2C_MCS_ERROR           0x00000002
#define I2C_MCS_RUN             0x00000001
#define I2C_MCS_BUSY            0x00000001
#define I2C_MCR_MFE             0x00000010

#define MAXRETRIES              5
#define NUM_POINTS              32
#define MAX_SCANS               8


// MY ASSIGNED LEDS
// Measurement Status = PN0
// UART Tx            = PN1
// Additional Status  = PF4


// Port H for motor --------------------
void PortH_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R7) == 0){};
    GPIO_PORTH_DIR_R |= 0x0F;
    GPIO_PORTH_AFSEL_R &= ~0x0F;
    GPIO_PORTH_DEN_R |= 0x0F;
    GPIO_PORTH_AMSEL_R &= ~0x0F;
}

// Port J for buttons -------------------
// PJ0 = send button, PJ1 = scan button
void PortJ_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0){};

    GPIO_PORTJ_DIR_R &= ~0x03;    // PJ0, PJ1 inputs
    GPIO_PORTJ_DEN_R |= 0x03;
    GPIO_PORTJ_AFSEL_R &= ~0x03;
    GPIO_PORTJ_AMSEL_R &= ~0x03;
    GPIO_PORTJ_PUR_R |= 0x03;     // pull-ups for onboard buttons
}

// PM4 output for clock demo -------------
void PortM4_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0){};

    GPIO_PORTM_DIR_R |= 0x10;     // PM4 output
    GPIO_PORTM_DEN_R |= 0x10;
    GPIO_PORTM_AFSEL_R &= ~0x10;
    GPIO_PORTM_AMSEL_R &= ~0x10;
}

void ClockDemo_PM4(void){
    while(1){
        GPIO_PORTM_DATA_R ^= 0x10;   // toggle PM4
        SysTick_Wait10ms(1);         // wait 10 ms
    }
}

// Assigned LED port init ------------------
void PortN_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){};

    GPIO_PORTN_DIR_R |= 0x03;      // PN0, PN1 outputs
    GPIO_PORTN_DEN_R |= 0x03;
    GPIO_PORTN_AFSEL_R &= ~0x03;
    GPIO_PORTN_AMSEL_R &= ~0x03;
}

void PortF_LED_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0){};

    GPIO_PORTF_DIR_R |= 0x10;      // PF4 output
    GPIO_PORTF_DEN_R |= 0x10;
    GPIO_PORTF_AFSEL_R &= ~0x10;
    GPIO_PORTF_AMSEL_R &= ~0x10;
}

// LED functions --------------------
// PN0 = measurement status
void MeasurementLED_Flash(void){
    GPIO_PORTN_DATA_R |= 0x01;     // PN0 on
    SysTick_Wait10ms(1);
    GPIO_PORTN_DATA_R &= ~0x01;    // PN0 off
}

// PN1 = UART transmission status
void UARTLED_Flash(void){
    GPIO_PORTN_DATA_R |= 0x02;     // PN1 on
    SysTick_Wait10ms(2);
    GPIO_PORTN_DATA_R &= ~0x02;    // PN1 off
}

// PF4 = additional status (for when a scan is in progress)
void ScanStatus_On(void){
    GPIO_PORTF_DATA_R |= 0x10;     // PF4 on
}
void ScanStatus_Off(void){
    GPIO_PORTF_DATA_R &= ~0x10;    // PF4 off
}

// Stepper motor --------------------
void Stepper_fullstep(void){
    uint32_t delay = 1;

    GPIO_PORTH_DATA_R = 0b00000011;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00000110;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00001100;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00001001;
    SysTick_Wait10ms(delay);
}

// reverse direction
void Stepper_fullstep_reverse(void){
    uint32_t delay = 1;

    GPIO_PORTH_DATA_R = 0b00001001;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00001100;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00000110;
    SysTick_Wait10ms(delay);

    GPIO_PORTH_DATA_R = 0b00000011;
    SysTick_Wait10ms(delay);
}

// 11.25 deg = 16 full-step loops
void Stepper_11_25deg(void){
    for(int i = 0; i < 16; i++){
        Stepper_fullstep();
    }
}

void Stepper_11_25deg_reverse(void){
    for(int i = 0; i < 16; i++){
        Stepper_fullstep_reverse();
    }
}

// full 360 deg return with NO measurements
void ReturnFullRotationHome(void){
    for(int i = 0; i < NUM_POINTS; i++){   // 32 * 11.25 = 360 deg
        Stepper_11_25deg_reverse();
        SysTick_Wait10ms(5);
    }

    GPIO_PORTH_DATA_R = 0x00;   // optional: turn motor outputs off after return
}

// -------------------- I2C --------------------
// so the mcu can talk to the sensor over i2c
void I2C_Init(void){ 
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;
    while((SYSCTL_PRGPIO_R & 0x0002) == 0){};

    GPIO_PORTB_AFSEL_R |= 0x0C;
    GPIO_PORTB_ODR_R |= 0x08;
    GPIO_PORTB_DEN_R |= 0x0C;
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) + 0x00002200;

    I2C0_MCR_R = I2C_MCR_MFE;

    // adjusted for 26 MHz bus clock
    // gives about 100 kHz I2C speed
    I2C0_MTPR_R = 12;
    // use this value to divide the system clock so i2c runs at ~100kHz
    // i2c speed = system clock / (20 * (TPR+1))
    // so by plugging in 12
    // speed = 26,000,000 / (20*13) = 100,000 Hz
}

void PortG_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R6) == 0){};
    GPIO_PORTG_DIR_R &= 0x00;
    GPIO_PORTG_AFSEL_R &= ~0x01;
    GPIO_PORTG_DEN_R |= 0x01;
    GPIO_PORTG_AMSEL_R &= ~0x01;
}

// Reset the ToF sensor using the XSHUT pin (active LOW)
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;        // Set PG0 as OUTPUT
    GPIO_PORTG_DATA_R &= 0b11111110; // Drive PG0 LOW
    FlashAllLEDs();                  
    SysTick_Wait10ms(10);            
    GPIO_PORTG_DIR_R &= ~0x01;       // Release line
}

// global
uint16_t dev = 0x29;    
int status = 0;         

int angle_x100[MAX_SCANS][NUM_POINTS];
uint16_t distance_mm[MAX_SCANS][NUM_POINTS];
int scanCount = 0;


// button functions
// ACTIVE-LOW buttons with pull-up resistors:
// not pressed = 1
// pressed     = 0

// PJ1 = scan button function
int ScanButtonPressed(void){
    if((GPIO_PORTJ_DATA_R & 0x02) == 0){
        SysTick_Wait10ms(2);                 // debounce
        if((GPIO_PORTJ_DATA_R & 0x02) == 0){
            while((GPIO_PORTJ_DATA_R & 0x02) == 0){}   // wait for release
            return 1;
        }
    }
    return 0;
}

// PJ0 = send button function
int SendButtonPressed(void){
    if((GPIO_PORTJ_DATA_R & 0x01) == 0){
        SysTick_Wait10ms(2);                 // debounce
        if((GPIO_PORTJ_DATA_R & 0x01) == 0){
            while((GPIO_PORTJ_DATA_R & 0x01) == 0){}   // wait for release
            return 1;
        }
    }
    return 0;
}

// scan/store one scan function
void DoOneScanAndStore(void){
    uint16_t Distance;
    uint8_t RangeStatus;
    uint8_t dataReady = 0;

    if(scanCount >= MAX_SCANS){
        UART_printf("MAXSCANS\r\n");
        return;
    }

    sprintf(printf_buffer, "Starting scan %d / %d\r\n", scanCount + 1, MAX_SCANS);
    UART_printf(printf_buffer);

    // additional LED on to show a scan is in progress
    ScanStatus_On();

    for(int i = 0; i < NUM_POINTS; i++) 
		{
        // wait until ToF sensor says new data is ready
        dataReady = 0;
        while(dataReady == 0){
            status = VL53L1X_CheckForDataReady(dev, &dataReady);
            VL53L1_WaitMs(dev, 5);
        }

        // read sensor
        status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
        status = VL53L1X_GetDistance(dev, &Distance);
        status = VL53L1X_ClearInterrupt(dev);

        // store angle and distance
        angle_x100[scanCount][i] = i * 1125;
        distance_mm[scanCount][i] = Distance;

        // flash measurement LED
        MeasurementLED_Flash();

        // rotate forward between measurement points
        if(i < NUM_POINTS - 1){
            Stepper_11_25deg();
            SysTick_Wait10ms(20);
        }
    }

    // move the final 11.25 deg to complete the full forward rotation
    Stepper_11_25deg();
    SysTick_Wait10ms(20);
    scanCount++;
    sprintf(printf_buffer, "Stored scan %d / %d\r\n", scanCount, MAX_SCANS);
    UART_printf(printf_buffer);

    // move back to the starting position
    ReturnFullRotationHome();

    // additional status LED OFF when scan is done
    ScanStatus_Off();
}

// send all stored scans
void SendAllScans(void){
    if(scanCount == 0){
        UART_printf("NOSCANS\r\n");
        return;
    }

    UARTLED_Flash();

    for(int s = 0; s < scanCount; s++){
        sprintf(printf_buffer, "SCAN %d\r\n", s + 1);
        UART_printf(printf_buffer);

        for(int i = 0; i < NUM_POINTS; i++){
            sprintf(printf_buffer, "%d.%02d,%u\r\n",
                    angle_x100[s][i] / 100,
                    angle_x100[s][i] % 100,
                    distance_mm[s][i]);
            UART_printf(printf_buffer);
        }

        UART_printf("ENDSCAN\r\n");
    }

    UART_printf("ENDALL\r\n");
}

// ----------------------  MAIN PROGRAM  ------------------------
int main(void) 
	{
		// initialize everything
    uint8_t sensorState = 0;
    uint16_t wordData;
    PLL_Init();
    SysTick_Init();
    onboardLEDs_Init();
    I2C_Init();
    UART_Init();
    PortH_Init();
    PortJ_Init();
    PortM4_Init();
    PortN_Init();
    PortF_LED_Init();

    // uncomment for ad3 demo on pm4
    // ClockDemo_PM4();

    // start with assigned LEDs off
    GPIO_PORTN_DATA_R &= ~0x03;
    GPIO_PORTF_DATA_R &= ~0x10;

    status = VL53L1X_GetSensorId(dev, &wordData);

    while(sensorState == 0){
        status = VL53L1X_BootState(dev, &sensorState);
        SysTick_Wait10ms(10);
    }
    FlashAllLEDs();
    status = VL53L1X_ClearInterrupt(dev);
    status = VL53L1X_SensorInit(dev);
    Status_Check("SensorInit", status);
    // Start ranging ONCE and keep it running
    status = VL53L1X_StartRanging(dev);
    while(1)
			{
        if(ScanButtonPressed()){
            DoOneScanAndStore();
        }

        if(SendButtonPressed()){
            SendAllScans();
        }
    }
}