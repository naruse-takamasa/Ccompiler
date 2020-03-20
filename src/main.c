#include "SverigeCC.h"
#include <stdio.h>

char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  } else {
	  fprintf(stderr, "%s\n", argv[0]);
	  fprintf(stderr, "input code : %s\n", argv[1]);
  }

  user_input = argv[1];
  token = tokenize(user_input);
  for (Token *now = token; now->kind != TK_EOF; now = now->next) {
	  fprintf(stderr, "%s, %d, %d\n", now->str, now->len, now->val);
  }
  Node *node = expr();
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  gen(node);
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}