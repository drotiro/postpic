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
#include <termios.h>

void pp_print_usage(const char *pname)
{
	fprintf(stderr, "Usage: %s [connection options] [additional options]\n", pname);
	fprintf(stderr, "Connection options:\n"
		"\t-U username\n"
		"\t-P password\n"
		"\t-h host\n"
		"\t-d database\n");
}

void pp_parse_connect_opt(char c, pp_connect_options * opts) {
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
		}
}

/*
 * askpass and set_echo are 'stolen' from boxfs,
 * I guess I can do it ;)
 */
void set_echo(int enable)
{
  struct termios tio;
  int tty = fileno(stdin); //a better way?

  if(!tcgetattr(tty, &tio)) {
    if (enable) tio.c_lflag |= ECHO;
    else tio.c_lflag &= ~ECHO;

    tcsetattr(tty, TCSANOW, &tio);
  }
}

char * askpass(const char * what)
{
  char * val = malloc(512);
  printf("%s ",what);
  set_echo(0);
  scanf("%s",val);
  set_echo(1); 
  printf("\n");
  return val;
}

#define PP_ADDPROP(NAME) if(opts->NAME) { strcat(buf, " " #NAME " = "); strcat(buf,opts->NAME); }
PGconn * pp_connect(pp_connect_options * opts)
{
	char buf[BUFSIZE]="";
	PP_ADDPROP(host);
	PP_ADDPROP(dbname);
	PP_ADDPROP(user);
	if(!opts->password) opts->password = askpass("password: ");
	PP_ADDPROP(password);
	
	return PQconnectdb(buf);
}