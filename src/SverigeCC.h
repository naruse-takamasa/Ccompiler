#include <stdbool.h>

extern char *user_input;

void error_at(char *loc, char *fmt, ...);

extern char reserved[12][20];
extern int reserved_len[];
extern const int reserved_size;

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

Token *tokenize(char *p);
bool consume(char *op);
void expect(char *op);
int expect_num();

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

Node *expr();

void gen(Node *node);
