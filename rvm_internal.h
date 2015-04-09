#ifndef RVM_INTERNAL_H
#define RVM_INTERNAL_H

#include <vector>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <cassert>
#include<cstring>

typedef struct {
    std::string name;		/* the name of the segment */
    std::string content;	/* the content */
    void *ptr;			/* first address of the content */
    bool beingModified;		/* busy bit */
} Segment;

typedef std::unordered_map<std::string, Segment*> SegNameMap;
typedef std::unordered_map<void*, Segment*> SegPtrMap;

typedef std::pair<int, std::string> Log;
typedef std::vector<Log> Logs;
typedef std::unordered_map<void*, Logs* > LogsMap;


class Rvmt { 
public:    
    std::string directory;	/* the directory name */
    SegNameMap seg_name;     /* list of existing segments */
    SegPtrMap seg_ptr;	     /* list of existing segments */
        
    Rvmt(const char *dir) : directory(dir) {
	// if the directory already exists, then mkdir return -1
	mkdir(dir, 0755); //create directory for log files
    }
    
    ~Rvmt(){}   
    
    Segment* find_by_name(std::string segname);
    Segment* find_by_ptr(void *p);    
    Segment* create_seg(std::string segname, int size);
    void delete_seg(Segment *seg);
    void load_seg(Segment *seg, int size_to_create);
    
};

typedef Rvmt* rvm_t;		/* rvmt points to _rvm_t  */


class Transaction {
public:
    Rvmt *rvm;
    LogsMap undo;

    Logs* find_logs(void *segbase);
    void create_logs(void* segbase);
    void append_log(Logs* logs, void *segbase, int offset, int size);
    void commit();
    void abort();

};

typedef Transaction* trans_t;

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
    std::fstream file;
    file.open(directory + seg->name, std::ifstream::in | std::ifstream::out);

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
	std::string logFileName = rvm->directory + segname;
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



#endif 		/* RVM_INTERNAL_H */
