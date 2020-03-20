#include "SverigeCC.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * @brief 文のどこでコンパイルエラーしたかをエラー出力
 * 
 * @param loc 
 * @param fmt 
 * @param ... 
 */
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
	return;
}