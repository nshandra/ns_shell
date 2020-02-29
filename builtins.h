#include <unistd.h>
#include <stdlib.h>
#include <err.h>

int ns_help();
void splash(char *file);
int builtins_execute(cmd_line_data *cmd_line);