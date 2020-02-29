#include "utils.h"

void *
u_calloc(size_t n, size_t type_size)
{
	void *p;
	if ((p = calloc(n, type_size)) == NULL) {
		err(1, "ut_calloc: ");
	}
	return p;
}

void *
u_realloc(void *ptr, int size)
{
	void *p;
	if ((p = realloc(ptr, size)) == NULL) {
		err(1, "u_realloc: ");
	}
	return p;
}

void
u_pipe(int* fd)
{
	if (pipe(fd) < 0) {
		err(1, "u_pipe: pipe failed");
	}
}


void
u_free_cmd_line (cmd_line_data *cmd_line)
{
	int i, j;
	for (i = 0; i <= cmd_line->cmd_index; ++i) {
		for (j = 0; j <= cmd_line->cmd_v[i]->index; ++j) {
			free(cmd_line->cmd_v[i]->arg_v[j]);
		}
		free(cmd_line->cmd_v[i]->arg_v);
		free(cmd_line->cmd_v[i]);
	}
	free(cmd_line->cmd_v);
}

void
u_asprint(char **strp, char *in_str) {
	if (asprintf(strp, "%s", in_str) == -1) {
		err(1, "u_asprint: memory allocation wasn't possible.");
	}
}

char *
whitespace_trim(char *line)
{
	while(isspace((unsigned char)*line)) {
		line++;
	}

	if(*line == 0)
	return line;

	char *end_p = line + strlen(line) - 1;
	while(end_p > line && isspace((unsigned char)*end_p)) {
		end_p--;
	}

	*++end_p = 0;

	return line;
}

void
tail_whitespace_trim(char *line)
{
	char *end_p = line + strlen(line) - 1;
	while(end_p > line && isspace((unsigned char)*end_p)) {
		end_p--;
	}
	*++end_p = 0;
}

int
varname_check(char *line)
{
	char *char_p = line;
	int line_len = strlen(line);

	for (int i = 0; i < line_len; ++i) {
		if ((isalnum((unsigned char) char_p[i])) == 0 && char_p[i] != '=') {
			return 1;
		}
	}
	return 0;
}

void
print_cmd_line(cmd_line_data cmd_line)
{
	printf("------------\n");
	printf("print_cmd_line\n");
	printf("------------\n");
	printf("cmd_line.file_redir_flag: %i\n", cmd_line.file_redir_flag);
	printf("------------\n");
	printf("cmd_line.file_redir[0]: !%i!\n", cmd_line.file_redir[0]);
	printf("------------\n");
	printf("cmd_line.file_redir[1]: !%i!\n", cmd_line.file_redir[1]);
	printf("------------\n");

	printf("cmd_line.no_stdin_flag: %i\n", cmd_line.no_stdin_flag);
	printf("------------\n");
	printf("cmd_line.here_document_flag: %i\n", cmd_line.here_document_flag);
	printf("------------\n");
	printf("cmd_line.store_env_flag: %i\n", cmd_line.store_env_flag);
	printf("------------\n");

	if (cmd_line.store_env_flag == 0) {
		printf("cmd_line.store_env_name: %s\n", cmd_line.store_env_name);
		printf("------------\n");
	}

	printf("cmd_line.if_flag: %i\n", cmd_line.if_flag);
	printf("------------\n");
	printf("cmd_line.exit_flag: %i\n", cmd_line.exit_flag);
	printf("------------\n");
	printf("cmd_line.cmd_index: %i\n", cmd_line.cmd_index);
	printf("------------\n");

	int i, j;
	for (i = 0; i <= cmd_line.cmd_index; ++i) {
		for (j = 0; j <= cmd_line.cmd_v[i]->index; ++j) {
			printf("cmd_line.cmd_v[%i][%i]: !%s!\n", i, j, cmd_line.cmd_v[i]->arg_v[j]);
			printf("------------\n");
		}
	}
}