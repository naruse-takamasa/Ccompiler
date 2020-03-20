#include "SverigeCC.h"
#include <stdlib.h>

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
	return node;
}