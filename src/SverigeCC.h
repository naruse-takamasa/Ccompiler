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
 * @brief define token kind
 * 
 */
typedef enum {
	// 文字列(変数名とか関数名とか)
	TK_IDENT,
	// 数値
	TK_NUM,
	// 記号
	TK_RESERVED,
	// sizeof
	TK_SIZEOF,
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
bool consume_nxt(char *op);
bool consume_ident();
Token *consume_ident_nxt();
bool is_ident();
int consume_cntrl_nxt();
int consume_cntrl();
int get_cntrl_id();
int get_d_type_id();
int consume_d_type_nxt();
int consume_d_type();
int is_cntrl();
int is_d_type();
void expect_nxt(char *op);
int expect_num_nxt();
void expect_ident_nxt();
void expect_ident(void);
bool at_eof();

////////////////////////////////////////////////////////////////////////////
// parse.c
////////////////////////////////////////////////////////////////////////////

typedef enum {
	// 算術演算
	ND_ADD,
	ND_PTR_ADD,
	ND_SUB,
	ND_PTR_SUB,
	ND_PTR_DIFF,
	ND_MUL,
	ND_DIV, // 6
	// 比較演算
	ND_LE,
	ND_GE,
	ND_LT,
	ND_GT,
	ND_EQ,
	ND_NEQ, // 12
	// 数値
	ND_NUM,
	// 代入
	ND_ASSIGN, // 14
	// 代入時の左辺の変数
	ND_LVAR,
	// 制御構文
	ND_RETURN, // 16
	ND_IF,
	ND_WHILE,
	ND_FOR,
	// {}
	ND_BLOCK, // 20
	// 関数呼び出し
	ND_FUNCALL,
	// 関数定義
	ND_FUNCDEF,
	// 仮引数
	ND_ARG, // 23
	// アドレス
	ND_ADDR,
	// ポインタ
	ND_DEREF, // 25
	// int 
	ND_INT,
	// 変数宣言のみ
	ND_NULL, // 27
} NodeKind;

typedef enum {
	TP_INT,
	TP_PTR,
	TP_ARRAY,
} TypeKind;

typedef struct Type Type;

struct Type {
	TypeKind ty;
	struct Type *ptr_to;
	int _sizeof;
	size_t array_size;
};

typedef struct LVar LVar;

struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
	Type *type;
};

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
	// 変数の型
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
 * @brief function
 * 
 */
struct Function {
	char *name;
	int arg_count;
	Node *arg;
	Node *stmt;
	Function *next;
	int total_offset;
	LVar *local;
	Type *type;
};

void program(void);

////////////////////////////////////////////////////////////////////////////
// codegen.c
////////////////////////////////////////////////////////////////////////////

void func_gen(Function *func);

////////////////////////////////////////////////////////////////////////////
// type_analyze.c
////////////////////////////////////////////////////////////////////////////

Type *new_type(TypeKind typekind, Type *ptr_to, int sz);
bool is_int(Type *type);
bool is_ptr(Type *type);
bool is_array(Type *type);
void type_analyzer(Node *node);