#ifndef cp_link_h___
#define cp_link_h___

//
// access internal registers of the CP
// The reg_id is a 20-bit number, and a 32-bit reg-value
// is accessed.  Note that the internal registers
// can be 32-bits wide or 64-bits wide.
//
uint32_t write_to_cp_reg(uint32_t reg_id, uint32_t wval);
uint32_t read_from_cp_reg(uint32_t reg_id);

// Note: reg_id is id of 64-bit register.
uint64_t write_u64_to_cp_reg(uint32_t reg_id, uint64_t wval);
uint64_t read_u64_from_cp_reg(uint32_t reg_id);

void write_u64_block_to_cp_shared_mem (uint32_t start_addr, uint64_t *send_values, uint32_t count);
uint64_t read_u64_from_cp_shared_mem (uint32_t read_addr);

void write_u32_block_to_cp_shared_mem (uint32_t start_addr, uint32_t *send_values, uint32_t count);
uint32_t read_u32_from_cp_shared_mem (uint32_t read_addr);


#endif
