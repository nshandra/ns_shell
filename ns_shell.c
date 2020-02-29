#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include "utils.h"
#include "builtins.h"

#define INPUT_BUF_LEN 256
#define HEREDOC_SIZE 4096
#define OUTPUT_BUF_SIZE 4096
#define ARGV_LEN 16
#define WS_DELIM " \t\r\n\a"
#define FIN_CHAR '<'
#define FOUT_CHAR '>'
#define NOSTDIN_CHAR '&'
#define HEREDOC_START_CHAR '['
#define HEREDOC_END_CHAR ']'
#define SET_ENV_CHAR '='
#define GET_ENV_CHAR '$'
#define STORE_ENV_CHAR '%'
#define SYNTAX_ERROR "Syntax error. Use 'help' for usage."
#define RESULT_ENV "RESULT"

#define uniq_char(line, char) (strchr(line, char) == strrchr(line, char))
#define get_uniq_char(char_p, line, char) ((char_p = strchr(line, char)) == strrchr(line, char))

static int getenv_err = 0;

// 
// UTILS
// 

void
cmd_line_flags_init(cmd_line_data *cmd_line)
{
	cmd_line->file_redir_flag = 1;
	cmd_line->file_redir[0] = -1;
	cmd_line->file_redir[1] = -1;
	cmd_line->no_stdin_flag = 1;
	cmd_line->here_document_flag = 1;
	cmd_line->store_env_flag = 1;
	cmd_line->if_flag = 1;
	cmd_line->exit_flag = 1;
}

int
flag_check(cmd_line_data cmd_line, char flag)
{
	switch (flag) {
		case HEREDOC_START_CHAR:
		if (cmd_line.file_redir_flag
			&& cmd_line.no_stdin_flag
			&& cmd_line.store_env_flag) {
			return 1;
		}
		break;
		case STORE_ENV_CHAR:
		if (cmd_line.file_redir_flag
			&& cmd_line.no_stdin_flag
			&& cmd_line.here_document_flag) {
			return 1;
		}
		break;
		default:
		if (cmd_line.file_redir_flag
			&& cmd_line.no_stdin_flag) {
			return 1;
		}
	}
	return 0;
}

// 
// GET LINE
// 

char *
get_line()
{
	char *line_buf = NULL;
	size_t len = 0;
	ssize_t n_read;

		if (isatty(fileno(stdin))) {
			// input prompt
			printf("> ");
		} else {
			// reset after isatty
			errno = 0;
		}

		if ((n_read = getline(&line_buf, &len, stdin)) != -1) {
			// descarta enter
			if (line_buf[n_read-1] == '\n') {
				line_buf[n_read-1] = '\0';
			}
		} else {
			if (errno == 0) {
				errx(1, "EOF");
			} else {
				err(1, "get_line: ");
			}
		}
	return line_buf;
}

// 
// FILE REDIR
// 

int
file_redir_check(char *line)
{
	char *char_p, *char_p2;
	
	if (get_uniq_char(char_p, line, FIN_CHAR) && get_uniq_char(char_p2, line, FOUT_CHAR)) {
		if ((char_p != NULL) || (char_p2 != NULL)) {
			return 0;
		}
		return 1;
	} else {
		fprintf(stderr, "file_redir_check: %s\n", SYNTAX_ERROR);
		return -1;
	}
}

int
file_redir_parse(char *line, cmd_line_data *cmd_line)
{
	char *fr_path[2];

	fr_path[0] = strchr(line, FIN_CHAR);
	fr_path[1] = strchr(line, FOUT_CHAR);

	// make substrings
	if (fr_path[0] != NULL) {
		*fr_path[0]++ = 0;
	}

	if (fr_path[1] != NULL) {
		*fr_path[1]++ = 0;
	}
	// trim & open
	if (fr_path[0] != NULL) {
		fr_path[0] = whitespace_trim(fr_path[0]);
		if ((cmd_line->file_redir[0] = open(fr_path[0], O_RDONLY)) == -1) {
			fprintf(stderr, "file_redir: wrong < path\n");
			return -1;
		}
	}

	if (fr_path[1] != NULL) {
		fr_path[1] = whitespace_trim(fr_path[1]);
		if ((cmd_line->file_redir[1] = open(fr_path[1], O_CREAT 
											| O_WRONLY | O_TRUNC, 
											0664)) == -1) {
		fprintf(stderr, "file_redir: wrong > path\n");
		return -1;
		}
	}
	return 0;
}

// 
// NO STDIN CHECK
// 

