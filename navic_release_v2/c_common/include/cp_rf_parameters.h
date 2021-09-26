#ifndef cp_rf_parameters_h___
#define cp_rf_parameters_h___
#define ACQUISITION_SAMPLES_PER_CODE   	4092
#define TRACKING_SAMPLES_PER_CODE		16368
//
// Give up 256KB for code, 256 kB for data and stack.
//
//
// start the shared data structures at 256+128=384kB
#define P_SHARED_WITH_CP_BASE_ADDR       0x40064000


// we need 32x64=2048 bytes here.
// give up half a 4kB page for this.
#define  P_MAX_NUMBER_OF_COMMANDS      64
#define  P_COMMAND_BUFFER_SIZE 	  64		// each command buffer has 64 bytes.
#define  P_COMMAND_REGION_SIZE 	  (P_MAX_NUMBER_OF_COMMANDS * 64)


//////////////////////////////////////////////////////////////////////////////////
//  MEMORY MAP..
//
// Flash starts at 0x00000000, and we have 16MB
// of it.
//
#define P_FLASH_ROM_BASE_ADDR		0x0
#define P_FLASH_ROM_MAX_ADDR			0x00ffffff

//
// Internal RAM starts at 0x400000000 and consists of 512 KB
//   of data.
//
#define P_RAM_BASE_ADDR			0x40000000
#define P_RAM_MAX_ADDR			0x4007ffff

// 
// External RAM starts at 0x01000000 and extends
// 16MB beyond that.
//
#define P_EXT_RAM_BASE_ADDR			0x01000000
#define P_EXT_RAM_MAX_ADDR			0x01ffffff


//
// Give up 256KB for code, 256 kB for data and stack.
//   Code starts at 0x40000000  (base)
//	 use 256KB
#define P_TEXT_BASE_ADDR		0x40000000

//   Data starts at 0x40040000  (base + 256kB)
//       use 128KB
#define P_DATA_BASE_ADDR		0x40040000

//   Stack starts at 0x4046000  (base + 384KB)
//       use 128KB 
#define P_STACK_BASE_ADDR		0x40060000

// Stack starts at 0x40064000 and grows until 0x40060004
#define P_STACK_SIZE			(128 * 1024) // provisional.

//
// Total memory used = 512KB
//


//////////////////  IO mapping            ///////////////////
#define P_IO_BASE_ADDR			0xfff00000
#define P_IO_MAX_ADDR			0xffffffff

//
//   CP internal memory starts at 0xfff00000
//
#define P_CP_IO_MAP_BASE_ADDR		0xfff00000
#define P_CP_IO_MAP_MAX_ADDR			0xfff3ffff

//   RF-sampler internal memory starts at 0xfff40000
//              (12 KB)
#define P_RF_RESAMPLER_IO_MAP_BASE_ADDR	0xfff40000
#define P_RF_RESAMPLER_IO_MAP_MAX_ADDR	0xfff7ffff

//   
//   Other IO starts at 0xffff0000 (serial)
//  
#define P_TIMER_IRL  	10
#define P_SERIAL_IRL 	12
#define P_EXTERNAL_IRL 	13

//
// IRC
//
#define P_ADDR_INTERRUPT_CONTROLLER_CONTROL_REGISTER 	0xFFFF3000

//
// TIMER
//
#define P_ADDR_TIMER_CONTROL_REGISTER 			0xFFFF3100

// 
// SERIAL
//
#define P_ADDR_SERIAL_CONTROL_REGISTER 		0xFFFF3200
#define P_ADDR_SERIAL_TX_REGISTER      		0xFFFF3210
#define P_ADDR_SERIAL_RX_REGISTER      		0xFFFF3220
#define P_ADDR_SERIAL_BAUD_CONFIG_REGISTER  		0xFFFF3230

//
// integrated SPI-MASTER device 
//    provides 4 registers.
//
#define P_ADDR_SPI_DATA_REGISTER_LOW                   0xFFFF3300
#define P_ADDR_SPI_DATA_REGISTER_HIGH                  0xFFFF3304
#define P_ADDR_SPI_COMMAND_STATUS_REGISTER             0xFFFF3308
#define P_ADDR_SPI_CONFIG_REGISTER                     0xFFFF330c

//
// GPIO and CONFIG
//
#define P_ADDR_GPIO_DATA_OUT_REGISTER                   0xFFFF3400
#define P_ADDR_GPIO_DATA_IN_REGISTER                    0xFFFF3404
#define P_ADDR_CONFIG_UART_BAUD_CONTROL_REGISTER        0xFFFF3408



//
// sine table
//
#define NCO_SINE_SIGN_TABLE_SIZE  (16 * 1024)
#define NCO_HALF_SINE_SIGN_TABLE_SIZE (8 * 1024)

