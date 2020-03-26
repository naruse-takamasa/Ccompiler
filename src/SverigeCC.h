#define _GNU_SOURCE

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////
// main.c
////////////////////////////////////////////////////////////////////////////

extern char *user_input;

////////////////////////////////////////////////////////////////////////////
// util.c
////////////////////////////////////////////////////////////////////////////

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

////////////////////////////////////////////////////////////////////////////
// tokenize.c
////////////////////////////////////////////////////////////////////////////

/**
 * @brief トークンの種類の定義
 * 
 */
typedef enum {
	// 制御構文
	// TK_CONTROL_FLOW,
	// 文字列(変数名とか関数名とか)
	TK_IDENT,
	// 基本データ型
	// TK_DATA_TYPE,
	// 数値
	TK_NUM,
	// 記号
	TK_RESERVED,
	// 終端
	TK_EOF,
} TokenKind;

typedef struct Token Token;
/**
 * @brief トークン情報を管理する
 * @param kind トークンの種類
 * @param next 次に続くトークンのアドレス
 * @param val 数値だったときの値
 * @param str このトークンの文字列
 * @param len このトークンの文字列の長さ
 * 
 */
struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *str;
	int len;
};

extern Token *token;

Token *tokenize(char *p);
void next();
bool consume(char *op);
bool consume_next(char *op);
bool consume_ident();
bool is_ident();
int consume_control_flow_next();
int consume_control_flow();
int get_control_id();
int get_type_id();
int consume_data_type_next();
int consume_data_type();
int is_control_flow();
int is_data_type();
void expect_next(char *op);
int expect_num_next();
void expect_ident_next();
bool at_eof();

////////////////////////////////////////////////////////////////////////////
// parse.c
////////////////////////////////////////////////////////////////////////////

typedef enum {
	// 算術演算
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	// 比較演算
	ND_LE,
	ND_GE,
	ND_LT,
	ND_GT,
	ND_EQ,
	ND_NEQ,
	// 数値
	ND_NUM,
	// 代入
	ND_ASSIGN,
	// 代入時の左辺の変数
	ND_LVAR,
	// 制御構文
	ND_RETURN,
	ND_IF,
	ND_WHILE,
	ND_FOR,
	// {}
	ND_BLOCK,
	// 関数呼び出し
	ND_FUNCALL,
	// 関数定義
	ND_FUNCDEF,
	// 仮引数
	ND_ARG,
	// アドレス
	ND_ADDR,
	// ポインタ
	ND_DEREF,
	// int
	ND_INT,
	// 変数宣言のみ
	ND_NULL,
	// ポインタの宣言
	ND_DEREF_DEC,
} NodeKind;

typedef struct Type Type;

struct Type {
	enum{INT, PTR} ty;
	struct Type *ptr_to;
};


typedef struct LVar LVar;

struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
	Type *type;
};

// extern LVar *locals[100];
extern LVar *lvar_list;

typedef struct Node Node;

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
	// ノードの種類
	NodeKind kind;
	// 左の子のアドレス
	Node *lhs;
	// 右の子のアドレス
	Node *rhs;
	// 数値
	int val;
	// 変数だったときの、rbpからのオフセット
	int offset;
	Type *type;

	// if
	Node *condition;
	Node *then_stmt;
	Node *else_stmt;

	// for 以下の変数とconditionとthen_stmtを使う
	Node *init;
	Node *loop;

	// block
	Node *next;

	// function call
	char *funcname;
	Node *next_stmt;
	Node *next_arg;
};

typedef struct Function Function;

/**
 * @brief 1つのfunction definitionをまとめてる
 * 
 */
struct Function {
	char *name;
	int arg_count;
	Node *next_arg;
	Node *next_stmt;
	Function *next;
	int total_offset;
	LVar *local;
};

extern Function *now_func;
extern Function *code;

void program();

////////////////////////////////////////////////////////////////////////////
// codegen.c
////////////////////////////////////////////////////////////////////////////

void gen(Node *node);
void func_gen(Function *func);