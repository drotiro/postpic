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
	fprintf(stderr, "Additional parameters: <filename> [<filename> ...]\n");
	exit(EXIT_FAILURE);
}

void pp_import(PGconn * conn, const char * file)
{
	Oid loid;

	loid = lo_import(conn, file);
		if(loid==InvalidOid) {
		printf("null_image()\n");
	} else {
		printf("%ld\n", (long)loid);
	}
}

int main(int argc, char * argv[])
{
	Oid loid;
	PGconn * conn;
	char * pname = argv[0];
	char * file;
	PGresult   *res;
	int i;
	
	// parse input and open file
	if(pp_parse_connect_options(&argc, &argv, &opts)) {
		pp_import_usage(pname);
	}
	conn = pp_connect(&opts);
	if(PQstatus(conn)!=CONNECTION_OK) {
		fprintf(stderr, "Cannot connect to database using given parameters\n");
		return -1;
	}
	
	// import files into server
	if(!argc) {
		pp_import_usage();
		PQfinish(conn);
		return -1;
	}
	res = PQexec(conn, "begin");
	PQclear(res);
	for(i = 0; i < argc; ++i) {
		pp_import(conn, argv[i]);
	}
	res = PQexec(conn, "end");
	PQclear(res);	
	PQfinish(conn);

	return 0;
}
