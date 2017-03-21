#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <regex.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

// Daniel Rushton, rushton, u0850493





// arguments
typedef struct ARGS {
 unsigned int s, E, b;
 char *path;
 bool h, v;
}ARGS;

// results (hits, misses, evictions)
typedef struct RES{
unsigned int h,m,e;
}RES;

// models one cache access
typedef struct ACS {
	uint64_t m;
	uint64_t tag;
	uint32_t set,block;
	uint32_t size;
	char type;
}ACS;

// cache structure
typedef struct LINE {
	uint64_t tag;
	bool vald;
	int age;
}LINE;

// cache is just a pointer to
// the first position in a contiguous
// block of memory allocated as
typedef LINE* CSH;





// globals
ARGS *args;
uint32_t SETS, LINS, BLKS;							// these are const once initially set
CSH csh;
unsigned hits, misses, evics;





// Function declarations
FILE* get_args(int argc, char **argv);
void sim (FILE *f, RES **r);
void proc_line (char *str, ACS **ac);
void print_access (ACS *ac);
void setup();
void simline (ACS *a);
LINE* get_ptr (ACS *a);
LINE* getlru (LINE *set);
uint64_t gettag(uint64_t m);
void repl (ACS *a);





// main entry point
int main(int argc, char **argv) {

	hits = misses = evics = 0;

	struct RES *r;
	FILE *file;
	// parse command line args and attempt to open file.
	file = get_args(argc, argv);

	// set up the cache
	setup();

	// run simulations
	sim(file, &r);
	// print the results
	printSummary(hits, misses, evics);
}



// parses command line arguments, opens file and
// sets up input for processing.
FILE* get_args(int argc, char **argv) {

	// error and useage mesages.
	char * usage = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n";
	char * arg_error = "Missing required command line argument\n";
	char * options = "Options\n\
	-h        Print this help message.\n\
	-v        Optional verbose flag.\n\
	-s <num>  Number of set index bits.\n\
	-E <num>  Number of lines per set.\n\
	-b <num>  Number of block offset bits.\n\
	-t <file> Trace file.\n";

	// allocate the arguments struct
	args = (ARGS*)malloc(sizeof(ARGS));
	FILE *file;
  int c = 0;

	// parse command line arguments and options.
  // Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>
	while ((c = getopt (argc, argv, "hvs:E:b:t:")) != -1) {
			switch(c)
				{
					case 's' : args->s = atoi(optarg); break;
				  case 'E' : args->E = atoi(optarg); break;
					case 'b' : args->b = atoi(optarg); break;
					case 't' : args->path = optarg; break;
					case 'v' : args->v = true; break;
					case 'h' : args->h = true;
					case '?' : printf("%s\n", usage);
				}
	}

	// make sure no args are null or invalid.
  if(( args->s < 1) || ( args->E < 1) ||( args->b < 1) ||( args->path == NULL )) {
		printf("%s\n", arg_error);
		printf("%s\n", usage);
		printf("%s\n", options);
		free(args);
		exit(EXIT_FAILURE);
	}

	// open file for reading and check for success
	file = fopen(args->path,"r");
	if (file == NULL) {
		printf("%s: No such file or directory \n", args->path);
		free(args);
		exit(EXIT_FAILURE);
	}

	return file;
}





// determines and sets global dimensions
// and allocates cache.
void setup () {

	SETS = (uint32_t) 1 << (args->s);
	LINS = (uint32_t) args->E;
	BLKS = (uint32_t) 1 << (args->b);
	csh = (LINE*)malloc(SETS * LINS * sizeof(LINE));

	// traverse cache and set ages to 0
	// cache lines will all be 'dirty'
	LINE *next = csh;
	int ln = 0;
	int TLNS = SETS*LINS;
	while(ln < TLNS) {
		next->age = 0;
		next->vald = false;
		next++;
		ln++;
	}

// debug
//	printf("Created cache with: %d sets\n", SETS);
//	printf("                  : %d lines per set\n", LINS);
//	printf("                  : %d bytes per block\n", BLKS);

}




