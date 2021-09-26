gcc -o prn_gen_test -g -I../include test_prn_gen.c ../src/irnss_prn_gen.c
gcc -o generate_prn_headers -g -I../include generate_prn_headers.c ../src/irnss_prn_gen.c
gcc -o generate_all_sats_prn_headers -g -I../include -I ../../include  generate_all_sats_prn_headers.c ../src/all_sats_prn_gen.c -D N___DEBUGPRINT___
