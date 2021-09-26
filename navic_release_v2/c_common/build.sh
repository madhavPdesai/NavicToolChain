cd include
rm -f cp_rf_parameters.h
echo "#ifndef cp_rf_parameters_h___" > cp_rf_parameters.h
echo "#define cp_rf_parameters_h___" >> cp_rf_parameters.h
sed s/"\$parameter"/"#define"/  ../../aa_common/parameters.aa | sed s/"{"/"("/g | sed s/"}"/")"/g >> cp_rf_parameters.h
sed s/"\$parameter"/"#define"/  ../../aa_common/rf_parameters.aa | sed s/"{"/"("/g | sed s/"}"/")"/g >> cp_rf_parameters.h
echo "#endif" >> cp_rf_parameters.h
cd -
#scons


