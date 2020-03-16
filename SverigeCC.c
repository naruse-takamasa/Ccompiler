#include <stdio.h>
#include <stdlib.h>

void error() {
	fprintf(stderr, "error!!\n");
	return;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  } else {
	  fprintf(stderr, "%s\n", argv[0]);
	  fprintf(stderr, "%s\n", argv[1]);
  }

  char *p = argv[1];
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  printf("  mov rax, %ld\n", strtol(p, &p, 10));
  while (*p) {
	  if (*p == '+') {
		  printf("  add rax, %ld\n", strtol(p, &p, 10));
		  continue;
	  } else if (*p == '-') {
		  printf("  sub rax, %ld\n", strtol(p, &p, 10));
		  continue;
	  } else {
		  error();
		  return 1;
	  }
  }
  printf("  ret\n");
  return 0;
}
