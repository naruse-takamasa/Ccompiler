#include <stdbool.h>

////////////////////////main.c//////////////////////
extern char *user_input;

////////////////////////util.c//////////////////////
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

////////////////////////tokenize.c//////////////////////
extern char reserved[][20];
extern int reserved_len[];
extern const int reserved_size;

/**
 * @brief トークンの種類の定義
 * 
 */
typedef enum {
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

Token *token;

Token *tokenize(char *p);
bool consume(char *op);
Token *consume_ident();
void expect(char *op);
int expect_num();
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
} NodeKind;

typedef struct Node Node;
Node *code[100];

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
	int offset;
};

void program();

////////////////////////codegen.c//////////////////////
void gen(Node *node);
