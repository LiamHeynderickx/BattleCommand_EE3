// ADAPTED MIRF EXAMPLE
// ESPIDF MIRF LIBRARY: https://github.com/nopnop2002/esp-idf-mirf/tree/master

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "mcc_generated_files/system/system.h"

#include "mirf.h"

#define CONFIG_RECEIVER 1
#define CONFIG_SENDER 0
#define CONFIG_RADIO_CHANNEL 99
#define CONFIG_RETRANSMIT_DELAY 100
#define PAYLOAD_SIZE 32
#define ARRAY_SIZE 200
#define CONFIG_ADRES "GAME4"

NRF24_t dev;
uint8_t received_data[ARRAY_SIZE];

int received_index = 0;


void init_nrf(){
    DELAY_milliseconds(100);  // Allow NRF module to stabilize
    
    Nrf24_init(&dev);
	Nrf24_config(&dev, CONFIG_RADIO_CHANNEL, PAYLOAD_SIZE);

	// Set my own address using 5 characters
	int ret = Nrf24_setRADDR(&dev, (uint8_t *) CONFIG_ADRES);
	if (ret != NRF_OK) {
		while(1) { DELAY_milliseconds(1); }
	}
    ret = Nrf24_setTADDR(&dev, (uint8_t *) CONFIG_ADRES);
	if (ret != NRF_OK) {
		while(1) { DELAY_milliseconds(1); }
	}

    Nrf24_SetSpeedDataRates(&dev, 0); //Set speed data rate to 250K
	Nrf24_setRetransmitDelay(&dev, CONFIG_RETRANSMIT_DELAY);

	// Print settings
	Nrf24_printDetails(&dev);
    uint8_t buf[PAYLOAD_SIZE];
    while(1) {
		if (Nrf24_dataReady(&dev) == false) break;
		Nrf24_getData(&dev, buf);
	}
}




#define sendBit(bit) \
    LATFbits.LATF2 = 1; \
    NOP(); NOP(); NOP(); \
    LATFbits.LATF2 = bit; \
    NOP(); NOP(); NOP(); NOP(); \
    LATFbits.LATF2 = 0; \
    NOP(); NOP(); NOP(); NOP();

void sendByte (unsigned char byte) {
    if (byte & 0b10000000) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b01000000) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00100000) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00010000) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00001000) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00000100) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00000010) { sendBit(1); } else { sendBit(0); }
    if (byte & 0b00000001) { sendBit(1); } else { sendBit(0); }   
}

void sendRGB (unsigned char r, unsigned char g, unsigned char b) {
    sendByte(g);
    sendByte(r);
    sendByte(b);
}

void sendCell(uint8_t cell){
    unsigned char bright = 20;
    switch (cell) {
        case 0:
            sendRGB(0, 0, 0);
            break;
        case 1:
            sendRGB(bright, 0, 0);
            break;
        case 2:
            sendRGB(0, bright, 0);
            break;
        
        case 3:
            sendRGB(0, 0, bright);
            break;
         
        case 4:
            sendRGB(bright, bright, bright);
            break;    
        
        case 5:
            sendRGB(bright, 0, bright);
            break;     
        
        case 6:
            sendRGB(bright, bright, 0);
            break; 
        
        case 6:
            sendRGB(0, bright, bright);
            break;     
            
        default:
            sendRGB(0, 0, bright);
            break;
    }
}

void sendGrid(uint8_t grid[ARRAY_SIZE]){
    int gridSize = 10; // Assuming a 10x10 grid based on 100 effective elements
    
//    
//    sendCell(grid[0]);
    for (int row = 0; row < gridSize*2; row++) {
        for (int col = 0; col < gridSize; col++) {
            int index = row * gridSize + col + 1; // +1 to skip the dummy number at the beginning
            if (row % 2 == 1) { // Reverse the order for uneven rows
                index = row * gridSize + (gridSize - 1 - col) + 1;
            }
            sendCell(grid[index]);
        }
    }
}

void nrf_interrupt(){
    uint8_t buf[PAYLOAD_SIZE];
    if (Nrf24_dataReady(&dev)) {
        Nrf24_getData(&dev, buf);

        // Copy received chunk into the main array
        for (int i = 0; i < PAYLOAD_SIZE; i++) {
            if (received_index < ARRAY_SIZE) {
                received_data[received_index++] = buf[i];
            }
        }

        // When the full array is received
        if (received_index >= ARRAY_SIZE) {
            received_index = 0;
            sendGrid(received_data);
        }
    }
}


void main(void)
{
    SYSTEM_Initialize();
    
    uint8_t gridE[ARRAY_SIZE] = { [0 ... (ARRAY_SIZE-1)] = 0 }; 
    uint8_t gridR[ARRAY_SIZE] = { [0 ... (ARRAY_SIZE-1)] = 1 }; 
    uint8_t gridG[ARRAY_SIZE] = { [0 ... (ARRAY_SIZE-1)] = 2 }; 
    uint8_t gridB[ARRAY_SIZE] = { [0 ... (ARRAY_SIZE-1)] = 3 }; 
    
    
    INTERRUPT_GlobalInterruptEnable();
    IOCCN6_Enable();  // Enable positive edge interrupt
    IOCC6_Enable();   // Enable IOC interrupts
    RC6_SetInterruptHandler(nrf_interrupt);
    
    init_nrf();
    
    sendGrid(gridB);
    DELAY_milliseconds(500);
    sendGrid(gridR);
    DELAY_milliseconds(500);
    sendGrid(gridG);
    DELAY_milliseconds(500);
    sendGrid(gridE);
    
	while(1) {
        
	}
}

