#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////main.c//////////////////////
extern char *user_input;

////////////////////////util.c//////////////////////
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

////////////////////////tokenize.c//////////////////////
extern char reserved[][10];
extern int reserved_len[];
extern const int reserved_size;

/**
 * @brief トークンの種類の定義
 * 
 */
typedef enum {
	TK_CONTROL_FLOW,
	TK_IDENT,
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

extern Token *token;

typedef struct LVar LVar;

struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
};

extern LVar *locals;
LVar *find_lvar(Token *tok);

Token *tokenize(char *p);
bool consume(char *op);
bool consume_ident();
int consume_control_flow();
void expect(char *op);
int expect_num();
void expect_ident();
bool at_eof();

////////////////////////parse.c//////////////////////
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
	ND_ASSIGN,
	ND_LVAR,
	ND_RETURN,
	ND_IF,
	ND_WHILE,
	ND_FOR,
	ND_BLOCK,
	ND_FUNCALL,
} NodeKind;

typedef struct Node Node;
extern Node *code[100];

/**
 * @brief 抽象構文木の頂点の定義
 * @param lhs 左の子ノード
 * @param rhs 右の子ノード
 * @param condition ND_IFのときの条件またはND_FORのときの条件
 * @param them_stmt ND_IFまたはND_FORのときの条件を満たした場合のステートメント
 * @param else_stmt ND_IFのときの条件を満たさなかった場合のステートメント
 * @param init ND_FORのときの初期条件
 * @param loop ND_FORのときの繰り返し動作
 * 
 */
struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
	int offset;

	// if
	Node *condition;
	Node *then_stmt;
	Node *else_stmt;

	// for 以下の変数とconditionとthen_stmtを使う
	Node *init;
	Node *loop;

	// block
	Node *next;

	// function
	char *funcname;
};

void program();

////////////////////////codegen.c//////////////////////
void gen(Node *node);
