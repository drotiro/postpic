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
#define CONNECTOPTS "U:P:h:d:"

typedef struct pp_connect_options {
	char * host;
	char * user;
	char * password;
	char * dbname;
} pp_connect_options;

void pp_parse_connect_opt(char c, pp_connect_options * opts);

PGconn *	pp_connect(pp_connect_options * opts);
void		pp_print_error(const char * msg);
#endif
//PP_UTILS_H