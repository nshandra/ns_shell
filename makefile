CC=gcc
#-Werror
CFLAGS= -Wall -Wshadow -g
DEPS = utils.h builtins.h ns_shell.h
OBJ = ns_shell.o utils.o builtins.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ns_shell: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)