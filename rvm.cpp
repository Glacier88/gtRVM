#include "rvm.h"

rvm_t rvm_init(const char *directory){
    rvm_t rvm = new Rvmt(directory);  
    
    return rvm;              //return the rvm
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){

    bool doesFind = rvm->find(segname);
    if(doesFind){
	rvm->createSeg(size_to_create);
    }
    else{
	rvm->enlarge(size_to_create);
    }
    
    return newMem; // return new allocated memory
}

void rvm_unmap(rvm_t rvm, void *segbase) {
    
    bool doesFind = rvm->find(segbase);
    if(doesFind){
	
    }
    else{
	
    }
    
}

void rvm_destroy(rvm_t rvm, const char *segname) {
    
    
}
