//
// An application which initializes the coprocessor and rf resampler
//
//

#include <stdlib.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>

#define NCMD 2

// from Matlab (Gaurav).
int8_t filter_stage_1_coeff[128] = {0,1,0,-1,0,1,-1,-1,1,0,-2,1,1,-1,-1,1,0,0,0,-1,2,1,-4,1,6,-5,-5,9,1,-11,4,9,-8,-5,9,0,-6,2,1,2,1,-10,3,17,-17,-17,34,4,-48,21,48,-52,-29,76,-6,-81,48,62,-82,-23,95,-23,-82,62,48,-81,-6,76,-29,-52,48,21,-48,4,34,-17,-17,17,3,-10,1,2,1,2,-6,0,9,-5,-8,9,4,-11,1,9,-5,-5,6,1,-4,1,2,-1,0,0,0,1,-1,-1,1,1,-2,0,1,-1,-1,1,0,-1,0,1,0};

int8_t filter_stage_2_coeff[128] = {0,0,0,1,1,1,1,0,0,-1,-1,-2,-1,-1,0,0,0,0,-1,-1,-1,-1,1,3,6,7,7,6,2,-2,-7,-10,-11,-10,-6,-2,1,3,1,-2,-5,-6,-3,4,16,28,37,38,30,12,-14,-41,-65,-77,-73,-53,-20,20,58,85,95,85,58,20,-20,-53,-73,-77,-65,-41,-14,12,30,38,37,28,16,4,-3,-6,-5,-2,1,3,1,-2,-6,-10,-11,-10,-7,-2,2,6,7,7,6,3,1,-1,-1,-1,-1,0,0,0,0,-1,-1,-2,-1,-1,0,0,1,1,1,1,0,0,0};

int8_t filter_stage_3_coeff[128] = {0,0,0,1,1,1,1,0,0,-1,-1,-2,-1,-1,0,0,0,0,-1,-1,-1,-1,1,3,6,7,7,6,2,-2,-7,-10,-11,-10,-6,-2,1,3,1,-2,-5,-6,-3,4,16,28,37,38,30,12,-14,-41,-65,-77,-73,-53,-20,20,58,85,95,85,58,20,-20,-53,-73,-77,-65,-41,-14,12,30,38,37,28,16,4,-3,-6,-5,-2,1,3,1,-2,-6,-10,-11,-10,-7,-2,2,6,7,7,6,3,1,-1,-1,-1,-1,0,0,0,0,-1,-1,-2,-1,-1,0,0,1,1,1,1,0,0,0,   0, 0, 0, 0, 0, 0, 0};

#include <irnss_prn.h>

int8_t prn_buffer[1024];
	
// write 16X oversamples prn into buffers.
void writePrnIntoTrackRfBuffers(int sat_id)
{
	uint32_t wword = 0;
	int I;

	generatePrnSequence(sat_id, prn_buffer);
			
	PRINTF("Info: entered writePrnIntoTrackRfBuffers\n");
	for(I = 0; I < 1023; I++)
	{
		int8_t prn_val = prn_buffer[I];
		//PRINTF("SAT %d PRN[%d]=%d\n", sat_id, I, prn_val);

		uint8_t uprn_val = ((prn_val == 1) ? 2 : 0);

		int J;
		wword = 0;

		for(J = 0; J < 16; J++)
		{
			wword = (wword << 2) | uprn_val;
		}

		writeCoprocessorRfWord(0,RF_L1_BAND,0, I, wword);
		writeCoprocessorRfWord(0,RF_L1_BAND,1, I, wword);

		//PRINTF("Info: acq-rf-word[%d] = 0x%x\n", (I>>2), wword);
	}
	
	// last one.
	writeCoprocessorRfWord(0,RF_L1_BAND,0, 1023, 0);
	writeCoprocessorRfWord(0,RF_L1_BAND,1, 1023, 0);
}

void execTrack (CoprocessorState* cp_state,
			int sat_id,
			double carrier_freq,
			double carrier_phase,
			uint32_t early,
			uint32_t prompt,
			uint32_t late)
{
	PRINTF("Info: checking command buffer status\n");
	CpCommandStatus status = parseCommandResponse(cp_state,
								0,
								NULL,
								NULL,
								NULL,
								NULL);

	// 	schedule register write commands for a satellite
	// 	write to utility register.
	if((status == INVALID) || (status == COMPLETED))
	{
		PRINTF("Info: scheduling track\n");
		scheduleTrackCommand(cp_state,
					sat_id-1, // satellite-id
					RF_L1_BAND, // l1 band
					carrier_freq, //  doppler-freq	
					carrier_phase,  //  doppler-phase
					early, // early
					prompt,	// mid
					late		// late
					);
			
	}


	//
	// generate the strobe .
	//
	PRINTF("Info: generate strobe.\n");
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_COMMAND_STROBE_MASK);


	// check that the track command has started
	while(1)
	{
		CpCommandDaemonStatus cds = getCoprocessorCommandDaemonStatus(cp_state);
		uint32_t CR = read_from_cp_reg(0);
		if(cds == COMPLETED_SWEEP)
		{
			PRINTF("SWEEP COMPLETED (cr=0x%x)\n", CR);
			writeCoprocessorControlWord (CP_ENABLE_MASK);
			toggleActiveRfBlockId(RF_L1_BAND);
			
			// all commands have completed?
			while(1)
			{
				uint32_t register_value;
				uint32_t rbuf_id;
				uint32_t out_args[8];
				CpCommand op_code;
			
			

				CpCommandStatus status = parseCommandResponse(cp_state,
						0, &op_code, &register_value, &rbuf_id, out_args);
				if((status != COMPLETED) && (status != INVALID))
				{
					PRINTF("Command not completed.\n");
				}
				else
				{
					PRINTF("Track response: reg-value=%d, rbuf_id=%d\n",register_value, rbuf_id);
					int I;
					for(I=0; I < 8; I++)
					{
						PRINTF("         out_arg[%d] = 0x%x\n", I, out_args[I]);	
					}
					break;
				}
			}
			break;
		}
		else
			PRINTF("SWEEP NOT COMPLETED (cr=0x%x)\n", CR);
	}
	PRINTF("Info: track command completed\n");
	return;
}

