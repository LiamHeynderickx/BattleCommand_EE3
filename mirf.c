// Port of the MIRF Library for PIC18f57Q43
// Yusuf Hussein (yusufyy2004@hotmail.com)
// can work for other PIC microcontrollers

#include <string.h>

#include "mcc_generated_files/system/system.h"
#include "mirf.h"
#include <stdio.h>

#define TAG "NRF24"

// Set up the CSN and CE pins in MCC and replace the functions if necessary
// Same with the SPI

// GPIO PINS
#define NRF24L01_CSN_H()            nRF24_CSN_SetHigh()
#define NRF24L01_CSN_L()            nRF24_CSN_SetLow()
#define NRF24L01_CSN_SetOutput()    nRF24_CSN_SetDigitalOutput()

#define NRF24L01_CE_H()             nRF24_CE_SetHigh()
#define NRF24L01_CE_L()             nRF24_CE_SetLow()
#define NRF24L01_CE_SetOutput()     nRF24_CE_SetDigitalOutput()

// SPI Stuff
#define SPI_WRITE_BYTE(dt)          SPI1_ByteExchange(dt)
#define SPI_READ_BYTE(dt)           SPI1_ByteExchange(dt)
#define SPI_INIT()                  SPI1_Open(0)
#define SPI_CLOSE()                 SPI1_Close()
// spi config index 0 should look like this:
// { 0x2, 0xC0, 0x0, 0xff, 0x0 }
// refer to the PIC18f datasheet and the NRF24 datasheet
// register values set the expected spi mode, CKE, SMP, and CKP


// delay
#define DELAY(t)                    DELAY_milliseconds(t)
#define DELAY_US(t)                 DELAY_microseconds(t)

// some defaults
#define DEFAULT_CHANNEL 99
#define DEFAULT_PAYLOAD 32

void SPI_read_buff(unsigned char* buffer, unsigned char bytes) {
    NRF24L01_CSN_L();
    unsigned char i;
    for (i = 0; i < bytes; i++) {
        *buffer = SPI_READ_BYTE(NRF_NOP);
        buffer++;
    }
    NRF24L01_CSN_H();
}

void SPI_write_buff(unsigned char* buffer, unsigned char bytes) {
    NRF24L01_CSN_L();
    unsigned char i;
    for (i = 0; i < bytes; i++) {
        SPI_WRITE_BYTE(*buffer);
        buffer++;
        DELAY_US(10);
    }
    NRF24L01_CSN_H();
}


char rf24_datarates[][8] = {"1Mbps", "2Mbps", "250Kbps"};
const char rf24_crclength[][10] = {"Disabled", "8 bits", "16 bits"};
char rf24_pa_dbm[][8] = {"PA_MIN", "PA_LOW", "PA_HIGH", "PA_MAX"};

void Nrf24_init(NRF24_t * dev)
{    
    SPI_INIT();
    
    dev->channel = DEFAULT_CHANNEL;
    dev->payload = DEFAULT_PAYLOAD;
    
    NRF24L01_CSN_SetOutput();
    NRF24L01_CE_SetOutput();
}

void Nrf24_deinit(NRF24_t *dev) {
	memset(dev, 0, sizeof(NRF24_t));
	SPI_CLOSE();
}

// Sets the important registers in the MiRF module and powers the module
// in receiving mode
// NB: channel and payload must be set now.
void Nrf24_config(NRF24_t * dev, uint8_t channel, uint8_t payload)
{
	dev->channel = channel;
	dev->payload = payload;
    Nrf24_getStatus(dev); // necessary or channel doesn't get updated on next line
    Nrf24_configRegister(RF_CH, channel); // Set RF channel
	Nrf24_configRegister(RX_PW_P0, dev->payload); // Set length of incoming payload
	Nrf24_configRegister(RX_PW_P1, dev->payload);
	Nrf24_powerUpRx(dev); // Start receiver
	Nrf24_flushRx(dev);
}

