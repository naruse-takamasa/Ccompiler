/**
 * @file main.c
 * @author Takamasa Naruse
 * @brief main function
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"

char *user_input;

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "argc is not equal 2\n");
		return 1;
	} else fprintf(stderr, "input code : %s\n", argv[1]);

	user_input = argv[1];
	token = tokenize(user_input);
	// for (Token *now = token; now->kind != TK_EOF; now = now->next) {
	// 	fprintf(stderr, "%s, %d, %d\n", now->str, now->len, now->val);
	// }
	fprintf(stderr, "tokenize OK\n");

	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	program();
	return 0;
}