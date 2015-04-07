#include <vector>
#include <string>


class Rvmt { 
 public:    
    std::string directory;	/* the directory name */
    std::vector<std::string> segs; /* array of existing segments */
    
 Rvmt(const char *dir) : directory(dir) {
	// if the directory already exists, then mkdir return -1
	mkdir(dir, 0755); //create directory for log files
    }
    
    ~Rvmt(){}   
    
    bool find(std::string segname){
	std::vector<std::string>::iterator = 
	    std::find(segs.begin(), segs.end(), segname);
	if(iterator == segs.end()) return false;
	else return true;
    }
    
    
};

typedef *Rvmt rvm_t;		/* rvmt points to _rvm_t  */
