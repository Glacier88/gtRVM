#ifndef RVM_INTERNAL_H
#define RVM_INTERNAL_H

#include <vector>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <dirent.h>

typedef struct {
    std::string name;		/* the name of the segment */
    std::string content;	/* the content */
    void *ptr;			/* first address of the content */
    bool beingModified=false;	/* busy bit */
} Segment;


typedef std::unordered_map<std::string, Segment*> SegNameMap;
typedef std::unordered_map<void*, Segment*> SegPtrMap;
/* Log : offset + content. */
typedef std::pair<int, std::string> Log;
typedef std::vector<Log> Logs;
typedef std::unordered_map<void*, Logs* > LogsMap;


class Rvmt { 
public:    
    std::string directory;	/* the directory name */
    SegNameMap seg_name;     /* list of existing segments */
    SegPtrMap seg_ptr;	     /* list of existing segments */
        
    Rvmt(const char *dir) : directory(dir) {
	/* if the directory already exists, then mkdir return -1*/
        /*create directory for log files*/
	mkdir(dir, 0755); 
    }
    ~Rvmt(){}  
    Segment* find_by_name(std::string segname);
    Segment* find_by_ptr(void *p);    
    Segment* create_seg(std::string segname, int size);
    void delete_seg(Segment *seg);
    void load_seg(Segment *seg, int size_to_create);
    void apply_log(std::string segName);
    void truncate_log();
    void print_file(std::string file); 
};
typedef Rvmt* rvm_t;		/* rvmt points to _rvm_t  */

class Transaction {
public:
    Rvmt *rvm;			/* the rvm that is associated with */
    LogsMap undo;		/* undo copy */
    Logs* find_logs(void *segbase);
    void create_logs(void* segbase);
    void append_log(Logs* logs, void *segbase, int offset, int size);
    void commit();
    void abort();
};
typedef Transaction* trans_t;	/* trans_t points to Transaction */

#endif 		/* RVM_INTERNAL_H */
