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

Oid pp_import(PGconn * conn, const char * file)
{
	PGresult   *res;
	Oid loid;
	
	res = PQexec(conn, "begin");
	PQclear(res);
	loid = lo_import(conn, file);
	res = PQexec(conn, "end");
	PQclear(res);	
	
	return loid;
}

int main(int argc, char * argv[])
{
	Oid loid;
	PGconn * conn;
	char * pname = argv[0];
	char * file;
	
	// parse input and open file
	if(pp_parse_connect_options(&argc, &argv, &opts)) {
		pp_import_usage(pname);
	}
	conn = pp_connect(&opts);
	if(PQstatus(conn)!=CONNECTION_OK) {
		fprintf(stderr, "Cannot connect to database using given parameters\n");
		return -1;
	}
	
	// import into server
	file = argv[0];
	if(!file) {
		pp_import_usage();
		PQfinish(conn);
		return -1;
	}
	loid = pp_import(conn, file);
	PQfinish(conn);

	// print out the result
	if(loid==InvalidOid) {
		printf("null_image()\n");
	} else {
		printf("%ld\n", (long)loid);
	}
	
	return 0;
}