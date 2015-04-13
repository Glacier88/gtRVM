#include "rvm.h"

//////////////////////////////////////////////////////////////////////
//                  Implementation of Rvmt                          //
//////////////////////////////////////////////////////////////////////

Segment* Rvmt::find_by_name(std::string segname){
    SegNameMap::iterator iter = seg_name.find(segname);
    if(iter == seg_name.end()) return NULL;
    else return iter->second;
}

Segment* Rvmt::find_by_ptr(void *p){
    SegPtrMap::iterator iter = seg_ptr.find(p);
    if(iter == seg_ptr.end()) return NULL;
    else return iter->second;
}

Segment* Rvmt::create_seg(std::string segname, int size){
    Segment* seg = new Segment; 
    seg->name=segname;
    (seg->content).resize(size);
    seg->ptr = (void*) &(seg->content[0]);
    seg_name.emplace (segname, seg); /* insert new entry to hash table */
    seg_ptr.emplace (&(seg->content[0]), seg);
    return seg;
}

void Rvmt::delete_seg(Segment *seg){
    std::string segname = seg->name;
    void *p = seg->ptr;
    seg_name.erase(segname);
    seg_ptr.erase(p);
    delete seg;
}

void Rvmt::load_seg(Segment *seg, int size_to_create){
    std::fstream file(directory+"/"+seg->name,std::fstream::binary | std::fstream::out | std::fstream::in);
    //Debug
    if(!file){
    	file.open(directory+"/"+seg->name,std::fstream::binary | std::fstream::out);
    }

    if (file) { // successfully open the file
	/* get length of file: */
	file.seekg (0, file.end); 
	int length = file.tellg();
	file.seekg (0, file.beg);
	    
	if (length < size_to_create){
	    /* set pos to size-1 => after writing one char, we get size */
	    file.seekp(size_to_create-1);
	    char c = '\0';
	    file.write(&c, 1);
	}
	    
	assert ( seg->content.size() == size_to_create);
	file.read(&(seg->content[0]), size_to_create);
	file.close();
	    
	//std::cout.write (&(seg->content[0]), size_to_create);
    }
    else{
	fprintf(stderr, "cannot open the file %s \n", seg->name.c_str());
	exit(-1);
    }
}
    
    

//////////////////////////////////////////////////////////////////////
//               Implementation of Transaction                      //
//////////////////////////////////////////////////////////////////////

Logs* Transaction::find_logs(void *segbase){
    LogsMap::iterator iter = undo.find(segbase);
    if(iter == undo.end()) return NULL;
    else return iter->second;
}

void Transaction::create_logs(void* segbase){
    Logs *logs = new Logs;
    undo.emplace(segbase, logs);
}

void Transaction::append_log(Logs* logs, void *segbase, int offset, int size){
    Log a(offset, std::string( (char*)segbase + offset, size));
    logs->push_back(a);
}

void Transaction::commit(){
    for(LogsMap::iterator iter = undo.begin(); iter != undo.end(); iter++){    
	void* segbase = iter->first;
	Logs* logs = iter->second;
	Segment* seg = rvm -> find_by_ptr(segbase); 

	/* open the log file */
	std::string segname = seg->name; 
	//open the back store
	std::fstream seg_file;
	seg_file.open((rvm->directory+"/"+segname).c_str(), std::fstream::in|std::fstream::out|std::fstream::binary);
	if(seg_file==NULL)
		fprintf(stdout, "cannot open the file %s at commit\n", segname.c_str());
        //Debug
	std::string logFileName = rvm->directory+"/"+segname+".log";
	std::ofstream logfile;
	logfile.open(logFileName, std::ofstream::binary | 
		     std::ofstream::out | std::ofstream::app);
	 
	/* write changed data (offset, length, new data) onto disk */
	for (size_t i = 0; i < logs->size(); i++) {
	    int offset = (*logs)[i].first;
	    int len = (*logs)[i].second.size();
	    logfile.write((char*)&offset, sizeof(int));
	    logfile.write((char*)&len, sizeof(int));
	    logfile.write((char*)segbase+offset, len);
	    //debug
	    seg_file.seekp(offset);
	    seg_file.write((char*)segbase+offset, len);
	}
	    
	logfile.close();
	seg->beingModified = false; /* reset the busy bit */
	delete logs;
    }
}

