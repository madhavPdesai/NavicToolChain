#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <macros.h>
#include <cp_rf_parameters.h>
#include <data_structures.h>
#include <cp_internals.h>
#include <cp_driver.h>
#include <cp_link.h>

int _verbose_flag = 0;
void setCpLinkVerboseFlag(int v)
{
	_verbose_flag = v;
}

uint32_t write_to_cp_reg(uint32_t reg_id, uint32_t wval)
{
	uint32_t ret_val = 0;
	if(reg_id > RF_ACQ_REGISTERS_ADDR_MAX)
	{
		PRINTF("Error:write_to_cp_reg: out-of-bounds reg-id=%d\n", reg_id);
		return(0);
	}

	// memory mapped I/O
	uint32_t addr = P_CP_IO_MAP_BASE_ADDR + (reg_id << 2);
	*((uint32_t*) addr) = wval;
	ret_val = wval;

	if(_verbose_flag)
	{
		PRINTF("Info: cp_reg[0x%x] = 0x%x\n", reg_id, wval);
	}
	return(ret_val);
}

uint32_t read_from_cp_reg(uint32_t reg_id)
{
	uint32_t ret_val = 0;

	if(reg_id > RF_ACQ_REGISTERS_ADDR_MAX)
	{
		PRINTF("Error:read_from_cp_reg: out-of-bounds reg-id\n");
		return(0);
	}

	 // Memory mapped I/O
	uint32_t addr = P_CP_IO_MAP_BASE_ADDR + (reg_id << 2);
	ret_val = *((uint32_t*)addr);

	if(_verbose_flag)
	{
		PRINTF("Info: 0x%x = cp_reg[0x%x]\n", ret_val, reg_id);
	}
	return(ret_val);
}

uint64_t write_u64_to_cp_reg(uint32_t reg_id, uint64_t wval)
{
	write_to_cp_reg ((reg_id << 1), (wval >> 32) & 0xffffffff);
	write_to_cp_reg ((reg_id << 1) | 0x1, wval & 0xffffffff);
	return(0);
}

uint64_t read_u64_from_cp_reg(uint32_t reg_id)
{
	uint32_t hw = read_from_cp_reg (reg_id << 1);
	uint32_t lw = read_from_cp_reg ((reg_id << 1) | 0x1);

	uint64_t ret_val = hw;
	ret_val = (ret_val << 32) | lw;
	return(ret_val);
}

void write_u64_block_to_cp_shared_mem (uint32_t start_addr, uint64_t *send_values, uint32_t count)
{
	uint32_t I;
	uint64_t* ptr = (uint64_t*) start_addr;
	for(I=0; I < count; I++)
	{
		*(ptr + I) = send_values[I];	
	}
}

uint64_t read_u64_from_cp_shared_mem (uint32_t read_addr)
{
	uint64_t ret_val = 0;

	uint64_t* ptr = (uint64_t*) read_addr;
	ret_val = *ptr;

	return(ret_val);
}

void write_u32_block_to_cp_shared_mem (uint32_t start_addr, uint32_t *send_values, uint32_t count)
{
	uint32_t I;
	uint32_t* ptr = (uint32_t*) start_addr;
	for(I=0; I < count; I++)
	{
		*(ptr + I) = send_values[I];	
	}
}

uint32_t read_u32_from_cp_shared_mem (uint32_t read_addr)
{
	uint32_t ret_val = 0;

	uint32_t* ptr = (uint32_t*) read_addr;
	ret_val = *ptr;

	return(ret_val);
}

