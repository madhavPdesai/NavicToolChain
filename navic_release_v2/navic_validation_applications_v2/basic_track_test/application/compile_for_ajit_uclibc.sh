MAIN=basic_track_test
compileToSparcUclibc.py -V vmap.txt -I $NAVIC_HOME/c_common/include -I $NAVIC_HOME/c_common/prn/include -c $NAVIC_HOME/c_common/prn/src/all_sats_prn_gen.c -o 3 -s init.s -s trap_handlers.s -I $AJIT_PROJECT_HOME/tools/ajit_access_routines/include -C $NAVIC_HOME/c_common/src -C $AJIT_PROJECT_HOME/tools/ajit_access_routines/src -C $AJIT_PROJECT_HOME/tools/minimal_printf_timer/src -I $AJIT_PROJECT_HOME/tools/minimal_printf_timer/include -c main.c  -N ${MAIN} -L LinkerScript.lnk  -D HAS_FLOAT -D AJIT -U -D __CPPA__=0x40040000