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
#define DEPTH 10

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

  char buf[DIRSIZ+1];
  char name[BUFSIZE];
  char *ptr;

  struct dirent de;
  struct stat st;

  if (lvl > DEPTH) {
    return;
  }

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

    memmove(buf, de.name, DIRSIZ);
    buf[DIRSIZ] = '\0';

    printf(STDOUT, "%s\n", buf);
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
    path = "./";
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

void exec_list(char *path) {
  int fd;
  int pad;
  struct stat st;
  struct dirent de;
  char buf[DIRSIZ+1];

  if (strlen(path) == 0) {
    path = "./";
  }

  if ((fd = open(path, 0)) < 0) {
    printf(STDERR, "list: cannot open path '%s'\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    printf(STDERR, "list: cannot stat path '%s'\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    printf(STDERR, "list: not a directory at path '%s'\n", path);
    close(fd);
    return;
  }

  pad = DIRSIZ - 4;
  printf(STDOUT, "name");
  while (pad-- > 0) {
    printf(STDOUT, " ");
  }
  printf(STDOUT, " inumber\n");

  for (pad = 0; pad < DIRSIZ; pad++) {
    printf(STDOUT, "-");
  }
  printf(STDOUT, "--------\n");

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) {
      continue;
    }

    memmove(buf, de.name, DIRSIZ);
    buf[DIRSIZ] = '\0';

    pad = DIRSIZ - strlen(buf);
    printf(STDOUT, "%s", buf);
    while (pad-- > 0) {
      printf(STDOUT, " ");
    }
    printf(STDOUT, " %d\n", de.inum);
  }
  printf(STDOUT, "\n");

  close(fd);
}

void exec_stat(char *path) {
  int fd;
  struct stat st;

  if (strlen(path) == 0) {
    printf(STDOUT, "usage: stat PATH\n");
    return;
  }

  if ((fd = open(path, 0)) < 0) {
    printf(STDERR, "stat: cannot open path '%s'\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    printf(STDERR, "stat: cannot stat path '%s'\n", path);
    close(fd);
    return;
  }

  printf(STDOUT, "%s\n", path);
  printf(STDOUT, "--------\n");

  if (st.type == T_DIR) {
    printf(STDOUT, "   type: dir\n");
  } else if (st.type == T_FILE) {
    printf(STDOUT, "   type: file\n");
  } else {
    printf(STDOUT, "   type: dev\n");
  }
  printf(STDOUT, " device: %d\n", st.dev);
  printf(STDOUT, "inumber: %d\n", st.ino);
  printf(STDOUT, "  links: %d\n", st.nlink);
  printf(STDOUT, "   size: %d\n", st.size);
  printf(STDOUT, "\n");
}

void exec_info(char *arg) {
  struct superblock sb;

  if (strlen(arg) == 0) {
    return;
  }

  if (strcmp(arg, "super") == 0) {
    if (readsb(&sb) < 0) {
      printf(STDOUT, "info: cannot get superblock info\n");
      return;
    }

    printf(STDOUT, "superblock\n");
    printf(STDOUT, "-----------------\n");
    printf(STDOUT, "         fs blks: %d\n", sb.size);
    printf(STDOUT, "   num data blks: %d\n", sb.nblocks);
    printf(STDOUT, "      num inodes: %d\n", sb.ninodes);
    printf(STDOUT, "    num log blks: %d\n", sb.nlog);
    printf(STDOUT, "   log start blk: %d\n", sb.logstart);
    printf(STDOUT, " inode start blk: %d\n", sb.inodestart);
    printf(STDOUT, "bitmap start blk: %d\n", sb.bmapstart);
    printf(STDOUT, "\n");
  }
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
  } else if (strcmp(cmd, "list") == 0) {
    exec_list(arg);
  } else if (strcmp(cmd, "stat") == 0) {
    exec_stat(arg);
  } else if (strcmp(cmd, "info") == 0) {
    exec_info(arg);
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
