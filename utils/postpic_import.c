#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

#include <stdio.h>
#include <stdlib.h>

void pp_usage(const char * argv0)
{
	fprintf(stderr, "Usage: %s <filename>\n", argv0);
	exit(EXIT_FAILURE);
}

FILE * pp_open(const char * fname)
{
	return fopen(fname, "r");
}

Oid pp_import(FILE * file)
{
	return InvalidOid;
}

int main(int argc, char * argv[])
{
	FILE * file;
	Oid loid;
	
	// parse input and open file
	if(argc<2 || argv[1][0]=='-') {
		pp_usage(argv[0]);
	}
	file = pp_open(argv[1]);
	if(!file) {
		fprintf(stderr, "Cannot open input file %s\n", argv[1]);
		return -1;
	}
	
	// import into server
	loid = pp_import(file);
	fclose(file);

	// print out the result
	if(loid==InvalidOid) {
		printf("null_image()");
	} else {
		printf("%ld", (long)loid);
	}
	
	return 0;
}