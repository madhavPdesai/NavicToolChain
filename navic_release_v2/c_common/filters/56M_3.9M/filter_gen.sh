rm filters.h
# filter 1 fs = 56M, if=3.9M
python ../scripts/filter_design.py -s 56000000.0 -c 3900000.0 -w 2600000.0 -t 250000.0 -n 128 -B -o filters.h -f FILTER_1  -d
# filter 2 fs = 56M, if=4M
python ../scripts/filter_design.py -s 56000000.0 -c 4000000.0 -w 2600000.0 -t 250000.0 -n 128 -B -o filters.h -f FILTER_2  -d 
# filter 3 fs = 56M, lpf cutoff=2M
python ../scripts/filter_design.py -s 56000000.0 -x 1300000.0  -t 250000.0 -n 128 -L -o filters.h -f FILTER_3  -d

