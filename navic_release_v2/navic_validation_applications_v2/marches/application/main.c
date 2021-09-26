//
// An application which initializes the coprocessor and rf resampler
//
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include "ajit_access_routines.h"

#define OFFSET 3
#define NWORDS   256
#define CPNWORDS 256
#define RFNWORDS  64

int runMarch(uint32_t* fptr, char*  tid, int L)
{
	int err = 0;

	// read and write some locations from the following
	// areas.
	//	flash 
	// 		(write bytes from address 0x0 to 0xff and read back)
	int  v;
	for(v = 0; v < L; v++)
	{
		*(fptr + v) = v + OFFSET;
	}
	for(v = 0; v < L; v++)
	{
		uint32_t rb = *(fptr + v);
		if(rb != (v + OFFSET))
		{
			ee_printf("Error: in march test for %s (fptr=0x%x, got 0x%x, expected 0x%x)\n", tid, 
									(uint32_t) fptr, rb, v + OFFSET);
			err = 1;
		}	
#ifdef DEBUGPRINT
		else
		{
			ee_printf("Info: in march test for %s (fptr=0x%x, got 0x%x, expected 0x%x)\n", tid, 
									(uint32_t) fptr, rb, v + OFFSET);
		}
#endif
	}
	ee_printf("Info: finished march test for %s.\n", tid);

	return(err);
	
}

void writeSpiDataLowRegister(uint32_t I);
void writeSpiCommandRegister(uint32_t cmd);
uint32_t readSpiStatusRegister();

int checkSpi(uint8_t dev_id)
{
	int _err_flag_ = 0;
	uint8_t I;
	for(I = 0; I < 8; I++)
	{
		writeSpiDataLowRegister(I);

		uint8_t cmd = (dev_id << 4) | 3;	
		ee_printf("checkSpi: addr=0x%x cmd = 0x%x\n", ADDR_SPI_COMMAND_STATUS_REGISTER, cmd);

		writeSpiCommandRegister(cmd);
		
		int spin_count = 0;
		while(1)
		// spin on spi command completion.
		{
			uint32_t status = readSpiStatusRegister();
			if(!(status & 0x1))
				break;

			spin_count++;
			if(spin_count == (2*4096))
			{
				ee_printf("Error: spi device-id %d timed out (status=0x%x, spin-count=%d)\n",
							dev_id, status, spin_count);
				break;
			}
		}
		uint32_t val = *((uint32_t*) ADDR_SPI_DATA_REGISTER_LOW);

		if((val & 0xff) != ((~I) & 0xff))
		{
			_err_flag_ = 1;
			ee_printf("Error: spi device-id %d expected 0x%x, observed 0x%x (spin-count=%d)\n",
					dev_id, (~I) & 0xff, val, spin_count);
		}
		else
		{
			ee_printf("Info: spi device-id %d expected 0x%x, observed 0x%x (spin-count=%d)\n",
					dev_id, (~I) ^ 0xff,  val, spin_count);
		}
	}
	return(_err_flag_);
}


	
int runGpioConfigChecks()
{
	int err = 0;

	int I;

	
	uint32_t br = *((uint32_t*) ADDR_CONFIG_UART_BAUD_CONTROL_REGISTER);

	for(I = 0; I < NWORDS; I++)
	{
		*((uint32_t*) ADDR_GPIO_DOUT_REGISTER) = I;
		uint32_t gpio_dout = *((uint32_t*) ADDR_GPIO_DOUT_REGISTER);
		if(gpio_dout != (I & 0xff))
		{
			PRINT ("Error: gpio_dout expected 0x%x, observed 0x%x\n", (I & 0xff), gpio_dout);
			err = 1;
		}

	}

	ee_printf("Info: done gpio config checks (err=%d)\n", err);
	return(err);
}


int runMarchTestsAndSpiChecks()
{
	int err = 0;

	// run GPIO and config checks
	ee_printf("Info: starting runGpioConfigChecks\n");
	err = runGpioConfigChecks();

	// march on flash
	ee_printf("Info: starting flash tests\n");
	err = runMarch(0x0, "flash", NWORDS);

	// march on external memory
	ee_printf("Info: starting ext mem tests\n");
	err = runMarch(0x1000000, "extmem", NWORDS) || err;

	// march on internal memory (in the command region)
	ee_printf("Info: starting int mem tests\n");
	err = runMarch(0x40064000, "intmem", NWORDS) || err;

	// march on cp prn memory.
	uint32_t prn_base_location = P_CP_IO_MAP_BASE_ADDR + 
					(PRN_REGISTERS_ADDR_MIN << 2);
	ee_printf("Info: starting cp mem tests\n");
	err = runMarch(prn_base_location, "cp", CPNWORDS) || err;

	// march on RF cos table 
	uint32_t rf_cos_table_location = P_RF_RESAMPLER_IO_MAP_BASE_ADDR
						+ (RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR  << 2)
						+ (RF_COS_TABLE_BASE_ID << 2);
		
	ee_printf("Info: starting rf mem tests\n");
	err = runMarch(rf_cos_table_location, "rf", RFNWORDS) || err;

	int I;
	for(I =0; I < 8; I++)
	{
		err  = checkSpi(I) || err;
	}

	return(err);
}

void coprocessor_interrupt_handler()
{
	return;
}

int main()
{
	setCpLinkVerboseFlag(0);

	__ajit_write_serial_control_register__ (TX_ENABLE);
 	__ajit_write_irc_control_register__(1);

#ifndef __CPPA__
	ee_printf("Error: physical address of command buffer not defined\n");
	return(1);
#endif



	ee_printf("Info: march tests and spi checks\n");
	int err = runMarchTestsAndSpiChecks();
	
	if(err)
		ee_printf("ERROR :-[ \n");
	else
		ee_printf("SUCCESS!!\n");
	
	return (0);
}

