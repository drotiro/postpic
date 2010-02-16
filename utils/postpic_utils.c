/*******************************
 PostPic - An image-enabling
 extension for PostgreSql
  
 Author:    Domenico Rotiroti
 License:   LGPLv3
*******************************/

#include "postpic_utils.h"
#include <string.h>
#include <stdio.h>
#include <getopt.h>

void pp_print_usage(const char *pname)
{
	fprintf(stderr, "Usage: %s [connection options] [additional options]\n", pname);
	fprintf(stderr, "Connection options:\n"
		"\t-U username\n"
		"\t-P password\n"
		"\t-h host\n"
		"\t-d database\n");
}

int pp_parse_connect_options (int *argc, char ** argv[], pp_connect_options * opts)
{
	int c;
	
	while((c = getopt(*argc, *argv, "U:P:h:d:")) != -1) {
		switch(c) {
			case 'U':
				opts->user = optarg;
				break;
			case 'P':
				opts->password = optarg;
				break;
			case 'h':
				opts->host = optarg;
				break;
			case 'd':
				opts->dbname = optarg;
				break;
			default:
				pp_print_usage((*argv)[0]);
				return 1;			
		}
	}
	(*argv) += optind;
	return 0;
}

#define PP_ADDPROP(NAME) if(opts->NAME) { strcat(buf, " " #NAME " = "); strcat(buf,opts->NAME); }
PGconn * pp_connect(pp_connect_options * opts)
{
	char buf[BUFSIZE]="";
	PP_ADDPROP(host);
	PP_ADDPROP(dbname);
	PP_ADDPROP(user);
	PP_ADDPROP(password);
	
	return PQconnectdb(buf);
}