// Sets the receiving device address
int Nrf24_setRADDR(NRF24_t * dev, uint8_t * adr)
{
	int ret = NRF_OK;
	Nrf24_writeRegister(RX_ADDR_P1, adr, mirf_ADDR_LEN);
	uint8_t buffer[5];
	Nrf24_readRegister(RX_ADDR_P1, buffer, sizeof(buffer));
	for (int i=0;i<5;i++) {
		if (adr[i] != buffer[i]) ret = NRF_FAIL;
	}
	return ret;
}

// Sets the transmitting device  address
int Nrf24_setTADDR(NRF24_t * dev, uint8_t * adr)
{
	int ret = NRF_OK;
	Nrf24_writeRegister(RX_ADDR_P0, adr, mirf_ADDR_LEN); //RX_ADDR_P0 must be set to the TX addr for auto ack to work.
	Nrf24_writeRegister(TX_ADDR, adr, mirf_ADDR_LEN);
	uint8_t buffer[5];
	Nrf24_readRegister(RX_ADDR_P0, buffer, sizeof(buffer));
	for (int i=0;i<5;i++) {
		if (adr[i] != buffer[i]) ret = NRF_FAIL;
	}
	return ret;
}

// Add the receiving device address
void Nrf24_addRADDR(NRF24_t * dev, uint8_t pipe, uint8_t adr)
{
	uint8_t value;
	Nrf24_readRegister(EN_RXADDR, &value, 1);

	if (pipe == 2) {
		Nrf24_configRegister(RX_PW_P2, dev->payload);
		Nrf24_configRegister(RX_ADDR_P2, adr);
		value = value | 0x04;
		Nrf24_configRegister(EN_RXADDR, value);
	} else if (pipe == 3) {
		Nrf24_configRegister(RX_PW_P3, dev->payload);
		Nrf24_configRegister(RX_ADDR_P3, adr);
		value = value | 0x08;
		Nrf24_configRegister(EN_RXADDR, value);
	} else if (pipe == 4) {
		Nrf24_configRegister(RX_PW_P4, dev->payload);
		Nrf24_configRegister(RX_ADDR_P4, adr);
		value = value | 0x10;
		Nrf24_configRegister(EN_RXADDR, value);
	} else if (pipe == 5) {
		Nrf24_configRegister(RX_PW_P5, dev->payload);
		Nrf24_configRegister(RX_ADDR_P5, adr);
		value = value | 0x20;
		Nrf24_configRegister(EN_RXADDR, value);
	}
}

// Checks if data is available for reading
extern bool Nrf24_dataReady(NRF24_t * dev)
{
	uint8_t status = Nrf24_getStatus(dev);
	if ( status & (1 << RX_DR) ) {
		return 1;
	}
	return 0;
}

// Get pipe number for reading
uint8_t Nrf24_getDataPipe(NRF24_t * dev) {
	return ((Nrf24_getStatus(dev) & 0x0E) >> 1);
}

extern bool Nrf24_rxFifoEmpty(NRF24_t * dev)
{
	uint8_t fifoStatus;
	Nrf24_readRegister(FIFO_STATUS, &fifoStatus, sizeof(fifoStatus));
	return (fifoStatus & (1 << RX_EMPTY));
}

// Reads payload bytes into data array
extern void Nrf24_getData(NRF24_t * dev, uint8_t * data)
{
	NRF24L01_CSN_L(); // Pull down chip select
	SPI_WRITE_BYTE( R_REGISTER | R_RX_PAYLOAD ); // Send cmd to read rx payload
	SPI_read_buff( data, dev->payload ); // Read payload
	NRF24L01_CSN_H(); // Pull up chip select
	// NVI: per product spec, p 67, note c:
	// "The RX_DR IRQ is asserted by a new packet arrival event. The procedure
	// for handling this interrupt should be: 1) read payload through SPI,
	// 2) clear RX_DR IRQ, 3) read FIFO_STATUS to check if there are more
	// payloads available in RX FIFO, 4) if there are more data in RX FIFO,
	// repeat from step 1)."
	// So if we're going to clear RX_DR here, we need to check the RX FIFO
	// in the dataReady() function
	Nrf24_configRegister(NRF_STATUS, (1 << RX_DR)); // Reset status register
}

