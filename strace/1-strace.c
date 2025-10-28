#include "syscalls.h"

/**
 * main - entry
 * @argc: args count
 * @argv: args
 * Return: exit or fail
 */
int main(int argc, char **argv)
{
	pid_t child_pid;
	int status, syscall, flip;
	struct user_regs_struct u_in;

	setbuf(stdout, 0);
	child_pid = fork();

	if (child_pid < 0)
		exit(-1);
	else if (child_pid == 0)
	{
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);
		argv[argc] = NULL;
		execvp(argv[1], argv + 1);

	}
	else
	{
		wait(&status);
		ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
		for (flip = 0; !WIFEXITED(status); flip ^= 1)
		{
			wait(&status);
			ptrace(PTRACE_GETREGS, child_pid, 0, &u_in);
			if (flip)
			{
				syscall = u_in.orig_rax;
				printf("%s", syscalls_64_g[syscall].name);
			}
			if (!flip && (long)u_in.rax != -38)
				printf("\n");
			ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
		}
		printf("\n");
	}
	return (0);
}
