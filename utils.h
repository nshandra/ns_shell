#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

typedef struct cmd_data cmd_data;
struct cmd_data {
	int index;
	char **arg_v;
};

typedef struct cmd_line_data cmd_line_data;
struct cmd_line_data {
	int file_redir_flag;
	int file_redir[2];
	int no_stdin_flag;
	int here_document_flag;
	int store_env_flag;
	char *store_env_name;
	int if_flag;
	int exit_flag;
	int cmd_index;
	cmd_data **cmd_v;
};

void *u_calloc(size_t n, size_t type_size);
void *u_realloc(void *ptr, int size);
void u_pipe(int* fd);
void u_free_cmd_line (cmd_line_data *cmd_line);
void u_asprint(char **strp, char *in_str);
char *whitespace_trim(char *line);
void tail_whitespace_trim(char *line);
int varname_check(char *line);
void print_cmd_line(cmd_line_data cmd_line);