/********************************************************************
 PostPic - An image-enabling extension for PostgreSql
 (C) Copyright 2010 Domenico Rotiroti

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation, version 3 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 A copy of the GNU Lesser General Public License is included in the
 source distribution of this software.

 postpic_import:
 Import images from client file system to server

********************************************************************/

#include "postpic_utils.h"
#include <libpq/libpq-fs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

typedef struct pp_import_options {
	pp_connect_options co;
	char * callback;
	char * usrdata;
} pp_import_options;

static pp_import_options opts;

void pp_import_usage(const char * pname)
{
	pp_print_usage(pname);
	fprintf(stderr, "Additional options: -c callback [ -u userdata ]  <filename> [<filename> ...]\n"
		"\t-c callback\tcallback is a stored procedure to call with each\n"
		"\t\t\t eg. to insert the image in a table. The signature needs to be:\n"
		"\t\t\t callback(i image, imgpath varchar, usrdata varchar)\n"
		"\t-u usrdata\toptional userdata to pass to callback function\n");
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

/*
 * Fixed data for pp_import
 */
const int binFmt = 1;
int len;
char * val;

void pp_import(PGconn * conn, const char * file)
{
	PGresult * res;
	char q[BUFSIZE];
	const char * const * ival = (const char * const *) &val;
	struct stat st;
	FILE * fimg;

	/* read file */
	if(stat(file, &st)<0) {
		fprintf(stderr, "ERROR: Can't stat file %s\n", file);
		return;
	}
	len = st.st_size;
	val = malloc(len);
	if(!val) {
		pp_print_error("Out of memory");
		return;
	}
	fimg = fopen(file, "r");
	fread(val, 1, len, fimg);
	fclose(fimg);
	/* run query */
	sprintf(q, "select %s($1::image, '%s', '%s')", opts.callback, file, opts.usrdata);
	res = PQexecParams(conn, q,
			1, //n. of params
			NULL, //oids guessed by backend
			ival,
			&len,
			&binFmt,
			binFmt);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) pp_print_error(PQerrorMessage(conn));
	PQclear(res);
	free(val);
	fprintf(stderr, "INFO: imported file %s\n", file);
}

int main(int argc, char * argv[])
{
	PGconn * conn;
	char * pname = argv[0];
	PGresult   *res;
	int i;
	
	// parse input and test connections
	if((argc==1) || pp_parse_import_options(&argc, &argv, &opts)) {
		pp_import_usage(pname);
		return -1;
	}
	
	if(!opts.callback || !argc) {
		if(!opts.callback) pp_print_error("What should I do with the images? Please use the -c option.");
		if(!argc) pp_print_error("Please specify at least one file to import.");
		return -1;
	}
	
	conn = pp_connect(&(opts.co));
	if(PQstatus(conn)!=CONNECTION_OK) {
		pp_print_error("Cannot connect to database using given parameters\n");
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