//
// Intervals for two-bit sine NCO.
//#define NCO_SINE_SIGN_TABLE_I0    886
#define NCO_SINE_SIGN_TABLE_I0             (NCO_SINE_SIGN_TABLE_SIZE / 12)

#define NCO_SINE_SIGN_TABLE_MASK  (NCO_SINE_SIGN_TABLE_SIZE - 1)
#define NCO_QUARTER_SINE_SIGN_TABLE_SIZE (4 * 1024)
#define NCO_LOG_SINE_SIGN_TABLE_SIZE 14
#define NCO_PHASE_RIGHT_SHIFT_AMOUNT  (32 - NCO_LOG_SINE_SIGN_TABLE_SIZE)

#define SET_LOCK_FLAG                   0x40


//
//
//
// Addresses of internal registers in the Coprocessor
//
//
//
#define AUX_REGISTERS_ADDR_MIN			0
#define AUX_REGISTERS_ADDR_MAX			15
#define UTILITY_REGISTERS_ADDR_MIN			16
#define UTILITY_REGISTERS_ADDR_MAX			31
#define RF_STATUS_REGISTERS_ADDR_MIN			32 
#define RF_STATUS_REGISTERS_ADDR_MAX			(RF_STATUS_REGISTERS_ADDR_MIN + 5) 
// PRN registers 64 x 1023 bits  = 64 x 16 x 8 bytes.. 8KB
#define PRN_REGISTERS_ADDR_MIN			(RF_STATUS_REGISTERS_ADDR_MAX + 1)
#define PRN_REGISTERS_ADDR_MAX                 	(PRN_REGISTERS_ADDR_MIN + 2047)
//
// ACQ_PRN store: scratch pad used by acquire engine.
//  1023 bits  = 128 bytes or 16 dwords.
//                  
// keeping them separate allows us to do 1.5us/acq correlation or 512 acq
// correlations per ms.
//
#define PRN_ACQ_REGISTERS_ADDR_MIN			(PRN_REGISTERS_ADDR_MAX + 1)
#define PRN_ACQ_REGISTERS_ADDR_MAX                 	(PRN_ACQ_REGISTERS_ADDR_MIN + 31)
//
// RF registers  16x oversample of chip rate
//
#define RF_L1_REGISTERS_ADDR_MIN                 	(PRN_ACQ_REGISTERS_ADDR_MAX + 1)
#define RF_L5_REGISTERS_ADDR_MIN                 	(RF_L1_REGISTERS_ADDR_MIN  + (2*1024))
#define RF_S_REGISTERS_ADDR_MIN                 	(RF_L5_REGISTERS_ADDR_MIN + (2*1024))
#define RF_REGISTERS_ADDR_MAX		 	(RF_S_REGISTERS_ADDR_MIN + ((2*1024) - 1))
//
// RF ACQ registers: two buffers for each band, each buffer has 4x oversample of chip rate
//
#define RF_ACQ_L1_REGISTERS_ADDR_MIN                	(RF_REGISTERS_ADDR_MAX + 1)
#define RF_ACQ_L5_REGISTERS_ADDR_MIN                	(RF_ACQ_L1_REGISTERS_ADDR_MIN  + (4*128))
#define RF_ACQ_S_REGISTERS_ADDR_MIN                 	(RF_ACQ_L5_REGISTERS_ADDR_MIN + (4*128))
#define RF_ACQ_REGISTERS_ADDR_MAX		 	(RF_ACQ_S_REGISTERS_ADDR_MIN + ((4*128) - 1))

// important aux register ids.
#define CP_SHARED_MEM_PHYS_ADDR_REGISTER_ID			1
#define CP_MAX_COMMAND_COUNT_REGISTER_ID			2
#define CP_INTERRUPT_INTERVAL_REGISTER_ID			3
#define CP_PHASE_1_DURATION_REGISTER_ID			4
#define CP_RF_MS_LIMIT_REGISTER_ID				5
#define CP_RF_L1_MS_COUNTER_REGISTER_ID			6
#define CP_RF_L5_MS_COUNTER_REGISTER_ID			7
#define CP_RF_S_MS_COUNTER_REGISTER_ID			8

// status register ids.
#define CP_ACQUIRE_COMMAND_STATUS_REG_ID			9
#define CP_TRACK_COMMAND_STATUS_REG_ID			10

// clocking (rise/fall edge) register.
#define CP_RF_ADC_CLOCKING_REG_ID				11

// block sizes in number of words.
#define RF_TRACK_BLOCK_SIZE				1024  // 16K 2-bit samples.
#define RF_ACQ_BLOCK_SIZE				256   // 4K  2-bit samples.

