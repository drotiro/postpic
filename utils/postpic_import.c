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
#include <string.h>
#include <getopt.h>

typedef struct pp_import_options {
	pp_connect_options co;
	char * callback;
	char * usrdata;
} pp_import_options;

static pp_import_options opts;

void pp_import_usage(const char * pname)
{
	pp_print_usage(pname);
	fprintf(stderr, "Additional options: [-c callback [ -u userdata ] ] <filename> [<filename> ...]\n"
		"\t-c callback\tcallback is a stored procedure to call after each import\n"
		"\t\t\t eg. to insert the image in a table. The signature should be:\n"
		"\t\t\t callback(i image, imgpath varchar, usrdata varchar)\n"
		"\t-u usrdata\toptional userdata to pass to callback function\n");
	exit(EXIT_FAILURE);
}

int	pp_parse_import_options(int * argc, char ** argv[], pp_import_options * opts)
{
	int c;
	char allopts[32];
	
	sprintf(allopts, "%s%s", CONNECTOPTS, "c:u:");
	opts->usrdata="";
	
	while((c = getopt(*argc, *argv, allopts)) != -1) {
		if(strchr(CONNECTOPTS, c)) {
			pp_parse_connect_opt((char) c, &opts->co);
		} else {
			switch(c) {
				case 'c':
					opts->callback = optarg;
					break;
				case 'u':
					opts->usrdata = optarg;
					break;
				default:
					return 1;
			}
		}
	}
	(*argv) += optind;
    (*argc) -= optind;
	return 0;
}

void pp_import(PGconn * conn, const char * file)
{
	Oid loid;
	PGresult * res;
	char q[BUFSIZE];

	loid = lo_import(conn, file);
		if(loid==InvalidOid) {
		printf("null_image()\n");
	} else {
		printf("%ld\n", (long)loid);
	}
	
	if(opts.callback) {
		sprintf(q, "select %s(image_from_large_object('%ld'), '%s', '%s')", opts.callback, (long)loid,
			file, opts.usrdata);
		res = PQexec(conn, q);
		PQclear(res);
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
	
	// parse input and test connections
	if(pp_parse_import_options(&argc, &argv, &opts)) {
		pp_import_usage(pname);
	}
	conn = pp_connect(&(opts.co));
	if(PQstatus(conn)!=CONNECTION_OK) {
		fprintf(stderr, "Cannot connect to database using given parameters\n");
		return -1;
	}
	
	// import files into server
	if(!argc) {		
		pp_import_usage(pname);
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
