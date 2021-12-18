
#include <stdio.h>

#include "cli_cmd.h"
#include "cli_util.h"
#include "cli_config.h"

int main (int argc, char *argv[])
{
	char *cmd[] = { "show version", "show config", NULL };
	char buf[16];
	int i;

	for (i = 0 ; cmd[i] ; i++)
	{
		printf("NRC> %s\n", cmd[i]);
		snprintf(buf, sizeof(buf), "%s", cmd[i]);
		if (cli_app_run_command(buf) == 0)
			printf("OK\n");
		else
			printf("FAIL\n");
	}

	return 0;
}
