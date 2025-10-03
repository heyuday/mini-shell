#include "wsh.h"
#include "dynamic_array.h"
#include "utils.h"
#include "hash_map.h"
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int rc;
HashMap *alias_hm = NULL;
DynamicArray *history_da = NULL;
static int suppress_history = 0;

/***************************************************
 * Helper Functions
 ***************************************************/
/**
 * @Brief Free any allocated global resources
 */
void wsh_free(void)
{
  if (history_da != NULL)
  {
    da_free(history_da);
    history_da = NULL;
  }
  // Free any allocated resources here
  if (alias_hm != NULL)
  {
    hm_free(alias_hm);
    alias_hm = NULL;
  }
}

/**
 * @Brief Trim leading and trailing whitespace from a string
 */
void trim_whitespace(char *str)
{
  if (!str)
    return;

  // Trim leading whitespace
  char *start = str;
  while (*start && isspace((unsigned char)*start))
    start++;
  if (*start == '\0')
  {
    *str = '\0';
    return;
  }

  // Trim trailing whitespace
  char *end = start + strlen(start) - 1;
  while (end > start && isspace((unsigned char)*end))
    end--;
  *(end + 1) = '\0';

  // Shift the trimmed string to the beginning
  if (start != str)
    memmove(str, start, end - start + 2); // +2 to include null terminator
}

/**
 * @Brief Execute an external command using execv
 */
void execute_external_command(char **argv)
{
  char *cmd = argv[0];

  if (cmd[0] == '/' || (cmd[0] == '.' && cmd[1] == '/'))
  {
    if (access(cmd, X_OK) == 0)
    { // Happy(found the executable)
      execv(cmd, argv);
      perror("execv");
      // exit(EXIT_FAILURE);
      _exit(127);
    }
    else
    {
      fprintf(stderr, "Command not found or not an executable: %s\n", cmd);
      // exit(EXIT_FAILURE);
      _exit(127);
    }
  }

  char *path = getenv("PATH");
  if (!path || strlen(path) == 0)
  {
    fprintf(stderr, "PATH empty or not set\n");
    // exit(EXIT_FAILURE);
    _exit(127);
  }

  char *paths = strdup(path);
  char *dir = strtok(paths, ":");
  while (dir)
  {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
    if (access(fullpath, X_OK) == 0)
    {
      execv(fullpath, argv);
      perror("execv");
      free(paths);
      // exit(EXIT_FAILURE);
      _exit(127);
    }
    dir = strtok(NULL, ":");
  }
  fprintf(stderr, "Command not found or not an executable: %s\n", cmd);
  free(paths);
  // exit(EXIT_FAILURE);
  _exit(127);
}

/**
 * @Brief Handle cd built-in command
 */