int
no_stdin_flag_check(char *line)
{
	int last_char_pos = strlen(line)-1;
	if (uniq_char(line, NOSTDIN_CHAR)) {
		if ((line[last_char_pos] == NOSTDIN_CHAR)) {
			line[last_char_pos] = 0;
			return 0;
		} else {
			return 1;
		}
	} else {
		fprintf(stderr, "no_stdin_flag_check: %s\n", SYNTAX_ERROR);
		return -1;
	}
}

// 
// MOD 1: HERE DOC
// 

int
hd_start_check(char *line, cmd_line_data *cmd_line)
{
	int last_char_pos = strlen(line)-1;
	if (uniq_char(line, HEREDOC_START_CHAR)) {	
		if (line[last_char_pos] == HEREDOC_START_CHAR) {
			line[last_char_pos] = '\0';
			return 0;
		}
	}
	return 1;
}

int
hd_end_check(char *line)
{
	if (line[0] == HEREDOC_END_CHAR && strlen(line) < 3) {
		line[0] = '\0';
		return 0;
	} else {
		return 1;
	}
}

int
here_document(int fd)
{
	char *line;
	int hd_end_flag = 1, n_print = 0;

	// read until ] or pipe full
	while (hd_end_flag != 0) {
		line = get_line();
		// check ]
		if ((hd_end_flag = hd_end_check(line)) != 0) {
			// ïf not pipe full
			if ((n_print + strlen(line)) <= HEREDOC_SIZE) {
				n_print += dprintf(fd, "%s\n", line);
				// reached pipe buff size
				if (n_print == HEREDOC_SIZE) {
					fprintf(stderr, "here_document: limit reached.\n");
					hd_end_flag = 0;
				}
			} else {
				fprintf(stderr, "here_document: limit reached, last line discarded.\n");
				hd_end_flag = 0;
			}
		}
	}

	fflush(stdin);
	close(fd);

	return 0;
}

// 
// MOD 2: ENVAR
// 

int
set_en_var(char *line)
{
	char *env_value;

	if (!get_uniq_char(env_value, line, SET_ENV_CHAR)) {
		fprintf(stderr, "set_en_var: %s\n", SYNTAX_ERROR);
		return -1;
	}

	if (env_value != NULL) {

		tail_whitespace_trim(line);

		if (varname_check(line) == 0) {
			*env_value++ = 0;

			if (setenv(line, env_value, 1) == -1) {
				err(1, "set_en_var: ");
			}
			return 0;
		} else {
			fprintf(stderr, "set_en_var: %s\n", SYNTAX_ERROR);
			return -1;
		}
	}
	return 1;
}

int
legal_var_invoc(char *line)
{
	char *char_p;
	if (!get_uniq_char(char_p, line, GET_ENV_CHAR)) {
		fprintf(stderr, "legal_var_invoc: %s\n", SYNTAX_ERROR);
		return -1;
	}
	if (char_p != NULL) {
		return 0;
	}
	return 1;
}

void
get_en_var(char *line, char **env_p)
{
	char *env_value;

	*line++ = 0;

	if ((env_value = getenv(line)) == NULL) {
		fprintf(stderr, "%s: No such variable.\n", line);
		getenv_err = 1;
	} else {
		u_asprint(env_p, env_value);
	}
}

// 
//  MOD 3 OUTPUT TO ENVAR
// 

int
store_env_flag_check(char *line)
{
	char *char_p;
	if (get_uniq_char(char_p, line, STORE_ENV_CHAR)) {
		if (char_p != NULL) {
			return 0;
		} else {
			return 1;
		}
	} else {
		fprintf(stderr, "store_env_flag_check: %s\n", SYNTAX_ERROR);
		return -1;
	}
}

char *
store_env_parse(char *line)
{
	char *new_line = strchr(line, STORE_ENV_CHAR);

	if (new_line != NULL) {

		printf("store_env_parse\n");

		*new_line++ = 0;

		tail_whitespace_trim(line);

		if (varname_check(line) == 0) {
			return new_line;
		} else {
			fprintf(stderr, "store_env_parse2: %s\n", SYNTAX_ERROR);
			return 0;
		}
	}
	fprintf(stderr, "store_env_parse new_line == NULL\n");
	return 0;
}


void
output_to_env(int fd, char *name)
{
	char *output_buf;
	int n_read = 0, count = 0;
	output_buf = u_calloc(OUTPUT_BUF_SIZE, sizeof(char));

	// read
	while ((count = read(fd, output_buf, OUTPUT_BUF_SIZE)) > 0) {
		n_read += count;
		if (n_read > OUTPUT_BUF_SIZE) {
			fprintf(stderr, "output_to_env: limit reached, output discarded.\n");
			break;
		}
	}
	// check for read error
	if (count < 0) {
		err(1, "output_to_env: ");
	}
	// set env
	if (n_read <= OUTPUT_BUF_SIZE) {
		printf("output_buf: %s\n", output_buf);
		if (setenv(name, output_buf, 1) == -1) {
			err(1, "output_to_env: ");
		}
	}

	close(fd);
	free(output_buf);
}

