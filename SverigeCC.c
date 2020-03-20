/**
 * @file SverigeCC.c
 * @author Takamasa Naruse (ckv14093@ict.nitech.ac.jp)
 * @brief C compiler
 * @version 0.1
 * @date 2020-03-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *user_input;

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

char reserved[12][20] = {"+", "-", "*", "/", "(", ")", "==", "!=", ">=", "<=", ">", "<"};
int reserved_len[] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1};
const int reserved_size = 12;

/**
 * @brief トークンの種類の定義
 * 
 */
typedef enum {
	TK_NUM,
	TK_RESERVED,
	TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *str;
	int len;
};

Token *token;

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

typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_LE,
	ND_GE,
	ND_LT,
	ND_GT,
	ND_EQ,
	ND_NEQ,
	ND_NUM,
} NodeKind;

typedef struct Node Node;

/**
 * @brief 抽象構文木の頂点の定義
 * @param lhs 左の子ノード
 * @param rhs 右の子ノード
 * 
 */
struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	//fprintf(stderr, "new node num : %d\n", val);
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *expr();

Node *primary() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	} else {
		// fprintf(stderr, "number\n");
		return new_node_num(expect_num());
	}
}

Node *unary() {
	if (consume("+")) {
		// fprintf(stderr, "consume+\n");
		Node *node = primary();
		return node;
	} else if (consume("-")) {
		// fprintf(stderr, "consume-\n");
		Node *node = new_node(ND_SUB, new_node_num(0), primary());
		return node;
	} else {
		// fprintf(stderr, "primary\n");
		return primary();
	}
}

Node *mul() {
	Node *node = unary();
	for (;;) {
		if (consume("*")) node = new_node(ND_MUL, node, unary());
		else if (consume("/")) node = new_node(ND_DIV, node, unary());
		else {
			// fprintf(stderr, "not * /\n");
			return node;
		}
	}
}

Node *add() {
	Node *node = mul();
	for (;;) {
		if (consume("+")) {
			// fprintf(stderr, "consume +\n");
			node = new_node(ND_ADD, node, mul());
		}
		else if (consume("-")) node = new_node(ND_SUB, node, mul());
		else return node;
	}
}

Node *relational() {
	Node *node = add();
	for (;;) {
		if (consume(">=")) node = new_node(ND_GE, node, add());
		else if (consume("<=")) node = new_node(ND_LE, node, add());
		else if (consume(">")) node = new_node(ND_GT, node, add());
		else if (consume("<")) node = new_node(ND_LT, node, add());
		else return node;
	}
}

Node *equality() {
	Node *node = relational();
	for(;;) {
		if (consume("==")) node = new_node(ND_EQ, node, relational());
		else if (consume("!=")) node = new_node(ND_NEQ, node, relational());
		return node;
	}
}

Node *expr() {
	Node *node = equality();
}
/**
 * @brief 抽象構文木からアセンブリを生成
 * 
 * @param node 
 */
void gen(Node *node) {
	if (node->kind == ND_NUM) {
		printf("  push %d\n", node->val);
		return;
	}
	if (node->kind == ND_GE) {
		gen(node->rhs);
		gen(node->lhs);
		node->kind = ND_LE;
	} else if (node->kind == ND_GT) {
		gen(node->rhs);
		gen(node->lhs);
		node->kind = ND_LT;
	} else {
		gen(node->lhs);
		gen(node->rhs);
	}
	printf("  pop rdi\n");
	printf("  pop rax\n");
	switch (node->kind)
	{
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("  imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("  cqo\n");
		printf("  idiv rdi\n");
		break;
	case ND_EQ:
		printf("  cmp rax, rdi\n");
		printf("  sete al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_NEQ:
		printf("  cmp rax, rdi\n");
		printf("  setne al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LE:
		printf("  cmp rax, rdi\n");
		printf("  setle al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LT:
		printf("  cmp rax, rdi\n");
		printf("  setl al\n");
		printf("  movzb rax, al\n");
		break;
	default:
		break;
	}
	printf("  push rax\n");
}

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