int built_in_cd(int argc, char **argv)
{
  if (argc > 2)
  {
    fprintf(stderr, "Incorrect usage of cd. Correct format: cd | cd directory\n");
    return EXIT_FAILURE;
  }
  const char *dir = NULL;
  if (argc == 1)
  {
    dir = getenv("HOME");
    if (!dir)
    {
      fprintf(stderr, "cd: HOME not set\n");
      return EXIT_FAILURE;
    }
  }
  else
  {
    dir = argv[1];
  }
  if (chdir(dir) != 0)
  {
    perror("cd");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

/**
 * @Brief Handle path built-in command
 */
int built_in_path(int argc, char **argv)
{
  if (argc > 2)
  {
    fprintf(stderr, "Incorrect usage of path. Correct format: path dir1:dir2:...:dirN\n");
    return EXIT_FAILURE;
  }
  if (argc == 1)
  {
    const char *path = getenv("PATH");
    if (path != NULL)
    {
      printf("%s\n", path);
    }
    else
    {
      printf("\n");
      // print nothing if PATH is empty
    }

    fflush(stdout);
    return EXIT_SUCCESS;
  }

  if (setenv("PATH", argv[1], 1) != 0)
  {
    perror("setenv");
    return EXIT_FAILURE;
  }
  fflush(stdout);
  return EXIT_SUCCESS;
}
/**
 * @Brief Find the full path of a command
 */
static int find_in_path(const char *cmd, char *out, size_t outsz)
{
  const char *path = getenv("PATH");
  if (!path)
    return 0;

  char *paths = strdup(path);
  if (!paths)
  {
    perror("strdup");
    return 0;
  }

  int found = 0;
  for (char *dir = strtok(paths, ":"); dir; dir = strtok(NULL, ":"))
  {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/%s", dir, cmd);
    if (access(buf, X_OK) == 0)
    {
      strncpy(out, buf, outsz);
      out[outsz - 1] = '\0';
      found = 1;
      break;
    }
  }
  free(paths);
  return found;
}

/**
 * @Brief Check if a command is a built-in command
 */
int builtin_is_builtin_name(const char *name)
{
  /* Extend this list as you add more builtins */
  return !strcmp(name, "exit") || !strcmp(name, "cd") || !strcmp(name, "path") || !strcmp(name, "which") || !strcmp(name, "alias") || !strcmp(name, "unalias") || !strcmp(name, "history");
}

/**
 * @Brief Handle which built-in command
 */
int builtin_which(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Incorrect usage of which. Correct format: which name\n");
    return EXIT_FAILURE;
  }

  const char *name = argv[1];

  if (alias_hm)
  { // Alias
    char *aval = hm_get(alias_hm, name);
    if (aval)
    {
      /* wrap command in single quotes per spec */
      printf("%s: aliased to '%s'\n", name, aval);
      fflush(stdout);
      return EXIT_SUCCESS;
    }
  }

  if (builtin_is_builtin_name(name))
  { // builtin
    printf("%s: wsh builtin\n", name);
    fflush(stdout);
    return EXIT_SUCCESS;
  }

  if (name[0] == '/' || (name[0] == '.' && name[1] == '/'))
  { // absolute or relative path
    if (access(name, X_OK) == 0)
    {
      printf("%s: found at %s\n", name, name);
      fflush(stdout);
      return EXIT_SUCCESS;
    }
    else
    {
      printf("%s: not found\n", name);
      fflush(stdout);
      return EXIT_FAILURE;
    }
  }

  char full[1024]; // search in PATH
  if (find_in_path(name, full, sizeof(full)))
  {
    printf("%s: found at %s\n", name, full);
    fflush(stdout);
    return EXIT_SUCCESS;
  }

  printf("%s: not found\n", name); // not found
  return EXIT_FAILURE;
}

/**
 * Brief Handle alias built-in command
 */
int builtin_alias(int argc, char **argv)
{
  if (argc == 1)
  {
    hm_print_sorted(alias_hm);
    fflush(stdout);
    return EXIT_SUCCESS;
  }

  if (argc < 3)
  {
    fprintf(stderr, "Incorrect usage of alias. Correct format: alias | alias name = 'command'\n");
    return EXIT_FAILURE;
  }
  if (strcmp(argv[2], "=") != 0 || strcmp(argv[1], "=") == 0)
  {
    fprintf(stderr, "Incorrect usage of alias. Correct format: alias | alias name = 'command'\n");
    return EXIT_FAILURE;
  }

  char *val = NULL;
  if (argc == 3)
  { // alias name =
    val = strdup("");
  }
  else
  {
    val = strdup(argv[3]);
    for (int i = 4; i < argc; i++)
    {
      val = append(val, " ");
      val = append(val, argv[i]);
    }
    if (argc > 4 && (argv[3][0] != '\'' || argv[argc - 1][strlen(argv[argc - 1]) - 1] != '\''))
    {
      fprintf(stderr, "Incorrect usage of alias. Correct format: alias | alias name = 'command'\n");
      fflush(stdout);
      free(val);
      return EXIT_FAILURE;
    }
    // If user wrapped the whole thing in single quotes, strip them
    size_t L = strlen(val);
    if (L >= 2 && val[0] == '\'' && val[L - 1] == '\'')
    {
      val[L - 1] = '\0';
      memmove(val, val + 1, L - 1);
    }
  }

  hm_put(alias_hm, argv[1], val);
  fflush(stdout);
  free(val);
  return EXIT_SUCCESS;
}
/**
 * Brief Handle unalias built-in command
 */
int builtin_unalias(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Incorrect usage of unalias. Correct format: unalias name\n");
    return EXIT_FAILURE;
  }

  const char *name = argv[1];
  if (alias_hm)
  {
    hm_delete(alias_hm, name); // your hashmap delete function handles not-found gracefully
  }
  return EXIT_SUCCESS;
}
/**
 * Brief Handle history built-in command
 */
