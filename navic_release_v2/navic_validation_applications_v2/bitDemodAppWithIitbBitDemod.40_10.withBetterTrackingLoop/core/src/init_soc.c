#include <stdlib.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <app_defines.h>
#include <acquire.h>
#include <satellite.h>

extern uint64_t volatile total_ticks_in_interrupt_handler;
extern uint64_t volatile max_ticks_in_interrupt_handler;
extern uint64_t volatile last_ticks_in_interrupt_handler;
extern uint32_t volatile number_of_interrupts;

extern NavicState volatile navic_state;
extern SatelliteState  volatile satellite_status[ACTIVE_NUMBER_OF_SATELLITES];

void init_soc()
{
	// enable AJIT processor serial device.
	__ajit_write_serial_control_register__ (TX_ENABLE);

	// enable AJIT processor interrupt controller.
	__ajit_write_irc_control_register__(1);


	//
	// initialize the satellites and enable those you have specified.
	//
	initializeAllSatelliteStructures(GPS_L1,IRNSS_L5,IRNSS_S);


	CoprocessorState* cp_state = (CoprocessorState*) &(navic_state.coprocessor_state);

	// tick counter (this is for performance measurement only)
	total_ticks_in_interrupt_handler = 0;
	max_ticks_in_interrupt_handler = 0;

	// milli-second counter.
	number_of_interrupts = 0;

	// turn off useless messages.
	setCpLinkVerboseFlag(0);

#ifndef __CPPA__
	PRINTF("Error: CP command buffer physical address not defined\n");
	return(1);
#endif

	PRINTF("Info: started initializing coprocessor.\n");
	initializeCoprocessor(cp_state,
			// coprocessor-index
			0,
			// command buffer virtual addres.
			CP_COMMAND_BUFFER_VIRTUAL_ADDR_BASE,  
			// command buffer physical address.
			__CPPA__ , 
			// 64 satellites 32xGPS, 14xIRNSS_L5, 14xIRNSS_S, 3 dummies.
			ACTIVE_NUMBER_OF_SATELLITES,
			// 16.368 MHz.
			RESAMPLER_OUTPUT_SAMPLING_FREQUENCY,
			//
			// these two fields are not used
			// in normal operation.
			//
			__CLOCK_FREQUENCY__/1000,
			// do not generate command strobe automatically
			// it will be generated in the interrupt handler.
			(__CLOCK_FREQUENCY__/1000) + 1);
	PRINTF("Info: virtual address of command_response_array is 0x%x\n",
			(uint32_t) navic_state.coprocessor_state.command_response_array);
	PRINTF("Info: virtual address of command_response_array[0].args[0]) is 0x%x\n",
			(uint32_t) &(navic_state.coprocessor_state.command_response_array[0].args[0]));

	///////////////////////////////////////////////////////////////////////////////////////////
	// Initialize RF chain
	///////////////////////////////////////////////////////////////////////////////////////////
	PRINTF("Info: started initializing RF chains.\n");

	//
	// Clocking edge for sampling ADC data. sampling edge can
	// be falling or rising edge.
	//
	// setCoprocessorAdcClocking(L1_ADC_FALLING_EDGE, L5_ADC_FALLING_EDGE, S_ADC_FALLING_EDGE);


#if GPS_L1
		setupRfL1Engine();
#else
		navic_state.resampler_state.configuration.l1_configuration.ignore_flag = 1;
#endif

#if IRNSS_L5
		setupRfL5Engine();
#else
		navic_state.resampler_state.configuration.l5_configuration.ignore_flag = 1;
#endif

#if IRNSS_S
		setupRfSEngine();
#else
		navic_state.resampler_state.configuration.s_configuration.ignore_flag = 1;
#endif

	// initialize the sine/cosine tables.
	initCosSineTables(navic_state.resampler_state.configuration.cos_table,
			navic_state.resampler_state.configuration.sine_table);

	configureRfResampler(&(navic_state.resampler_state.configuration));
	PRINTF("Info: finished configuring RF resampler.\n");

	verifyRfResamplerConfiguration(&(navic_state.resampler_state.configuration));
	PRINTF("Info: finished verifying RF resampler.\n");

	setRfResamplerEngineEnables(GPS_L1,IRNSS_L5,IRNSS_S);
	PRINTF("Info: enabled RF engine (%d,%d,%d).\n", GPS_L1, IRNSS_L5, IRNSS_S);

	///////////////////////////////////////////////////////////////////////////////////////////
	// Set the limit on the size of ms data... 0 means free-running.
	///////////////////////////////////////////////////////////////////////////////////////////
	setRfMaxMsCount (0,0,0);

	// enable the RF memory interfaces.
	uint32_t toggle_mask = 
		(GPS_L1 	? CP_TOGGLE_RF_L1_ENABLE_MASK : 0) |
		(IRNSS_L5 	? CP_TOGGLE_RF_L5_ENABLE_MASK : 0) |
		(IRNSS_S 	? CP_TOGGLE_RF_S_ENABLE_MASK : 0);
	writeCoprocessorControlWord (toggle_mask);
	PRINTF("Info: toggled RF mem interface enables.\n");

	// enable interrupts.
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_INTR_ENABLE_MASK);
	PRINTF("Info: enabled CP interrupts.\n");
}

