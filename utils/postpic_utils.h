/*******************************
 PostPic - An image-enabling
 extension for PostgreSql
  
 Author:    Domenico Rotiroti
 License:   LGPLv3
 
 Common utilities
*******************************/

#ifndef PP_UTILS_H
#define PP_UTILS_H

#include <libpq-fe.h>

#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 4096

typedef struct pp_connect_options {
	char * host;
	char * user;
	char * password;
	char * dbname;
} pp_connect_options;


int pp_parse_connect_options (int *argc, char ** argv[], pp_connect_options * opts);

PGconn * pp_connect(pp_connect_options * opts);

#endif
//PP_UTILS_H