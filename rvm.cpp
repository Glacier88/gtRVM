#include "rvm.h"

rvm_t rvm_init(const char *directory){
    // need to consider the case when directory already exists.
    rvm_t rvm = new Rvmt(directory);  
    
    return rvm;              //return the rvm
}

// procedure:
// 1 check for redo log, if it exists, then apply it to both the segment
//   on disk and in memory
// 2 load the content of the disk segment to memory.
// 
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){
    if(size_to_create <= 0) return (void*) -1;

    Segment* seg = rvm->find_by_name(segname);
    if(seg == NULL){
	Segment* newSeg = rvm->create_seg(segname, size_to_create);
	write_seg(newSeg, size_to_create);
	// return new allocated memory
	return &(newSeg->content[0]);
    }
    else{
	fprintf(stderr, "cannot map the same segment twice ! \n");
	exit(-1);
    }
}

void rvm_unmap(rvm_t rvm, void *segbase) {
    Segment* seg = rvm->find_by_ptr(segbase);
    if(seg != NULL){
	rvm->delete_seg(seg);
    }
    else{
	fprintf(sterr, "unmap non-existing segment !\n");
	exit(-1);
    }
}

// two cases :
// 1. find the segname, then remove the segments in memory and disk
// 2. not find the segname, rm the segment on disk.
// 3. not find the segname, and also not on disk, return.
void rvm_destroy(rvm_t rvm, const char *segname) {

    
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
    Transaction *trans = new Transaction;
    for (int i = 0; i < numsegs; i++) {
	Segment* seg = rvm->find_by_ptr(segbases[i]);
	if(seg == NULL){
	    fprintf(sterr, "segment not exist !\n");
	    exit(-1);
	}
	else{
	    if(seg->beingModified){
		fprintf(sterr, "segment is under transaction !\n");
		return -1;
	    }
	    else{
		seg->beingModified = true;
		Log log;
		trans.emplace(seg->ptr, log);
	    }
	}
    }
    
    return trans;
}

/**
 * Three cases:
 * 1. transaction not exist
 * 2. segment does not belong to this transaction
 * 3. It is there => create the undo log
 */
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
    if(tid == -1 || tid == NULL){
	fprintf(sterr, "transaction does not exist !\n");
	exit(-1);
    }
    else {
	Transaction::iterator =tid->find(segbase);
	if ( iterator == tid->end()){
	    fprintf(sterr, "segment does not belong to this transaction !\n");
	    exit(-1);
	}	
	else {
	    Log undoLog(offset, std::string(segbase+offset, size));
	    iterator->second.push_back(undoLog);
	}
    }
}

void rvm_commit_trans(trans_t tid){
    if(tid == -1 || tid == NULL){
	fprintf(sterr, "transaction does not exist !\n");
	exit(-1);
    }
    else {
	
	
    }
}
void rvm_abort_trans(trans_t tid);
void rvm_truncate_log(rvm_t rvm);