void Transaction::abort(){
    for(LogsMap::iterator iter = undo.begin(); iter != undo.end(); iter++){
	    
	void* segbase = iter->first;
	Logs* logs = iter->second;
	    	 
	/* copy the logs back to segments in memory */
	for (size_t i = 0; i < logs->size(); i++) {
	    int offset = (*logs)[i].first;
	    int len = (*logs)[i].second.size();
	    char *a = &( ( (*logs)[i].second )[0] );
	    memcpy((char*)segbase+offset, a, len);
	}
	    
	delete logs;
    }
}

//////////////////////////////////////////////////////////////////////
//               Implementation of RVM                              //
//////////////////////////////////////////////////////////////////////

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
	rvm->load_seg(newSeg, size_to_create);
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
	fprintf(stderr, "unmap non-existing segment !\n");
	exit(-1);
    }
}

// two cases :
// 1. not find the segname, rm the segment on disk.
// 2. not find the segname, and also not on disk, return.
void rvm_destroy(rvm_t rvm, const char *segname) {
	Segment* seg=rvm->find_by_name(segname);
	//Should not be called if the segment is mapped
	if(seg!=NULL)
		return;
        std::string seg_file=rvm->directory+"/"+segname;
	unlink(seg_file.c_str());
	std::string log_file=seg_file.append(".log");
	unlink(log_file.c_str());
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
    Transaction *trans = new Transaction;
    for (int i = 0; i < numsegs; i++) {
	Segment* seg = rvm->find_by_ptr(segbases[i]);
	if(seg == NULL){
	    fprintf(stderr, "segment:%s not exist !\n",seg->name.c_str());
	    delete trans;
	    exit(-1);
	}
	else{
	    if(seg->beingModified){
		fprintf(stderr, "segment:%s is under transaction !\n",seg->name.c_str());
		delete trans;
		return (trans_t) -1;
	    }
	    else{
		seg->beingModified = true; /* mark it as busy */
		trans->rvm = rvm;	   /* set the associated rvm */
		trans->create_logs(seg->ptr); /* create log for this segment */
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
    if( tid == (trans_t) -1 || tid == NULL){
	fprintf(stderr, "transaction does not exist !\n");
	exit(-1);
    }
    else {
	Logs* logs = tid->find_logs(segbase);
	if ( logs == NULL){
	    fprintf(stderr, "segment does not belong to this transaction !\n");
	    exit(-1);
	}	
	else {
	    tid->append_log(logs, segbase, offset, size);   
	}
    }
}

/**
 *
 */
void rvm_commit_trans(trans_t tid){
    if(tid == (trans_t) -1 || tid == NULL){
	fprintf(stderr, "transaction does not exist !\n");
	exit(-1);
    }
    else {
	tid->commit();
	delete tid;
    }
}


void rvm_abort_trans(trans_t tid){
    if(tid == (trans_t) -1 || tid == NULL){
	fprintf(stderr, "transaction does not exist !\n");
	exit(-1);
    }
    else {
	tid->abort();
	delete tid;
    }
}

void rvm_truncate_log(rvm_t rvm){
	DIR *store_dir;
	dirent *dir_entry;
	store_dir=opendir(rvm->directory.c_str());
	if(store_dir==NULL)
		return;
	while((dir_entry=readdir(store_dir))!=NULL){
		std::string fname=dir_entry->d_name;
		size_t idx=fname.rfind(".log");
		if(idx==std::string::npos)
			continue;
		std::string seg_name=fname.substr(0,idx);
		std::string log_name=rvm->directory+"/"+seg_name+".log";
		if(rvm->find_by_name(seg_name)==NULL){
			std::string command="truncate -s 0\t"+log_name;
			system(command.c_str());		
	        }	
	}
}


