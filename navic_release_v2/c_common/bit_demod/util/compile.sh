gcc -g -c -Wall ../src/bit_demodulate.c  -I ../include -D NDEBUG_MODE -D DUMP_BITS
gcc -g -c -Wall ./test_bit_demodulate.c -I ../include
gcc -g -o test_bit_demodulate  test_bit_demodulate.o bit_demodulate.o
