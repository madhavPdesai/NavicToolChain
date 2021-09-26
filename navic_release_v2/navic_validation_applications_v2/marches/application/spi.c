#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include "ajit_access_routines.h"

void writeSpiDataLowRegister(uint32_t I)
{
	*((uint32_t*) ADDR_SPI_DATA_REGISTER_LOW) = I;
}

void writeSpiCommandRegister(uint32_t cmd)
{
	*((uint32_t*) ADDR_SPI_COMMAND_STATUS_REGISTER) = cmd;
}

uint32_t readSpiStatusRegister()
{
	uint32_t rval = *((uint32_t*) ADDR_SPI_COMMAND_STATUS_REGISTER);
	return(rval);
}

