MAIN=bitDemodApp
CLOCK_FREQ=80000000
CMD_BUF_PHYS_ADDR=0x40040000
sameercode=sameerCodeDebug
CFLAGS=" -o 2 "
compileToSparcUclibc.py -V vmap.txt -I core/include/ -I hook/include -C hook/src  -I $NAVIC_HOME/c_common/include -I $NAVIC_HOME/c_common/bit_demod/include -I $NAVIC_HOME/c_common/prn/include -I $NAVIC_HOME/c_common/app_helpers/tracking_loop_v3/include  -C $NAVIC_HOME/c_common/app_helpers/tracking_loop_v3/src ${CFLAGS}  -s asm/init.s -s asm/trap_handlers.s -s asm/generic_isr.s  -s asm/mutexes.s -I $AJIT_PROJECT_HOME/tools/ajit_access_routines/include -C $NAVIC_HOME/c_common/bit_demod/src -c $NAVIC_HOME/c_common/prn/src/all_sats_prn_gen.c  -C $NAVIC_HOME/c_common/src -C $AJIT_PROJECT_HOME/tools/ajit_access_routines/src -C $AJIT_PROJECT_HOME/tools/minimal_printf_timer/src -I $AJIT_PROJECT_HOME/tools/minimal_printf_timer/include -C core/src  -N ${MAIN} -L LinkerScript.lnk  -D HAS_FLOAT -D AJIT -U -D __CPPA__=$CMD_BUF_PHYS_ADDR  -D __CLOCK_FREQUENCY__=$CLOCK_FREQ -D NDEBUG_PRINTF  -D NOFFLINE_PRINTF -D NPOP_STATUS_PRINT
