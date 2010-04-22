/*******************************
 PostPic - An image-enabling
 extension for PostgreSql
  
 Author:    Domenico Rotiroti
 License:   LGPLv3
 
 Export images to client file
 system
*******************************/

#include "postpic_utils.h"
#include <libpq/libpq-fs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

typedef struct pp_export_options {
	pp_connect_options co;
	char * namecol;
	char * nameprefix;
	char * imagecol;
} pp_export_options;

static pp_export_options opts;

#define NAMESIZE 2048
static char filenamebuf[NAMESIZE];

void pp_export_usage(const char * pname)
{
	pp_print_usage(pname);
	fprintf(stderr, "Additional options: -i image_column -c name_column -o name_prefix \"select query to execute\"\n"
		"\t-i image_column\tcolumn containing the images to export\n"
		"\t-c name_column\tcolumn to use as output file name\n"
		"\t-o name_prefix\tprefix to use to build output file name\n"
		"\t\t(use -c or -o)\n");
}

int	pp_parse_export_options(int * argc, char ** argv[], pp_export_options * opts)
{
	int c;
	char allopts[32];
	
	sprintf(allopts, "%s%s", CONNECTOPTS, "c:o:i:");
	opts->namecol = opts->nameprefix = opts->imagecol = NULL;
	
	while((c = getopt(*argc, *argv, allopts)) != -1) {
		if(strchr(CONNECTOPTS, c)) {
			pp_parse_connect_opt((char) c, &opts->co);
		} else {
			switch(c) {
				case 'c':
					opts->namecol = optarg;
					break;
				case 'i':
					opts->imagecol = optarg;
					break;
				case 'o':
					opts->nameprefix = optarg;
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

char * image_name(const char * prefix, int num)
{
	sprintf(filenamebuf, "%s%3d.jpg", prefix, num);
	return filenamebuf;
}

void pp_export(PGconn * conn, const char * query)
{

	const int binFmt = 1;
	int len = 0, i;
	int rows, ncol, icol;
	char * val;
	char * file;
	PGresult * res;
	const char * const * ival = (const char * const *) &val;
	FILE * fimg;

	/* execute the user provided query */
	res = PQexecParams(conn, query,
			0, //n. of params
			NULL, //oids guessed by backend
			NULL,
			NULL,
			NULL,
			binFmt);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		pp_print_error(PQerrorMessage(conn));
		PQclear(res);
		return;
	}
	
	/* some check */
	icol = PQfnumber(res, opts.imagecol);
	if(icol==-1) {
		fprintf(stderr, "ERROR: Image column '%s' does not exist in result set.\n", opts.imagecol);
		PQclear(res);
		return;
	}
	if (!opts.nameprefix) {
		ncol = PQfnumber(res, opts.namecol);
		if(ncol==-1) {
			fprintf(stderr, "ERROR: Name column '%s' does not exist in result set.\n", opts.namecol);
			PQclear(res);
			return;
		}
	}
	rows = PQntuples(res);
	
	/* fetch the data and save */
	for (i = 0; i < rows; ++i) {
		//val = ??
		//file = ??	
		if(opts.namecol) file = PQgetvalue(res, i, ncol);
		else file = image_name(opts.nameprefix, i);
		fimg = fopen(file, "w");
		//fwrite(val, 1, len, fimg);
		fclose(fimg);
		//free(val);
	}
	PQclear(res);
	fprintf(stderr, "INFO: exported file %s\n", file);
}

int main(int argc, char * argv[])
{
	PGconn * conn;
	char * pname = argv[0];
	
	// parse input and test connections
	if((argc==1) || pp_parse_export_options(&argc, &argv, &opts)) {
		pp_export_usage(pname);
		return -1;
	}
	
	if(!argc) {
		pp_print_error("Please specify the query to execute to fetch images.");
		return -1;
	}
	
	if( !opts.namecol && !opts.nameprefix) {
		pp_print_error("Please use -c or -o to set output file naming.");
		return -1;
	}

	if( !opts.imagecol) {
		pp_print_error("Please use -i to specify column containing images.");
		return -1;
	}

	conn = pp_connect(&(opts.co));
	if(PQstatus(conn)!=CONNECTION_OK) {
		pp_print_error("Cannot connect to database using given parameters\n");
		return -1;
	}
	
	if(argc>1) {
		fprintf(stderr, "WARN: ignoring extra arguments\n");
	}
	
	pp_export(conn, argv[0]);
	PQfinish(conn);

	return 0;
}