// 
//  MOD 4 STATUS & BUILTINS
// 

void
set_result(int result)
{
	char res_s[32];
	sprintf(res_s, "%i", result);
	if (setenv(RESULT_ENV, res_s, 1) == -1) {
		err(1, "set_result: ");
	}
}

int
get_result()
{
	char *res_val;
	if ((res_val = getenv(RESULT_ENV)) == NULL) {
		err(1, "get_result: ");
	}
	return atoi(res_val);
}

// 
//  LINE PARSE
// 

cmd_data *
whitespace_parse(char *line)
{
	int index = 0, alloc_size = ARGV_LEN;
	char *c_ptr, *saveptr;

	// data alloc
	cmd_data *cmd = u_calloc(1, sizeof(cmd_data));
	cmd->arg_v = u_calloc(ARGV_LEN, sizeof(char *));

	// tokenize
	c_ptr = strtok_r(line, WS_DELIM, &saveptr);
	do {
		// realloc
		if (index == alloc_size) {
			alloc_size += ARGV_LEN;
			cmd->arg_v = u_realloc(cmd->arg_v, (alloc_size * sizeof(char *)));
		}

		if (legal_var_invoc(c_ptr) == 0) {
			// get en var value
			get_en_var(c_ptr, cmd->arg_v+index);
		} else {
			// copy tosken
			u_asprint(&cmd->arg_v[index], c_ptr);
		}
		index++;
	} while ((c_ptr = strtok_r(NULL, WS_DELIM, &saveptr)) != NULL);

	cmd->index = index - 1;
	return cmd;
}

int
pipe_parse(char *line, cmd_line_data *cmd_line)
{
	int index = 0, alloc_size = ARGV_LEN;
	char *c_ptr, *saveptr;

	// data alloc
	cmd_data **cmd_v = u_calloc(ARGV_LEN, sizeof(cmd_data *));

	// tokenize
	c_ptr = strtok_r(line, "|", &saveptr);
	do {
		// realloc
		if (index == alloc_size) {
			alloc_size += ARGV_LEN;
			cmd_v = u_realloc(cmd_v, (alloc_size * sizeof(cmd_data *)));
		}
		// parse token
		cmd_v[index] = whitespace_parse(c_ptr);
		index++;
	} while ((c_ptr = strtok_r(NULL, "|", &saveptr)) != NULL);

	cmd_line->cmd_index = index - 1;
	cmd_line->cmd_v = cmd_v;
	return 0;
}

// 
// LAUNCH
// 

void
redir_stdin(cmd_line_data cmd_line)
{
	if (cmd_line.file_redir[0] >= 0) {
		dup2(cmd_line.file_redir[0], 0);

	// & without <
	} else if (cmd_line.no_stdin_flag == 0) {
		int null_fd = open("/dev/null", O_WRONLY);
		dup2(null_fd, 0);
		close(null_fd);
	} 
}

void
redir_stdout(cmd_line_data cmd_line)
{
	if (cmd_line.file_redir[1] >= 0) {
		dup2(cmd_line.file_redir[1], 1);
	}
}