int builtin_history(int argc, char **argv)
{
  size_t effective = history_da ? history_da->size : 0;
  if (effective > 0)
    effective--;
  if (argc == 1)
  {
    for (size_t i = 0; i < effective; i++)
    {
      printf("%s\n", da_get(history_da, i));
    }
    fflush(stdout);
    return EXIT_SUCCESS;
  }

  if (argc != 2)
  {
    fprintf(stderr, "Incorrect usage of history. Correct format: history | history n\n");
    return EXIT_FAILURE;
  }

  // Parse integer
  char *endptr;
  long n = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0' || n < 1 || n > (long)history_da->size)
  {
    fprintf(stderr, "Invalid argument passed to history\n");
    return EXIT_FAILURE;
  }

  // Print nth command (1-based index)
  printf("%s\n", da_get(history_da, n - 1));
  fflush(stdout);
  return EXIT_SUCCESS;
}

/**
 * @Brief Process a command line
 */
void process_command(const char *cmdline)
{
  if (!cmdline)
    return;

  char *line = strdup(cmdline);
  if (!line)
  {
    perror("strdup");
    clean_exit(EXIT_FAILURE);
  }

  trim_whitespace(line);
  if (strlen(line) == 0)
  {
    free(line);
    return; // Ignore empty lines
  }

  int argc = 0;
  char *argv[MAX_ARGS];
  parseline_no_subst(line, argv, &argc);

  if (argc == 0)
  {
    free(line);
    return;
  }

  if (!suppress_history)
  {
    da_put(history_da, line);
  }
  if (strcmp(argv[0], "exit") == 0)
  {
    if (argc > 1)
    {
      fprintf(stderr, "Incorrect usage of exit. Too many arguments\n");
      rc = EXIT_FAILURE;
      goto cleanup;
    }
    else
    {
      clean_exit(rc);
    }
  }
  else if (strcmp(argv[0], "cd") == 0)
  {
    rc = built_in_cd(argc, argv);
    goto cleanup;
  }
  else if (strcmp(argv[0], "path") == 0)
  {
    rc = built_in_path(argc, argv);
    goto cleanup;
  }
  else if (strcmp(argv[0], "which") == 0)
  {
    rc = builtin_which(argc, argv);
    goto cleanup;
  }
  else if (strcmp(argv[0], "alias") == 0)
  {
    rc = builtin_alias(argc, argv);
    goto cleanup;
  }
  else if (strcmp(argv[0], "unalias") == 0)
  {
    rc = builtin_unalias(argc, argv);
    goto cleanup;
  }
  else if (strcmp(argv[0], "history") == 0)
  {
    rc = builtin_history(argc, argv);
    goto cleanup;
  }

  char *aval = hm_get(alias_hm, argv[0]);
  if (aval)
  {
    // rebuild: aval + (line after the first token)
    // find start and end of first token in 'line'
    char *p = line;
    while (*p && isspace((unsigned char)*p))
      p++;
    while (*p && !isspace((unsigned char)*p))
      p++;          // end of first token
    char *rest = p; // from p to end (including spaces)

    // new buffer: aval + rest
    size_t newlen = strlen(aval) + strlen(rest) + 1;
    char *expanded = malloc(newlen);
    if (!expanded)
    {
      perror("malloc");
      clean_exit(EXIT_FAILURE);
    }
    strcpy(expanded, aval);
    strcat(expanded, rest);

    for (int i = 0; i < argc; i++)
      free(argv[i]);
    free(line);
    suppress_history++;
    process_command(expanded);
    suppress_history--;
    free(expanded);
    return;
  }

  pid_t pid = fork();
  if (pid < 0)
  {
    perror("fork");
    rc = EXIT_FAILURE;
  }
  else if (pid == 0)
  {
    execute_external_command(argv);
  }
  else
  {
    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
      perror("waitpid");
      rc = EXIT_FAILURE;
    }
    else
    {
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        rc = EXIT_SUCCESS;
      else
        rc = EXIT_FAILURE;
    }
  }

cleanup:
  for (int i = 0; i < argc; i++)
    free(argv[i]);
  free(line);
}

/**
 * @Brief Cleanly exit the shell after freeing resources
 *
 * @param return_code The exit code to return
 */
void clean_exit(int return_code)
{
  wsh_free();
  exit(return_code);
}

/**
 * @Brief Print a warning message to stderr and set the return code
 *
 * @param msg The warning message format string
 * @param ... Additional arguments for the format string
 */