// Clocks only one byte into the given MiRF register
void Nrf24_configRegister(uint8_t reg, uint8_t value)
{
	NRF24L01_CSN_L();
	SPI_WRITE_BYTE(W_REGISTER | (REGISTER_MASK & reg));
	SPI_WRITE_BYTE(value);
	NRF24L01_CSN_H();
}

// Reads an array of bytes from the given start position in the MiRF registers
void Nrf24_readRegister(uint8_t reg, uint8_t * value, uint8_t len)
{
    if(len) {
        NRF24L01_CSN_L();
        SPI_WRITE_BYTE(R_REGISTER | (REGISTER_MASK & reg));
        SPI_read_buff(value, len);
        NRF24L01_CSN_H();
    }
}

// Writes an array of bytes into the MiRF registers
void Nrf24_writeRegister(uint8_t reg, uint8_t * value, uint8_t len)
{   
    if(len) {
        NRF24L01_CSN_L();
        SPI_WRITE_BYTE(W_REGISTER | (REGISTER_MASK & reg));
        SPI_write_buff(value, len);
        NRF24L01_CSN_H();
    }
}

// Sends a data package to the default address. Be sure to send the correct
// amount of bytes as configured as payload on the receiver.
void Nrf24_send(NRF24_t * dev, uint8_t * value)
{
	uint8_t status;
    status = Nrf24_getStatus(dev);
	while (dev->PTX) // Wait until last packet is sent
	{
		status = Nrf24_getStatus(dev);
		if ((status & ((1 << TX_DS)  | (1 << MAX_RT))))
		{
			dev->PTX = 0;
			break;
		}
	}
	NRF24L01_CE_L();
	Nrf24_powerUpTx(dev); // Set to transmitter mode , Power up
	NRF24L01_CSN_L(); // Pull down chip select
	SPI_WRITE_BYTE( FLUSH_TX ); // Write cmd to flush tx fifo
	NRF24L01_CSN_H(); // Pull up chip select
	NRF24L01_CSN_L(); // Pull down chip select
	SPI_WRITE_BYTE( W_TX_PAYLOAD ); // Write cmd to write payload
	SPI_write_buff(value, dev->payload); // Write payload
	NRF24L01_CSN_H(); // Pull up chip select
	NRF24L01_CE_H(); // Start transmission
}

// Test if chip is still sending.
// When sending has finished return chip to listening.
bool Nrf24_isSending(NRF24_t * dev) {
	uint8_t status;
	if (dev->PTX)
	{
		status = Nrf24_getStatus(dev);
		if ((status & ((1 << TX_DS)  | (1 << MAX_RT)))) {// if sending successful (TX_DS) or max retries exceeded (MAX_RT).
			Nrf24_powerUpRx(dev);
			return false;
		}
		return true;
	}
	return false;
}

// Test if Sending has finished or retry is over.
// When sending has finished return true.
// When reach maximum number of TX retries return false.
bool Nrf24_isSend(NRF24_t * dev, int timeout) {
	uint8_t status;
	if (dev->PTX) {
		for(unsigned long int i = 0; i < timeout; i++) {
			status = Nrf24_getStatus(dev);

			if (status & (1 << TX_DS)) { // Data Sent TX FIFO interrupt
				Nrf24_powerUpRx(dev);
				return true;
			}

			if (status & (1 << MAX_RT)) { // Maximum number of TX retries interrupt
				Nrf24_powerUpRx(dev);
				return false;
			}

			DELAY(1);
		}
	}
	return false;
}


uint8_t Nrf24_getStatus(NRF24_t * dev) {
	uint8_t rv;
	Nrf24_readRegister(NRF_STATUS, &rv, 1);
	return rv;
}

