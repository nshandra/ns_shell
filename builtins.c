#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "ns_shell.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int ns_help();
int ns_exit(cmd_line_data *cmd_line);
int chd(cmd_line_data *cmd_line);
int ifok(cmd_line_data *cmd_line);
int ifnot(cmd_line_data *cmd_line);
int ns_true();
int ns_false();

char *builtin_str[] = {"chd", "help", "exit", "ifok", "ifnot", "true", "false"};

int (*builtin_func[]) (cmd_line_data *cmd_line) = {
	&chd,
	&ns_help,
	&ns_exit,
	&ifok,
	&ifnot,
	&ns_true,
	&ns_false
};

int
ns_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char *);
}

int
chd(cmd_line_data *cmd_line)
{
	
	if (cmd_line->cmd_v[0]->arg_v[1] == NULL) {
		if (chdir(getenv("HOME")) != 0) {
			warn("chd");
		}
	} else if (strcmp(cmd_line->cmd_v[0]->arg_v[1], "-") == 0) {
		printf("%s\n", getenv("HOME"));
		if (chdir(getenv("HOME")) != 0) {
			warn("chd");
		}
	} else {
		if (chdir(cmd_line->cmd_v[0]->arg_v[1]) != 0) {
			warn("chd");
		}
	}
	return 0;
}

int
ns_help(void)
{
	printf("These shell commands are built ins:\n");

	for (int i = 0; i < ns_num_builtins(); ++i) {
		printf("  %s\n", builtin_str[i]);
	}
	printf("Control characters: | < > & = %%\n");
	printf("Use 'apropos' and 'man' to find out more about commands not in this list.\n");

	return 0;
}

void
splash(char *file)
{
	FILE * fd;
	int len = 60;
	char buf[len];

	if ((fd = fopen(file, "r")) == NULL) {
		err(1, "splash");
	}

	while (fgets(buf, len, fd) != NULL) {
		printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, buf);
	}
	fclose(fd);
	printf("NS Shell - version 1.0 - Nazar Shandra\n");
	printf("---------------------------------------------\n");
}

int
ns_exit(cmd_line_data *cmd_line)
{
	cmd_line->exit_flag = 0;
	return 0;
}

int
ifok(cmd_line_data *cmd_line)
{
	int status;

	if (get_result() == 0) {
		cmd_line->if_flag = 0;
		status = launch(*cmd_line);
		set_result(status);
	} else {
		set_result(0);
	}
	return 0;
}

int
ifnot(cmd_line_data *cmd_line)
{
	int status;

	if (get_result() == 1) {
		cmd_line->if_flag = 0;
		status = launch(*cmd_line);
		set_result(status);
	} else {
		set_result(0);
	}
	return 0;
}

int
ns_true()
{
	set_result(0);
	return 0;
}

int
ns_false()
{
	set_result(1);
	return 0;
}

int
builtins_execute(cmd_line_data *cmd_line)
{
	if (cmd_line->cmd_v[0]->arg_v[0] == NULL) {
		return 1;
	}

	for (int i = 0; i < ns_num_builtins(); ++i) {
		if (strcmp(cmd_line->cmd_v[0]->arg_v[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(cmd_line);
		}
	}

	return 1;
}
