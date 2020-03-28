/**
 * @file tokenize.c
 * @author Takamasa Naruse
 * @brief tokenize user input
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"

char multi_punct[][10] = {"==", "!=", ">=", "<="};
int multi_punct_len[] = {2, 2, 2, 2};
int multi_punct_size = 4;

char control_flow[][10] = {"return", "if", "else", "while", "for"};
int control_flow_len[] = {6, 2, 4, 5, 3};
const int control_flow_size = 5;

char data_type[][10] = {"int"};
int data_type_len[] = {3};
const int data_type_size = 1;

Token *token;

void next() {
	token = token->next;
}

bool consume(char *op) {
	if (strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) return false;
	return true;
}

bool consume_nxt(char *op) {
	if (strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) return false;
	next();
	return true;
}

void expect_nxt(char *op) {
	if (strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) 
		error_at(token->str, "not '%s' -> '%s'(expect)", op, token->str);
	next();
}

int expect_num_nxt() {
	if (token->kind != TK_NUM) error_at(token->str, "not number\n");
	int res = token->val;
	next();
	return res;
}

void expect_ident_nxt() {
	if (token->kind == TK_IDENT) next();
	else error_at(token->str, "not ident\n");
}

bool is_ident() {
	return token->kind == TK_IDENT;
}

int consume_cntrl_nxt() {
	int res = is_cntrl(token->str);
	if (res == 0) return 0;
	next();
	return res;
}

int consume_cntrl() {
	return is_cntrl(token->str);
}

int consume_d_type_nxt() {
	int res = is_d_type(token->str);
	if (res == 0) return 0;
	next();
	return res;
}

int consume_d_type() {
	return is_d_type(token->str);
}

int get_cntrl_id() {
	for (int i = 0; i < control_flow_size; i++) {
		if (memcmp(token->str, control_flow[i], control_flow_len[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int get_d_type_id() {
	for (int i = 0; i < data_type_size; i++) {
		if (memcmp(token->str, data_type[i], data_type_len[i]) == 0) {
			return i;
		}
	}
	return -1;
}

bool is_alnum(char c) {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_');
}

int is_multi_punct(char *p) {
	for (int i = 0; i < multi_punct_size; i++) {
		if (memcmp(p, multi_punct[i], multi_punct_len[i]) == 0) return multi_punct_len[i];
	}
	return 0;
}

int is_single_punct(char *p) {
	if (memcmp(p, "_", 1) == 0) return 0;
	if (ispunct(*p) != 0) return 1;
	return 0;
}

int is_cntrl(char *p) {
	for (int i = 0; i < control_flow_size; i++) {
		if (memcmp(p, control_flow[i], control_flow_len[i]) == 0 &&
			!is_alnum(p[control_flow_len[i]])
		) {
			return control_flow_len[i];
		}
	}
	return 0;
}

int is_d_type(char *p) {
	for (int i = 0; i < data_type_size; i++) {
		if (memcmp(p, data_type[i], data_type_len[i]) == 0 &&
			!is_alnum(p[data_type_len[i]])
		) {
			return data_type_len[i];
		}
	}
	return 0;
}

int is_reserved(char *p) {
	int res = is_cntrl(p);
	if (res) return res;
	res = is_d_type(p);
	if (res) return res;
	res = is_multi_punct(p);
	if (res) return res;
	res = is_single_punct(p);
	if (res) return res;
	return 0;
}

bool is_sizeof(char *p) {
	char *szof = "sizeof";
	if (memcmp(p, szof, 6) == 0 && !is_alnum(p[6])) return true;
	return false;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	tok->len = len;
	return tok;
}

Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		// " "
		if (isspace(*p)) {
			p++;
			continue;
		}
		// reserved word
		int reserved_len = is_reserved(p);
		if (reserved_len) {
			cur = new_token(TK_RESERVED, cur, p, reserved_len);
			cur->len = reserved_len;
			p += reserved_len;
			continue;
		}
		// number
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, -1);
			cur->val = strtol(p, &p, 10);
			continue;
		}
		if (is_sizeof(p)) {
			cur = new_token(TK_SIZEOF, cur, p, 6);
			p += 6;
			continue;
		}
		// function name or variable
		if (is_alnum(*p)) {
			cur = new_token(TK_IDENT, cur, p, -1);
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
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}