//
// search space blocking parameters for acquisition: do not modify.
//
#define  ACQUIRE_DOPPLER_BLOCK_SIZE				4
#define  ACQUIRE_CODE_PHASE_BLOCK_SIZE			128


// Use 1/2-bit nco for acquire and track
#define  USE_TWO_BIT_NCO_FOR_ACQUIRE		1
#define  USE_TWO_BIT_NCO_FOR_TRACK		1

//
// PROBES
//
#define WAITCMDSTROBE_PROBE			0
#define RECVCMDSTROBE_PROBE			1
#define RECVCMD_PROBE			2
#define RECVACQUIRECMD_PROBE			3
#define RECVTRACKCMD_PROBE			4
#define STARTACQUIRE_PROBE			5
#define STARTACQUIREITERATION_PROBE		6
#define STARTACQUIRECORRELATE_PROBE		7
#define STARTCOPYPRN_PROBE			8
#define STARTACQUIRECORROUTERLOOP_PROBE      9
#define ACQUIRECORRINNERLOOP_PROBE      	10
#define STARTACQUIRECORR1MS_PROBE	      	11
#define ACQUIRECORR1MSITERATION_PROBE	12
#define PARAMETERACCESS_PROBE		13
#define CONTROLREGWRITE_PROBE		14
#define UPDATEMAX_PROBE			15
#define ACQCORRBEST_PROBE			16
#define STARTTRACK_PROBE			17
#define STARTTRACKCORR1MS_PROBE	      	18
#define TRACKCORR1MSITERATION_PROBE		19
#define TRACKSUMMARY_PROBE			20
#define MEMDAEMONWAITCPENABLE_PROBE		21
#define MEMDAEMONWAITMEMENABLE_PROBE		22
#define MEMDAEMONRECVMEMENABLE_PROBE		23
#define MEMDAEMONLOOPITERATION_PROBE		24
#define CPINTERRUPT_SYNCH_PROBE		25
#define CPREGWRITECMD_PROBE			26
#define CPREGREADCMD_PROBE			27


#define P_DEFAULT_UART_BAUD_RATE		115200
#define P_DEFAULT_CLK_FREQUENCY		80000000 // 80 Mhz for the prototype.
//
//
// id's of internal registers in the RF resmpler controller and
// engines.
//
//
#define RF_CONTROLLER_CONTROL_REGISTER_ID		0
#define RF_ENGINE_CONTROL_REGISTER_ID		0
#define RF_ENGINE_FILTER_2_OUTPUT_QUANT_THRES_REG_ID 1
#define RF_ENGINE_FILTER_3_OUTPUT_QUANT_THRES_REG_ID 2
#define RF_ENGINE_SEQUENCE_ERROR_REG_ID		3
#define RF_FILTER_1_COEFF_REGISTER_BASE_ID		8
#define RF_FILTER_2_COEFF_REGISTER_BASE_ID		40
#define RF_FILTER_3_COEFF_REGISTER_BASE_ID		72
#define RF_DELTA_PHASE_IN_BASE_ID			104
#define RF_DELTA_PHASE_OUT_BASE_ID			106
#define RF_NUM_INPUT_SAMPLES_PER_MS_BASE_ID  	108
#define RF_WIPEOFF_NCO_PHASE_STEP_BASE_ID		110
#define RF_WIPEOFF_NCO_ZERO_IF_PHASE_STEP_BASE_ID	111
#define RF_COS_TABLE_BASE_ID				112
#define RF_SINE_TABLE_BASE_ID			144
#define RF_COS_TABLE_COPY_BASE_ID			176
#define RF_SINE_TABLE_COPY_BASE_ID			208


//
// cos table is array of int8.
//
#define RF_LOG_COS_TABLE_SIZE              	8 
#define RF_COS_TABLE_SIZE			( 1 << RF_LOG_COS_TABLE_SIZE )
#define RF_COS_TABLE_MASK                    ( RF_COS_TABLE_SIZE - 1 )


// FIR filter in resampler has order 128.
#define RF_FILTER_ORDER			128

// band coding.
#define RF_L1_BAND				1
#define RF_L5_BAND				2
#define RF_S_BAND				3

//
// in RF controller, the register ids for the
// 3 engines are relative to these offsets.
//
#define RF_CONTROLLER_CONTROL_REG_ID			0
#define RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR		1024
#define RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR		2048
#define RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR		3072

#define RF_NSAMPLES_PER_MS_FROM_RESAMPLER		(16*1023)
#define RF_BLOCK_SIZE_IN_U64				512   // 2bits-per-sample x 256 u64 = 512.

#define I15_ZMAGN					(0x3fff / 0x3)
#define I23_ZMAGN					(0x3fffff / 0x3)
#endif
