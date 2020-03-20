#include "SverigeCC.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char reserved[12][20] = {"+", "-", "*", "/", "(", ")", "==", "!=", ">=", "<=", ">", "<"};
int reserved_len[] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1};
const int reserved_size = 12;

bool consume(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) 
		return false;
	token = token->next;
	return true;
}

void expect(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) 
		error_at(token->str, "not '%c'(expect)", op);
	token = token->next;
}

int expect_num() {
	if (token->kind != TK_NUM) error_at(token->str, "not number");
	int res = token->val;
	token = token->next;
	return res;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	if (kind == TK_RESERVED) {
		bool accept = false;
		for (int i = 0; i < reserved_size; i++) {
			if (memcmp(str, reserved[i], reserved_len[i]) == 0) {
				tok->len = reserved_len[i];
				accept = true;
				break;
			}
		}
		if (!accept) error_at(str, "new_token error\n");
	}
	cur->next = tok;
	return tok;
}

bool is_reserved(char *p) {
	for (int i = 0; i < reserved_size; i++) {
		if (memcmp(p, reserved[i], reserved_len[i]) == 0) return true;
	}
	// fprintf(stderr, "not reserved\n");
	return false;
}

Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		if (is_reserved(p)) {
			cur = new_token(TK_RESERVED, cur, p);
			p += cur->len;
			continue;
		}
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			//fprintf(stderr, "%s\n", p);
			continue;
		}
		fprintf(stderr, "can't tokenize\n");
	}
	new_token(TK_EOF, cur, p);
	//fprintf(stderr, "%s\n", head.next->str);
	return head.next;
}