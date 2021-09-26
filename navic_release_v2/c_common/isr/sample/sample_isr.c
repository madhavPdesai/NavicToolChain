//
// An application which initializes the coprocessor and rf resampler
//
//
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <isr.h>

// NAVIC state global data structure.
extern NavicState navic_state;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  coprocessor interrupt service routine								   	//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void coprocessor_interrupt_handler() 
//
// Called on every 1ms interrupt from the coprocessor
//
// Requirement:  This routine MUST complete in 500 usec.
//
//
{
	//
	// read back status information from the command
	// buffers.
	uint8_t satellite_id;
	for(satellite_id= 0; satellite_id < navic_state.coprocessor_state.max_number_of_commands; satellite_id++)
	{
		CpCommand op_code;
		uint32_t register_value;
		uint32_t out_args[8];

		CpCommandStatus status = parseCommandResponse(&(navic_state.coprocessor_state),
								satellite_id,
								&op_code,
								&register_value,
								out_args);

		if(status == COMPLETED)
		{
			if(op_code == TRACK_OP)
			{
				float carrier_frequency, carrier_phase;
				uint32_t early_code_delay, prompt_code_delay, late_code_delay;
				uint8_t  band;
				int continue_flag = updateTrackInformation(satellite_id,
										out_args[0], // early ip
										out_args[1], // early qp
										out_args[2], // prompt ip
										out_args[3], // prompt qp
										out_args[4], // late iq
										out_args[5], // late qp
										out_args[6], // residual phase
										&band,
										&carrier_frequency,
										&carrier_phase,
										&early_code_delay,	
										&prompt_code_delay,	
										&late_code_delay);	

				if(continue_flag)
				{
					scheduleTrackCommand(&(navic_state.coprocessor_state),
							satellite_id,
							band,
							carrier_frequency, 
							carrier_phase,
							early_code_delay,
							prompt_code_delay,
							late_code_delay);

				}	
			}
			else if (op_code == ACQUIRE_OP)
			{
				updateAcquireInformation(satellite_id,
						out_args[0], // best power
						out_args[1], // best doppler
						out_args[2]  // best code-delay
						);

			}
			else if (op_code == READ_REG_OP)
			{
				updateReadRegInformation (satellite_id, register_value);
			}
			else if ((op_code == WRITE_REG_OP) || (op_code == NOP_OP))
			{
				updateWriteRegOrNopInformation (satellite_id);
			}
		}
	}

	return;
}
