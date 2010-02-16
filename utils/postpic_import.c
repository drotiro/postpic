/*******************************
 PostPic - An image-enabling
 extension for PostgreSql
  
 Author:    Domenico Rotiroti
 License:   LGPLv3
 
 Import images from client file
 system to server
*******************************/

#include "postpic_utils.h"
#include <libpq/libpq-fs.h>

#include <stdio.h>
#include <stdlib.h>

static pp_connect_options opts;

void pp_import_usage()
{
	fprintf(stderr, "Additional parameters needed: <filename>\n");
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
	PGconn * conn;
	char * pname = argv[0];
	
	// parse input and open file
	if(pp_parse_connect_options(&argc, &argv, &opts)) {
		pp_import_usage(pname);
	}
	conn = pp_connect(&opts);
	if(PQstatus(conn)!=CONNECTION_OK) {
		fprintf(stderr, "Cannot connect to database using given parameters\n");
		return -1;
	}
	
	file = pp_open(argv[0]);
	if(!file) {
		fprintf(stderr, "Cannot open input file %s\n", argv[0]);
		PQfinish(conn);
		return -1;
	}
	
	// import into server
	loid = pp_import(file);
	PQfinish(conn);
	fclose(file);

	// print out the result
	if(loid==InvalidOid) {
		printf("null_image()");
	} else {
		printf("%ld", (long)loid);
	}
	
	return 0;
}