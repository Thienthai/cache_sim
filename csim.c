//Thienthai 5580636
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


typedef struct Result{
int h,m,e;
}Result;

typedef struct Cache {
	int m, tag, block, set, size;
	char type;
}Cache;

typedef struct line {
	int tag, age;
	bool vald;
}line;

typedef struct Input {
 char *inAdr;
 bool h, v;
 int s, E, b;
}Input;

typedef line* Point;


Input *args;
Point Mycache;
int hits, misses, evics, sets, Lines, blocks;


void getSim (FILE *f, Result **r);
void lineParse (char *str, Cache **ac);
void lineFind (Cache *a);
line* retPtr (Cache *a);
void LRU_Edit (Cache *a);


FILE* myInput(int argc, char **argv) {

	args = (Input*)malloc(sizeof(Input));
	FILE *file;
  int c = 0;

	while ((c = getopt (argc, argv, "hvs:E:b:t:")) != -1) {
			switch(c)
				{
					case 's' :
            args->s = atoi(optarg);
            break;
				  case 'E' :
            args->E = atoi(optarg);
            break;
					case 'b' :
            args->b = atoi(optarg);
            break;
					case 't' :
            args->inAdr = optarg;
            break;
					case 'v' :
            args->v = true;
            break;
					case 'h' :
            args->h = true;
				}
	}

	file = fopen(args->inAdr,"r");

	return file;
}

void getSim (FILE *f, Result **r) {

	char line[128];
	while (fgets(line, sizeof line, f) != NULL) {

		Cache *acs;
		if(line[0] == ' ') {
			lineParse(line, &acs);

			if(args->v)
				printf("%c %llx,%d ",line[1], (unsigned long long)acs->m,(int)acs->size );

			switch (line[1]) {

			     case 'L' :
   		     case 'S' :
              lineFind(acs);
              break;
			     case 'M' :
              lineFind(acs);
              lineFind(acs);
              break;

			}
	 		if (args->v)
        printf("\n");
		}
	}
}

void lineParse (char *str, Cache **ac) {

	*ac = (Cache*)malloc(sizeof(Cache));

	unsigned s, b;
	s = (*args).s;
 	b = (*args).b;


	(*ac)->type = str[1];
 	(*ac)->size = atoi(strchr(str, ',') + 1);
	int m = strtoul(&(strtok(str, ","))[2], NULL, 16 );
	(*ac)->m = m;

	int numt = 64 - (s + b);
	(*ac)->tag = m >> (s + b);
	(*ac)->set = (m << numt) >> (numt + b);
	(*ac)->block = (m << (numt + s)) >> (numt + s);
}

void lineFind (Cache *a) {


	line *lin = retPtr(a);

	bool hit = false;


	int ln = 0;
	while (ln < Lines) {

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


	if (!hit)
	{
		misses++;
		if(args->v) { printf(" miss"); }
		LRU_Edit(a);
	}
}

line* retPtr (Cache *a) {
  line* myPtr = Mycache + (a->set)*Lines ;
	return myPtr ;
}

int retTag(int m) {
  int myTag = m >> ( Lines + blocks );
	return myTag;
}

line* LRU (line *set) {

	line *lru = set;
	line *next = set;

	int ln = 0;
	while (ln < Lines) {
		if(next->age  > lru->age)
			lru = next;
		next++;
		ln++;
	}
	return lru;
}

void LRU_Edit (Cache *a) {

	line *set = retPtr(a);
	line *lru = LRU(set);

	if(lru->vald) {
		evics++;
		if(args->v) printf(" eviction");
	}

	lru->tag = a->tag;
	lru->vald = true;
	lru->age = 0;
}

int main(int argc, char **argv) {

	hits = misses = evics = 0;
	struct Result *r;
	FILE *file;
	file = myInput(argc, argv);
  sets = (int) 1 << (args->s);
	Lines = (int) args->E;
	Mycache = (line*)malloc(sets * Lines * sizeof(line));
  blocks = (int) 1 << (args->b);
	line *next = Mycache;
	int cnt = 0;
	int total = sets*Lines;
	while(cnt < total) {
    next->vald = false;
		next->age = 0;
    cnt++;
		next++;
	}

	getSim(file, &r);
	printSummary(hits, misses, evics);
}
