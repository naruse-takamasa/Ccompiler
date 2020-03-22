/**
 * @file tokenize.c
 * @author Takamasa Naruse
 * @brief トークナイズ用
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char reserved[][10] = {"+", "-", "*", "/", "(", ")", "==", "!=", ">=", "<=", ">", "<", "=", ";", "{", "}"};
int reserved_len[] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1};
const int reserved_size = 16;

char control_flow[][10] = {"return", "if", "else", "while", "for"};
int control_flow_len[] = {6, 2, 4, 5, 3};
const int control_flow_size = 5;

Token *token;

bool consume(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) 
		return false;
	token = token->next;
	return true;
}

bool consume_ident() {
	if (token->kind != TK_IDENT) return false;
	token = token->next;
	return token;
}

void expect(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) 
		error_at(token->str, "not '%s' -> '%s'(expect)", op, token->str);
	token = token->next;
}

int expect_num() {
	if (token->kind != TK_NUM) error_at(token->str, "not number\n");
	int res = token->val;
	token = token->next;
	return res;
}

void expect_ident() {
	if (token->kind == TK_IDENT) token = token->next;
	else error_at(token->str, "not ident\n");
}

bool is_alnum(char c) {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_');
}

int consume_control_flow() {
	if (token->kind != TK_CONTROL_FLOW) return -1;
	int res = token->val;
	token = token->next;
	return res;
	// error("consume_control_flow (該当なし) \n");
}

int is_reserved(char *p) {
	for (int i = 0; i < reserved_size; i++) {
		if (memcmp(p, reserved[i], reserved_len[i]) == 0) return i;
	}
	return -1;
}

int is_control_flow(char *p) {
	for (int i = 0; i < control_flow_size; i++) {
		if (memcmp(p, control_flow[i], control_flow_len[i]) == 0 &&
			!is_alnum(p[control_flow_len[i]])
		) {
			return i;
		}
	}
	return -1;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	// if (kind == TK_RESERVED) {
	// 	bool accept = false;
	// 	for (int i = 0; i < reserved_size; i++) {
	// 		if (memcmp(str, reserved[i], reserved_len[i]) == 0) {
	// 			tok->len = reserved_len[i];
	// 			accept = true;
	// 			break;
	// 		}
	// 	}
	// 	if (!accept) error_at(str, "new_token error\n");
	// }
	cur->next = tok;
	return tok;
}

Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		// fprintf(stderr, "now tokenize : %s \n", p);
		// 空白
		if (isspace(*p)) {
			p++;
			continue;
		}
		// 記号
		int reserved_id = is_reserved(p);
		if (reserved_id != -1) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = reserved_len[reserved_id];
			p += reserved_len[reserved_id];
			continue;
		}
		// 数値
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}
		// 制御構文
		int control_id = is_control_flow(p);
		if (control_id != -1) {
			cur = new_token(TK_CONTROL_FLOW, cur, p);
			cur->len = control_flow_len[control_id];
			p += control_flow_len[control_id];
			cur->val = control_id;
			continue;
		}
		// 変数
		if (is_alnum(*p)) {
			fprintf(stderr, "hi\n");
			cur = new_token(TK_IDENT, cur, p);
			int idx = 0;
			while (is_alnum(p[idx])) {
				idx++;
			}
			cur->len = idx;
			p += idx;
			continue;
		}
		error_at(token->str, "can't tokenize\n");
	}
	new_token(TK_EOF, cur, p);
	return head.next;
}

LVar *find_lvar(Token *tok) {
	for (LVar *now = locals; now; now = now->next) {
		if (tok->len == now->len && memcmp(tok->str, now->name, tok->len) == 0) {
			return now;
		}
	}
	return NULL;
}