void Nrf24_powerUpRx(NRF24_t * dev) {
	dev->PTX = 0;
	NRF24L01_CE_L();
	Nrf24_configRegister(CONFIG, mirf_CONFIG | ( (1 << PWR_UP) | (1 << PRIM_RX) ) ); //set device as RX mode
	NRF24L01_CE_H();
	Nrf24_configRegister(NRF_STATUS, (1 << TX_DS) | (1 << MAX_RT)); //Clear seeded interrupt and max tx number interrupt
}

void Nrf24_flushRx(NRF24_t * dev)
{
	NRF24L01_CSN_L();
	SPI_WRITE_BYTE( FLUSH_RX );
	NRF24L01_CSN_H();
}

void Nrf24_powerUpTx(NRF24_t * dev) {
	dev->PTX = 1;
	Nrf24_configRegister(CONFIG, mirf_CONFIG | ( (1 << PWR_UP) | (0 << PRIM_RX) ) ); //set device as TX mode
	Nrf24_configRegister(NRF_STATUS, (1 << TX_DS) | (1 << MAX_RT)); //Clear seeded interrupt and max tx number interrupt
}

void Nrf24_powerDown(NRF24_t * dev)
{
	NRF24L01_CE_L();
	Nrf24_configRegister(CONFIG, mirf_CONFIG );
}

//Set tx power : 0=-18dBm,1=-12dBm,2=-6dBm,3=0dBm
void Nrf24_SetOutputRF_PWR(NRF24_t * dev, uint8_t val)
{
	if (val > 3) return;

	uint8_t value;
	Nrf24_readRegister(RF_SETUP, &value, 1);
	value = value & 0xF9;
	value = value | (val<< RF_PWR);
	Nrf24_configRegister(RF_SETUP,	value);
}

//Select between the high speed data rates:0=1Mbps, 1=2Mbps, 2=250Kbps
void Nrf24_SetSpeedDataRates(NRF24_t * dev, uint8_t val)
{
	if (val > 2) return;

	uint8_t value;
	Nrf24_readRegister(RF_SETUP, &value, 1);
	if(val == 2)
	{
		value = value | 0x20;
		value = value & 0xF7;
    	Nrf24_configRegister(RF_SETUP,	value);
	}
	else
	{
		value = value & 0xD7;
		value = value | (val << RF_DR_HIGH);
		Nrf24_configRegister(RF_SETUP,	value);
	}
} 

//Set Auto Retransmit Delay 0=250us, 1=500us, ... 15=4000us
void Nrf24_setRetransmitDelay(NRF24_t * dev, uint8_t val)
{
	uint8_t value;
	Nrf24_readRegister(SETUP_RETR, &value, 1);
	value = value & 0x0F;
	value = value | (val << ARD);
	Nrf24_configRegister(SETUP_RETR, value);
}


void Nrf24_printDetails(NRF24_t * dev)
{
	printf("================ NRF Configuration ================\n");

	Nrf24_print_status(Nrf24_getStatus(dev));

	Nrf24_print_address_register(dev, "RX_ADDR_P0-1", RX_ADDR_P0, 2);
	Nrf24_print_byte_register(dev, "RX_ADDR_P2-5", RX_ADDR_P2, 4);
	Nrf24_print_address_register(dev, "TX_ADDR\t", TX_ADDR, 1);

	Nrf24_print_byte_register(dev, "RX_PW_P0-6", RX_PW_P0, 6);
	Nrf24_print_byte_register(dev, "EN_AA\t", EN_AA, 1);
	Nrf24_print_byte_register(dev, "EN_RXADDR", EN_RXADDR, 1);
	Nrf24_print_byte_register(dev, "RF_CH\t", RF_CH, 1);
	Nrf24_print_byte_register(dev, "RF_SETUP", RF_SETUP, 1);
	Nrf24_print_byte_register(dev, "CONFIG\t", CONFIG, 1);
	Nrf24_print_byte_register(dev, "DYNPD/FEATURE", DYNPD, 2);
	printf("Data Rate\t = %s\n",rf24_datarates[Nrf24_getDataRate(dev)]);
	printf("CRC Length\t = %s\n", rf24_crclength[Nrf24_getCRCLength(dev)]);
	printf("PA Power\t = %s\n", rf24_pa_dbm[Nrf24_getPALevel(dev)]);
	uint8_t retransmit = Nrf24_getRetransmitDelay(dev);
	int16_t delay = (retransmit+1)*250;
	printf("Retransmit\t = %d us\n", delay);
}

