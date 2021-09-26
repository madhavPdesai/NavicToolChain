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

#define NCMD    2
//#define NMS     1
//#define NBLOCKS 1

#define DOPPLERMIN  -1000.0
#define DOPPLERMAX   1000.0
#define DOPPLERSTEP    500.0
#define ACTUALDOPPLER  1000.0

#define CODEDELAYMIN  0
#define CODEDELAYMAX  127
#define CODEDELAYSTEP 1

#include <irnss_prn.h>
#include <filters.h>

void testRf(int I)
{
	uint32_t wword = 0x87654321;
	writeCoprocessorRfWord(1,RF_L1_BAND,1, I, wword);

	uint32_t rword = readCoprocessorRfWord(1,RF_L1_BAND,1, I);
	if(rword != wword)
		PRINTF("Error: acq-rf-word[%d] = 0x%x, expected 0x%x\n", I, rword, wword);
}
	
// write 4X oversamples prn into buffers.
void writePrnIntoAcquireRfBuffers()
{
	uint32_t wword = 0;
	float time_step =  ((2.0 * M_PI * ACTUALDOPPLER)/(4.0*1023.0* 1000.0));
	int I;
	int sample_count = 0;
	double elapsed_time = 0;
			
	PRINTF("Info: entered writePrnIntoAcquireRfBuffers (timestep=%12.8f)\n", time_step);
	I = 0;
	int buf_id = 0;
	while(1)
	{
		//if((I % 32) == 0) PRINTF("I=%d, buf_id=%d\n",I, buf_id);

		int8_t prn_val = IRNSS_PRN_CODES[I];
		int J = 0;

		uint8_t os_val = 0;
		for(J=0; J < 4; J++)
		{
			sample_count++;

			int8_t cos_val = 1;
			if((elapsed_time > (M_PI/2.0)) && 
					(elapsed_time < (1.5*M_PI)))
			{
				cos_val = -1;
			}

			int8_t sig_val = prn_val * cos_val;
			uint8_t usig_val = (uint8_t) ((sig_val == 1) ? 0 : 2);
			os_val = (os_val << 2) | (usig_val & 0x3);

			elapsed_time += time_step;
			if(elapsed_time > (2.0*M_PI))
				elapsed_time = elapsed_time - (2.0*M_PI);
		}

		wword = (wword << 8) | os_val;
		if(I == 1022)
			wword = (wword << 8);

		if ((I == 1022) || (((I+1) % 4)  == 0))
		{
			writeCoprocessorRfWord(1,RF_L1_BAND,buf_id, (I >> 2), wword);
			uint32_t rword = readCoprocessorRfWord(1,RF_L1_BAND, buf_id, (I >> 2));
			if(rword != wword)
			{
				PRINTF("Error: RF-word buf=%d, offset=%d = 0x%x (expected 0x%x)\n", buf_id, (I >> 2), rword, wword);
			}
			
			wword = 0;
		}

		int break_flag = ((buf_id == 1) && (I == 1022));
		if(break_flag)
			break;

		buf_id =  ((I == 1022) ? (1 - buf_id) : buf_id);
		I = ((I == 1022) ? 0 : (I+1));

	}
}

