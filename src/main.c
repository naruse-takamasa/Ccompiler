/**
 * @file main.c
 * @author Takamasa Naruse
 * @brief main関数実行用
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"
#include <stdio.h>
#include <stdlib.h>

char *user_input;
LVar *locals;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "argc is not equal 2\n");
    return 1;
  } else {
	  fprintf(stderr, "compiler name : %s\n", argv[0]);
	  fprintf(stderr, "input code : %s\n", argv[1]);
  }

  user_input = argv[1];
  token = tokenize(user_input);
  for (Token *now = token; now->kind != TK_EOF; now = now->next) {
	  fprintf(stderr, "%s, %d, %d\n", now->str, now->len, now->val);
  }

  locals = calloc(1, sizeof(LVar));
  locals->len = 0;
  locals->name = "";
  locals->next = NULL;
  locals->offset = 0;

  program();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", locals->offset);
  for (int i = 0; code[i]; i++) {
	  gen(code[i]);
	  printf("  pop rax\n");
  }
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}