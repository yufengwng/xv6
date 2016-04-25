// EECE 7376 Operating Systems
// Final Project
//
// xray - examine inside the xv6 file system

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

#define STDOUT 1
#define STDERR 2
#define BUFSIZE 512

static uint iswhitespace(char c) {
  return (c == '\n' || c == '\r' || c == '\f'
      || c == '\t' || c == '\v' || c == ' ');
}

static void trim_right(char *buf) {
  uint last = strlen(buf) - 1;
  while (iswhitespace(buf[last])) {
    buf[last--] = '\0';
  }
}

void print_dir(int lvl, char *path) {
  int i;
  int fd;

  char name[BUFSIZE];
  char *ptr;

  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    return;
  }

  strcpy(name, path);
  ptr = name + strlen(name) - 1;
  if (*ptr != '/') {
    ptr++;
    *ptr = '/';
  }
  ptr++;

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) {
      continue;
    }

    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
      continue;
    }

    memmove(ptr, de.name, DIRSIZ);
    ptr[DIRSIZ] = '\0';
    if (stat(name, &st) < 0) {
      continue;
    }

    if (st.type == T_DEV) {
      continue;
    }

    for (i = 0; i < lvl; i++) {
      printf(STDOUT, "   ");
    }
    printf(STDOUT, "|- ");

    printf(STDOUT, "%s\n", de.name);
    if (st.type == T_DIR) {
      print_dir(lvl + 1, name);
    }
  }

  close(fd);
}

void exec_tree(char *path) {
  int fd;
  struct stat st;

  if (strlen(path) == 0) {
    printf(STDOUT, "usage: tree DIR\n");
    return;
  }

  if ((fd = open(path, 0)) < 0) {
    printf(STDERR, "tree: cannot open path '%s'\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    printf(STDERR, "tree: cannot stat path '%s'\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    printf(STDERR, "tree: not a directory at path '%s'\n", path);
    close(fd);
    return;
  }

  close(fd);
  printf(STDOUT, "%s\n", path);
  print_dir(1, path);
  printf(STDOUT, "\n");
}

void interpret(char *cmd) {
  int i;
  char *arg = "";

  for (i = 0; i < strlen(cmd) - 1; i++) {
    if (cmd[i] == ' ') {
      cmd[i] = '\0';
      arg = &cmd[i+1];
      break;
    }
  }

  if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
    exit();
  } else if (strcmp(cmd, "tree") == 0) {
    exec_tree(arg);
  }
}

int main(int argc, char *argv[]) {
  char *prompt = ">> ";
  char buf[BUFSIZE];

  while (1) {
    printf(STDOUT, "%s", prompt);
    gets(buf, BUFSIZE);

    trim_right(buf);
    if (strlen(buf) == 0) {
      continue;
    }

    interpret(buf);
  }

  exit();
}