void wsh_warn(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);

  vfprintf(stderr, msg, args);
  va_end(args);
  rc = EXIT_FAILURE;
}

/**
 * @Brief Main entry point for the shell
 *
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return
 */
int main(int argc, char **argv)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  alias_hm = hm_create();
  history_da = da_create(10);
  setenv("PATH", "/bin", 1);
  if (argc > 2)
  {
    wsh_warn(INVALID_WSH_USE);
    return EXIT_FAILURE;
  }
  switch (argc)
  {
  case 1:
    interactive_main();
    break;
  case 2:
    rc = batch_main(argv[1]);
    break;
  default:
    break;
  }
  wsh_free();
  return rc;
}

/***************************************************
 * Modes of Execution
 ***************************************************/

/**
 * @Brief Interactive mode: print prompt and wait for user input
 * execute the given input and repeat
 */
void interactive_main(void)
{
  // TODO: Implement interactive mode here
  char line[1024];
  while (1)
  {
    printf("wsh> ");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
      if (feof(stdin))
      {
        printf("\n");
        clean_exit(rc); // Exit on EOF (Ctrl+D)
      }
      else
      {
        fprintf(stderr, "fgets error\n");
        continue; // Error reading input, prompt again
      }
    }
    process_command(line); // Call to helper function to process the command
  }
}

/**
 * @Brief Batch mode: read commands from script file line by line
 * execute each command and repeat until EOF
 *
 * @param script_file Path to the script file
 * @return EXIT_SUCCESS(0) on success, EXIT_FAILURE(1) on error
 */

int batch_main(const char *script_file)
{
  // TODO: Implement batch mode here
  FILE *file = fopen(script_file, "r");
  if (!file)
  {
    perror("fopen");
    return EXIT_FAILURE;
  }
  char line[1024];
  while (fgets(line, sizeof(line), file))
  {
    process_command(line); // Call to helper function to process the command
  }
  if (ferror(file))
  {
    fprintf(stderr, "Error reading file: %s\n", strerror(errno));
    fclose(file);
    return EXIT_FAILURE;
  }
  fclose(file);
  fflush(stdout);
  return rc;
}

/***************************************************
 * Parsing
 ***************************************************/

/**
 * @Brief Parse a command line into arguments without doing
 * any alias substitutions.
 * Handles single quotes to allow spaces within arguments.
 *
 * @param cmdline The command line to parse
 * @param argv Array to store the parsed arguments (must be preallocated)
 * @param argc Pointer to store the number of parsed arguments
 */
void parseline_no_subst(const char *cmdline, char **argv, int *argc)
{
  if (!cmdline)
  {
    *argc = 0;
    argv[0] = NULL;
    return;
  }
  char *buf = strdup(cmdline);
  if (!buf)
  {
    perror("strdup");
    clean_exit(EXIT_FAILURE);
  }
  /* Replace trailing newline with space */
  const size_t len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n')
    buf[len - 1] = ' ';
  else
  {
    char *new_buf = realloc(buf, len + 2);
    if (!new_buf)
    {
      perror("realloc");
      free(buf);
      clean_exit(EXIT_FAILURE);
    }
    buf = new_buf;
    strcat(buf, " ");
  }

  int count = 0;
  char *p = buf;
  while (*p && *p == ' ')
    p++; /* skip leading spaces */

  while (*p)
  {
    char *token_start = p;
    char *token = NULL;
    if (*p == '\'')
    {
      token_start = ++p;
      token = strchr(p, '\'');
      if (!token)
      {
        /* Handle missing closing quote - Print `Missing closing quote` to stderr */
        wsh_warn(MISSING_CLOSING_QUOTE);
        free(buf);
        for (int i = 0; i < count; i++)
          free(argv[i]);
        *argc = 0;
        argv[0] = NULL;
        return;
      }
      *token = '\0';
      p = token + 1;
    }
    else
    {
      token = strchr(p, ' ');
      if (!token)
        break;
      *token = '\0';
      p = token + 1;
    }
    argv[count] = strdup(token_start);
    if (!argv[count])
    {
      perror("strdup");
      for (int i = 0; i < count; i++)
        free(argv[i]);
      free(buf);
      clean_exit(EXIT_FAILURE);
    }
    count++;
    while (*p && (*p == ' '))
      p++;
  }
  argv[count] = NULL;
  *argc = count;
  free(buf);
}
