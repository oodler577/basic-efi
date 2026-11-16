#include "prg_checks.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size){
    if(size>PRG_CAPACITY+3)return 0;
    static int initialized;if(!initialized){maps();initialized=1;}
    check_bytes(data,size,PRG_CAPACITY+2);
    if(size>=2)check_bytes(data,size,((unsigned)data[0]+256u*data[1])%(PRG_CAPACITY+3));
    return 0;
}