int interrupt_serviced_flag = 0;
NavicState navic_state;
int main()
{
	setCpLinkVerboseFlag(0);

	__ajit_write_serial_control_register__ (TX_ENABLE);
 	__ajit_write_irc_control_register__(1);

	PRINTF("Info: started initializing coprocessor.\n");
	initializeCoprocessor(&(navic_state.coprocessor_state),
						0, 
						CP_COMMAND_BUFFER_VIRTUAL_ADDR_BASE,  // on the processor side
						__CPPA__, // for test purposes
						NCMD,
						(16.0*1023*1000), 
						100000,
						 20000);
	PRINTF("Info: virtual address of command_response_array is 0x%x\n",
			(uint32_t) navic_state.coprocessor_state.command_response_array);
	PRINTF("Info: virtual address of command_response_array[0].args[0]) is 0x%x\n",
			(uint32_t) &(navic_state.coprocessor_state.command_response_array[0].args[0]));
			
	PRINTF("Info: finished initializing coprocessor.\n");

	///////////////////////////////////////////////////////////////////////////////////////////
	// Initialize RF chain
	///////////////////////////////////////////////////////////////////////////////////////////

	// same adc rates at l1,l5,s
#ifndef INCLUDE_RF
	navic_state.resampler_state.configuration.l1_configuration.ignore_flag = 1;
	navic_state.resampler_state.configuration.l5_configuration.ignore_flag = 1;
	navic_state.resampler_state.configuration.s_configuration.ignore_flag = 1;
#else
	int    upsample_factor = 1;
	double adc_sampling_frequency = 56.0 * 1000000; // 56 MHz.
	double adc_if = 16221000.0;
	double resampler_output_sampling_rate = 16 * 1023000;  	// resampler output sampling rate
	double IF_at_resampler_output = 4000000;  	// IF = 4MHz at resampler output.

	setupRfResamplerEngineConfiguration(&(navic_state.resampler_state.configuration.l1_configuration),
						RF_L1_BAND,
						adc_sampling_frequency,
						adc_if,
						upsample_factor,
						IF_at_resampler_output,
						resampler_output_sampling_rate,
						filter_stage_1_coeff,
						filter_stage_2_coeff,
						filter_stage_3_coeff);	
	navic_state.resampler_state.configuration.l5_configuration.ignore_flag = 1;
	navic_state.resampler_state.configuration.s_configuration.ignore_flag = 1;

	/*
	setupRfResamplerEngineConfiguration(&(navic_state.resampler_state.configuration.l5_configuration),
						RF_L5_BAND,
						adc_sampling_frequency,
						adc_if,
						upsample_factor,
						IF_at_resampler_output,
						resampler_output_sampling_rate,
						filter_stage_1_coeff,
						filter_stage_2_coeff,
						filter_stage_3_coeff);	
	setupRfResamplerEngineConfiguration(&(navic_state.resampler_state.configuration.s_configuration),
						RF_S_BAND,
						adc_sampling_frequency,
						adc_if,
						upsample_factor,
						IF_at_resampler_output,
						resampler_output_sampling_rate,	
						filter_stage_1_coeff,
						filter_stage_2_coeff,
						filter_stage_3_coeff);	
	*/

	initCosTable(navic_state.resampler_state.configuration.cos_table);
	configureRfResampler(&(navic_state.resampler_state.configuration));
	verifyRfResamplerConfiguration(&(navic_state.resampler_state.configuration));

	setRfResamplerEngineEnables(1,0,0);
#endif


	///////////////////////////////////////////////////////////////////////////////////////////
	// Start the coprocessor.
	///////////////////////////////////////////////////////////////////////////////////////////
	CoprocessorState *cp_state = &(navic_state.coprocessor_state); 

	//
	// 	enable the coprocessor (without interrupt enabled)
	//
	PRINTF("Info: enable coprocesor.\n");
	writeCoprocessorControlWord (CP_ENABLE_MASK);


	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= 32; SAT_ID++)
	{
		PRINTF("Info: activating satellite %d.\n", SAT_ID);
		writePrnIntoTrackRfBuffers(SAT_ID);
		generatePrnAndActivateSatellite(cp_state, SAT_ID, RF_L1_BAND);
		execTrack(cp_state, SAT_ID, 0.0, 0.0, 0, 8, (1023-8));
	}

	PRINTF("Info: all done.\n");
	return (0);
}

void coprocessor_interrupt_handler()
{
	return;
}


