/**
 * @file util.c
 * @author Takamasa Naruse
 * @brief 汎用的な関数(エラー文の出力など)の実装
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */

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

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
	return;
}