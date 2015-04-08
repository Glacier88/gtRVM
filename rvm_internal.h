#include <vector>
#include <string>

typedef struct {
    std::string seg;		/* segment in memory */
    std::string log;		/* log */
    std::string copy;
} Content;


typedef struct {
    std::string name;
    std::string content;
    void *ptr;
    bool beingModified;
} Segment;

typedef std::unordered_map<std::string, Segment*> SegNameMap;
typedef std::unordered_map<void*, Segment*> SegPtrMap;

typedef std::pair<int, std::string> Log;
typedef std::vector<std::pair<int, std::string> > Log;
typedef std::unordered_map<void*, std::vector<Log> > Transaction;


class Rvmt { 
 public:    
    std::string directory;	/* the directory name */
    SegNameMap seg_name; /* array of existing segments */
    SegPtrMap seg_ptr;
        
    Rvmt(const char *dir) : directory(dir) {
	// if the directory already exists, then mkdir return -1
	mkdir(dir, 0755); //create directory for log files
    }
    
    ~Rvmt(){}   
    
    Segment* find_by_name(std::string segname){
	SegNameMap::iterator = seg_name.find(segname);
	if(iterator == segs.end()) return NULL;
	else return iterator->second;
    }

    Segment* find_by_ptr(void *p){
	SegPtrMap::iterator = seg_ptr.find(p);
	if(iterator == segs.end()) return NULL;
	else return iterator->second;
    }
    
    /* load content in sgement on disk to memeory, and insert the entry in
     * to hash table
     */
    Segment* create_seg(std::string segname, int size){
	Segment* seg = new Segment; 
	(seg->content).resize(size);
	seg_name.emplace (segname, seg); /* insert new entry to hash table */
	seg_ptr.emplace (&(seg->content[0]), seg);
	
	return seg;
    }
    
    void delete_seg(Segment *seg){
	std::string segname = seg->name;
	void *p = seg->ptr;
	seg_name.erase(segname);
	seg_ptr.erase(p);
	delete seg;
    }

    void write_seg(Segment *seg, int size_to_create){
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
	    fprintf(stderr, "cannot open the file %s\n", file);
	    exit(-1);
	}
    }
    
    
};

typedef *Rvmt rvm_t;		/* rvmt points to _rvm_t  */
typedef Transaction* trans_t;