int
launch(cmd_line_data cmd_line)
{
	pid_t pid[cmd_line.cmd_index + 1];
	int fd[cmd_line.cmd_index][2];
	int s_fd[2];
	int hd_fd[2];
	int status;

	// make | fd
	for (int i = 0; i < cmd_line.cmd_index; ++i) {
		u_pipe(fd[i]);
	}

	// here doc pipe
	if ((cmd_line.here_document_flag == 0)) {
		u_pipe(hd_fd);
		cmd_line.file_redir[0] = hd_fd[0];
	}

	// here doc write doc
	if (cmd_line.here_document_flag == 0) {
		here_document(hd_fd[1]);
	}

	// store out pipe
	if (cmd_line.store_env_flag == 0) {
		u_pipe(s_fd);
		cmd_line.file_redir[1] = s_fd[1];
	}

	// main loop
	for (int i = 0; i <= cmd_line.cmd_index; ++i) {

		pid[i] = fork();
		switch(pid[i]){
		case -1:
			warn("fork error\n");
			return -1;
		// Child process
		case 0:
			// stdin & out redir
			if (i == 0) {
				redir_stdin(cmd_line);
			}

			if (i == cmd_line.cmd_index) {
				redir_stdout(cmd_line);
			}

			// if |
			if (cmd_line.cmd_index > 0) {
				// connect pipe
				if (i == 0) {
					dup2(fd[i][1], 1);
				} else if (i == cmd_line.cmd_index) {
					dup2(fd[i-1][0], 0);
				} else {
					dup2(fd[i-1][0], 0);
					dup2(fd[i][1], 1);
				}

				// cierra los pipes
				for (int j = 0; j < cmd_line.cmd_index; ++j) {
					close(fd[j][0]);
					close(fd[j][1]);
				}

				// cierra pipe here doc
				if (cmd_line.here_document_flag == 0) {
					close(hd_fd[0]);
					close(hd_fd[1]);
				}

				// cierra pipe store
				if (cmd_line.store_env_flag == 0) {
					close(s_fd[0]);
					close(s_fd[1]);
				}
			}

			// ifok & ifnot: ignore first arg
			if (cmd_line.if_flag == 0) {
				if (execvp(cmd_line.cmd_v[i]->arg_v[1], cmd_line.cmd_v[i]->arg_v+1) == -1) {
					// better warnings
					err(1, "%s ", cmd_line.cmd_v[i]->arg_v[1]);
				}
			} else {
				if (execvp(cmd_line.cmd_v[i]->arg_v[0], cmd_line.cmd_v[i]->arg_v) == -1) {
					// better warnings
					err(1, "%s ", cmd_line.cmd_v[i]->arg_v[0]);
				}
			}
		default:
			// Parent process
			continue;
		}
	}

	// close | pipes
	for (int i = 0; i < cmd_line.cmd_index; ++i) {
		close(fd[i][0]);
		close(fd[i][1]);
	}

	// close in/out redir
	if (cmd_line.file_redir[0] >= 0) {
		close(cmd_line.file_redir[0]);
	}

	if (cmd_line.file_redir[1] >= 0) {
		close(cmd_line.file_redir[1]);
	}

	// store out in envar
	if (cmd_line.store_env_flag == 0) {
		output_to_env(s_fd[0], cmd_line.store_env_name);
	}

	// wait forks
	for (int i = 0; i <= cmd_line.cmd_index; ++i) {
		waitpid(pid[i], &status, WUNTRACED);
	}
	return WEXITSTATUS(status);
}

void
input_loop()
{
	char *line_buf;
	cmd_line_data cmd_line;
	int status = 0;

	// RESULT_ENV init
	set_result(status);

	do {
		// set flags
		cmd_line_flags_init(&cmd_line);

		line_buf = get_line();

		// check empty buffer
		if (line_buf[0] == 0) {
			free(line_buf);
			continue;
		}

		// check =
		if (set_en_var(line_buf) != 1) {
			free(line_buf);
			continue;
		}

		// check &
		if ((cmd_line.no_stdin_flag = no_stdin_flag_check(line_buf)) == -1) {
			free(line_buf);
			continue;
		}

		// check <>
		if ((cmd_line.file_redir_flag = file_redir_check(line_buf)) == -1) {
			free(line_buf);
			continue;
		};

		// check [
		if (flag_check(cmd_line, HEREDOC_START_CHAR)) {
			cmd_line.here_document_flag = hd_start_check(line_buf, &cmd_line);
		}

		// check x % ...
		if ((cmd_line.store_env_flag = store_env_flag_check(line_buf)) == 0) {
			if (flag_check(cmd_line, STORE_ENV_CHAR)) {
				// env name
				cmd_line.store_env_name = line_buf;
				// cmd line start
				line_buf = store_env_parse(line_buf);
			} else {
				fprintf(stderr, "%s\n", SYNTAX_ERROR);
				free(line_buf);
				continue;
			}
		}

		if (cmd_line.file_redir_flag == 0) {
			file_redir_parse(line_buf, &cmd_line);
		}

		pipe_parse(line_buf, &cmd_line);

		if (getenv_err == 1) {
			free(line_buf);
			u_free_cmd_line(&cmd_line);
			getenv_err = 0;
			continue;
		}

		// print_cmd_line(cmd_line);

		if (builtins_execute(&cmd_line) != 0) {
			status = launch(cmd_line);
			set_result(status);
		}

		if (cmd_line.store_env_flag == 0) {
			free(cmd_line.store_env_name);
		} else {
			free(line_buf);
		}
		u_free_cmd_line(&cmd_line);

	} while(cmd_line.exit_flag != 0);
}

int
main(int argc, char const *argv[])
{
	splash("splash.txt");
	ns_help();
	input_loop();
	return 0;
}