void execAcquire(CoprocessorState* cp_state, int COUNT, int NMS, int NBLOCKS, double doppler_min, double doppler_max, double doppler_step,
			uint32_t code_phase_min, uint32_t code_phase_max, uint32_t code_phase_step)
{	
	PRINTF("Info: checking command buffer status [%d]\n", COUNT);

	double NSTEPS = (ceil((doppler_max - doppler_min)/(doppler_step))+1)*
				(((ceil(fabs(code_phase_max - code_phase_min)))/code_phase_step)+1.0);

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
		PRINTF("Info: scheduling acquire [%d]\n", COUNT);
		scheduleAcquireCommand(cp_state,
					0, // satellite-id
					1, // diff combining
					1, // l1 band
					NMS, // number of rf blocks to use
					doppler_min, //  doppler-min	
					doppler_max,  //  doppler-max
					doppler_step,  //  doppler-bin-size
					code_phase_min,	 //  code-delay min
					code_phase_max,	 //  code-delay max
					code_phase_step, 	 //  code-delay bin-size
					0);	 //  drop-threshold.
			
		// read back.
		// uint64_t cval = cp_state->command_response_array[0].args[0];
	}


	//
	// generate the strobe .
	//
	PRINTF("Info: generate strobe [%d].\n", COUNT);
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_COMMAND_STROBE_MASK);


	// check that the acquire command has started
	while(1)
	{
		CpCommandDaemonStatus cds = getCoprocessorCommandDaemonStatus(cp_state);
		uint32_t CR = read_from_cp_reg(0);
		if(cds == COMPLETED_SWEEP)
		{
			PRINTF("SWEEP COMPLETED (cr=0x%x) [%d]\n", CR, COUNT);
			writeCoprocessorControlWord (CP_ENABLE_MASK);
			
			int MS=0;
			int done=0;
			while(1)
			{
				uint32_t cell_count;
				uint8_t  command_id, started, finished, waiting_for_rf;

				while(1)
				{
					probeAcquireCommandStatus(&cell_count, &command_id, 
							&started, &finished, &waiting_for_rf);

					//PRINTF("Info:probeAcquire[%d]: %d %d %d %d %d [%d]\n", MS, cell_count, command_id, started, finished, waiting_for_rf, COUNT);
					if(finished)
					{
						PRINTF("Acquire command [%d] finished\n", COUNT);	
						done = 1;
						break;
					}
					else if(waiting_for_rf)
					{
						PRINTF("Acquire: waiting for 1ms [%d] [%d]\n", MS, COUNT);
						//toggleActiveRfBlockId(RF_L1_BAND);
						toggleActiveRfBlockId();

						MS++;
						PRINTF("Acquire: toggled block-id\n", MS, COUNT);
						break;
					}

				}
				if(done)
					break;
			}
						
			PRINTF("Acquire: finished advancing MS to %d\n",MS);

			// all commands have completed?
			while(1)
			{
				uint32_t register_value;
				uint32_t out_args[8];
				CpCommand op_code;



				CpCommandStatus status = parseCommandResponse(cp_state,
						0, &op_code, &register_value, NULL, out_args);
				if(status == COMPLETED)
				{
					PRINTF("Acquire response [%d]: reg-value=%d\n",COUNT, register_value);
					int I;
					for(I=0; I < 8; I++)
					{
						PRINTF("         out_arg[%d] = 0x%x\n", I, out_args[I]);	
					}

					double max_e = out_args[0];
					double rms_scaled_e = out_args[3];
					double fom_value = (max_e * NSTEPS) / ((1 << 8) * rms_scaled_e);

					PRINTF("FOM (NSTEPS=%10.0f, peak/mean-squared) = %10.4f\n", NSTEPS, fom_value);
					break;
				}
			}
			break;
		}
		else
			PRINTF("SWEEP NOT COMPLETED (cr=0x%x)\n", CR);
	}

	PRINTF("Info: acquire command completed [%d]\n", COUNT);
}

int interrupt_serviced_flag = 0;
NavicState navic_state;
int main()
{
	setCpLinkVerboseFlag(0);

	__ajit_write_serial_control_register__ (TX_ENABLE);
	__ajit_write_irc_control_register__(1);

#ifndef __CPPA__
	PRINTF("Error: CP command buffer physical address not defined\n");
	return(1);
#endif

	PRINTF("Info: started initializing coprocessor.\n");
	initializeCoprocessor(&(navic_state.coprocessor_state),
			0, 
			CP_COMMAND_BUFFER_VIRTUAL_ADDR_BASE,  // on the processor side
			//CP_COMMAND_BUFFER_PHYSICAL_ADDR_BASE, // on the coprocessor side.
			__CPPA__ , // for test purposes
			NCMD,
			(16.0*1023*1000), 
			100000,
			20000);
	PRINTF("Info: virtual address of command_response_array is 0x%x\n",
			(uint32_t) navic_state.coprocessor_state.command_response_array);
	PRINTF("Info: virtual address of command_response_array[0].args[0]) is 0x%x\n",
			(uint32_t) &(navic_state.coprocessor_state.command_response_array[0].args[0]));

	testRf(0);
	testRf(63);
	testRf(127);

	writePrnIntoAcquireRfBuffers();
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
						FILTER_1_COEFFS,
						FILTER_2_COEFFS,
						FILTER_3_COEFFS);	
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

	//
	// download prn code for satellite
	//
	PRINTF("Info: activating satellite 0.\n");
	activateSatellite (cp_state, 0, 1, IRNSS_PRN_CODES);

	//
	// 32 code-phases, 20 dopplers = 2 blocks..
	//

	// should get good FOM here.
	//execAcquire(cp_state, 0, 1, 4, 0.0 ,10000.0, 500.0, 0, 63, 1);
	execAcquire(cp_state, 0, 1, 1, 0.0 ,1000.0, 500.0, 0, 16, 1);

	// should get even better FOM here.
	execAcquire(cp_state, 1, 1, 164,  0.0 ,10000.0, 500.0,  0, (4*1023)-2, 2);

	// FOM should be degraded.
	execAcquire(cp_state, 2, 1, 164,  0.0 ,10000.0, 500.0,  16, (4*1023)-2, 2);

	// FOM should be degraded.
	execAcquire(cp_state, 3, 1, 164,  500.0 ,10000.0, 500.0,  0, (4*1023)-2, 2);

	// FOM should be even more degraded.
	execAcquire(cp_state, 4, 1, 164,  1000.0 ,10000.0, 500.0,  32, (4*1023)-2, 2);

	
	// Good FOM
	execAcquire(cp_state, 5, 1, 2, 0.0 ,1000.0, 500.0, 0, 256, 1);

	PRINTF("Info: all done\n");
	return (0);
}

void coprocessor_interrupt_handler()
{
	return;
}