#define _BV(x) (1<<(x))

void Nrf24_print_status(uint8_t status)
{
	printf("STATUS\t\t = 0x%02x RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\r\n", status, (status & _BV(RX_DR)) ? 1 : 0,
			(status & _BV(TX_DS)) ? 1 : 0, (status & _BV(MAX_RT)) ? 1 : 0, ((status >> RX_P_NO) & 0x07), (status & _BV(TX_FULL)) ? 1 : 0);
}

void Nrf24_print_address_register(NRF24_t * dev, const char* name, uint8_t reg, uint8_t qty)
{
	printf("%s\t =",name);
	while (qty--) {
		uint8_t buffer[5];
		Nrf24_readRegister(reg++, buffer, sizeof(buffer));

		printf(" 0x");
        
		for(int i=0;i<5;i++) {
			printf("%02x", buffer[i]);
		}
	}
	printf("\r\n");
}

void Nrf24_print_byte_register(NRF24_t * dev, const char* name, uint8_t reg, uint8_t qty)
{
	printf("%s\t =", name);
	while (qty--) {
		uint8_t buffer[1];
		Nrf24_readRegister(reg++, buffer, 1);
		printf(" 0x%02x", buffer[0]);
	}
	printf("\r\n");
}

uint8_t Nrf24_getDataRate(NRF24_t * dev)
{
	rf24_datarate_e result;
	uint8_t dr;
	Nrf24_readRegister(RF_SETUP, &dr, sizeof(dr));
	dr = dr & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

	// Order matters in our case below
	if (dr == _BV(RF_DR_LOW)) {
		// '10' = 250KBPS
		result = RF24_250KBPS;
	} else if (dr == _BV(RF_DR_HIGH)) {
		// '01' = 2MBPS
		result = RF24_2MBPS;
	} else {
		// '00' = 1MBPS
		result = RF24_1MBPS;
	}
	return result;
}

char * Nrf24_getDataRateString(NRF24_t * dev)
{
	return rf24_datarates[Nrf24_getDataRate(dev)];
}

uint8_t Nrf24_getCRCLength(NRF24_t * dev)
{
	rf24_crclength_e result = RF24_CRC_DISABLED;

	uint8_t config;
	Nrf24_readRegister(CONFIG, &config, sizeof(config));
	config = config & (_BV(CRCO) | _BV(EN_CRC));
	uint8_t AA;
	Nrf24_readRegister(EN_AA, &AA, sizeof(AA));

	if (config & _BV(EN_CRC) || AA) {
		if (config & _BV(CRCO)) {
			result = RF24_CRC_16;
		} else {
			result = RF24_CRC_8;
		}
	}

	return result;
}

uint8_t Nrf24_getPALevel(NRF24_t * dev)
{
	uint8_t level;
	Nrf24_readRegister(RF_SETUP, &level, sizeof(level));
	level = (level & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH))) >> 1;
	return (level);
}

char * Nrf24_getPALevelString(NRF24_t * dev)
{
	return rf24_pa_dbm[Nrf24_getPALevel(dev)];
}

uint8_t Nrf24_getRetransmitDelay(NRF24_t * dev)
{
	uint8_t value;
	Nrf24_readRegister(SETUP_RETR, &value, 1);
	return (value >> 4);
}

uint8_t Nrf24_getChannel(NRF24_t * dev)
{
	return dev->channel;
}

uint8_t Nrf24_getPayload(NRF24_t * dev)
{
	return dev->payload;
}
