/*
 * win32/ezx_main.c
 * The following code is derived from SDL-1.2.7/main/win32/SDL_win32_main.c.
 * This file is placed in the public domain.
 */

#include <windows.h>
#include <malloc.h>
#include <ctype.h>

int ezx_main(int argc, char *argv[]);

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
  char *bufp;
  int argc;

  argc = 0;
  for (bufp = cmdline; *bufp;) {
    /* Skip leading Whitespace */
    while (isspace(*bufp)) {
      ++bufp;
    }
    /* Skip over argument */
    if (*bufp == '"') {
      ++bufp;
      if (*bufp) {
	if (argv) {
	  argv[argc] = bufp;
	}
	++argc;
      }
      /* Skip over word */
      while (*bufp && (*bufp != '"')) {
	++bufp;
      }
    } else {
      if (*bufp) {
	if (argv) {
	  argv[argc] = bufp;
	}
	++argc;
      }
      /* Skip over word */
      while (*bufp && !isspace(*bufp)) {
	++bufp;
      }
    }
    if (*bufp) {
      if (argv) {
	*bufp = '\0';
      }
      ++bufp;
    }
  }
  if (argv) {
    argv[argc] = NULL;
  }
  return argc;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  char *bufp;
  char *cmdline;
  int argc, ret;
  char **argv;

  bufp = GetCommandLine();
  cmdline = malloc(strlen(bufp) + 1);
  strcpy(cmdline, bufp);

  argc = ParseCommandLine(cmdline, NULL);
  argv = malloc((argc + 1) * (sizeof *argv));
  ParseCommandLine(cmdline, argv);

  ret = ezx_main(argc, argv);

  free(cmdline);
  free(argv);

  return ret;
}