// run the simulation...
// simulate cache accesses line by line
void sim (FILE *f, RES **r) {

	char line[128];
	while (fgets(line, sizeof line, f) != NULL) {

		ACS *acs;
		if(line[0] == ' ') {
			proc_line(line, &acs);

			// print current line details
			if(args->v)
				printf("%c %llx,%d ",line[1], (unsigned long long)acs->m,(int)acs->size );

			switch (line[1]) {

			case 'L' :
   		case 'S' :  simline(acs); break;
			case 'M' :  simline(acs); simline(acs); break; // a mod is just two accesses

			}
	 		if (args->v) printf("\n");
		}
	}
}



// parse file line...
// takes a line of input, finds cache
// location and populates an access struct
void proc_line (char *str, ACS **ac) {

	*ac = (ACS*)malloc(sizeof(ACS));

	// get arguments
	unsigned s, b;
	s = (*args).s;		// index bits  (number of sets = 2^s)
 	b = (*args).b;		// block bits (block size = 2^b)

	// split string into type, add, and size field
	(*ac)->type = str[1];
 	(*ac)->size = atoi(strchr(str, ',') + 1);
	uint64_t m = strtoul(&(strtok(str, ","))[2], NULL, 16 );
	(*ac)->m = m;

	// isolate set, line, and block bits and populate access data
	int numt = 64 - (s + b);
	(*ac)->tag = m >> (s + b);													// tag bits
	(*ac)->set = (m << numt) >> (numt + b);							// set bits
	(*ac)->block = (m << (numt + s)) >> (numt + s);     // block bits
}




// traverse lines in set, if line with
// matching tag and valid set, hit.
void simline (ACS *a) {

  // locate first line of set
	LINE *lin = get_ptr(a);

	bool hit = false;

	// search through lines
	// important! : set line age to zero on a hit
	int ln = 0;
	while (ln < LINS) {

		if ( lin->tag == a->tag && lin->vald) {
			hit = true;
			hits++;
			lin->age = 0;
			if(args->v)
				printf(" hit");
		}
		else
			lin->age++;
		lin++;
		ln++;
	}

	// not found, it's a miss, needs replacement
	if (!hit)
	{
		misses++;
		if(args->v) { printf(" miss"); }
		repl(a);
	}
}




// returns a pointer to the first line
// of the appropriate set
LINE* get_ptr (ACS *a) {
	return csh + (a->set)*LINS ;
}




// returns the tag bits of an address
uint64_t gettag(uint64_t m) {
	return m >> ( LINS + BLKS );
}




// traverse set and find LRU (greatest age)
LINE* getlru (LINE *set) {

	LINE *lru = set;
	LINE *next = set;

	int ln = 0;
	while (ln < LINS) {
		if(next->age  > lru->age)
			lru = next;
		next++;
		ln++;
	}
	return lru;
}




// locate set, find lru, replace
// print to console if eviction occurs
void repl (ACS *a) {

	// locate first line of set
	LINE *set = get_ptr(a);

	// get LRU
	LINE *lru = getlru(set);

	if(lru->vald) {
		evics++;
		if(args->v) printf(" eviction");
	}
	// replace tag and update status
	lru->tag = a->tag;
	lru->vald = true;
	lru->age = 0;
}


// debugging function, prints the contents
// of one access struct
// void print_access(ACS *ac) {

//	printf("New Access --------------------------------\n");
//	printf("Address: %llu\n",(long long) ac->m);
//	printf("Type: %c\n", ac->type);
//	printf("Size: %d\n", ac->size);
//	printf("Set: %d\n", ac->set);
//	printf("Tag: %x\n", (unsigned int)ac->tag);
//	printf("Block: %d\n", ac->block);
//}
