/**
 * @file parse.c
 * @author Takamasa Naruse
 * @brief 抽象構文木の構築
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"

Node *code[100];
int func_id = 0;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

/**
 * @brief tok->strという変数のオフセットを計算し、ノードを作成
 * 
 * @param tok 
 * @return Node* 
 */
Node *new_node_lvar(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	LVar *lvar = find_lvar(tok);
	if (lvar) {
		node->offset = lvar->offset;
	} else {
		lvar = calloc(1, sizeof(LVar));
		lvar->name = tok->str;
		lvar->len = tok->len;
		lvar->offset = locals[func_id]->offset + 8;
		lvar->next = locals[func_id];
		locals[func_id] = lvar;
		node->offset = lvar->offset;
	}
	return node;
}

Node *new_node_if(Node *condition, Node *then_stmt, Node *else_stmt) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_IF;
	node->condition = condition;
	node->then_stmt = then_stmt;
	node->else_stmt = else_stmt;
	return node;
}

Node *new_node_for(Node *init, Node *condition, Node *loop) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_FOR;
	node->init = init;
	node->condition = condition;
	node->loop = loop;
	return node;
}

Node *expr();
Node *unary();

Node *primary() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}
	if (token->kind == TK_IDENT) {
		Token *now = token;
		expect_ident();
		if (consume("(")) {
			Node *node = calloc(1, sizeof(Node));
			node->kind = ND_FUNCALL;
			node->funcname = strndup(now->str, now->len);
			Node *now = node;
			while (!consume(")")) {
				Node *arg = expr();
				now->next = arg;
				now = arg;
				consume(",");
			}
			now->next = NULL;
			return node;
		}
		Node *node = new_node_lvar(now);
		return node;
	}
	// マジで????
	if (token->kind == TK_NUM) return new_node_num(expect_num());
	return unary();
}

Node *unary() {
	if (consume("+")) {
		Node *node = primary();
		return node;
	} else if (consume("-")) {
		Node *node = new_node(ND_SUB, new_node_num(0), primary());
		return node;
	} else {
		return primary();
	}
}

Node *mul() {
	Node *node = unary();
	for (;;) {
		if (consume("*")) node = new_node(ND_MUL, node, unary());
		else if (consume("/")) node = new_node(ND_DIV, node, unary());
		else {
			return node;
		}
	}
}

Node *add() {
	Node *node = mul();
	for (;;) {
		if (consume("+")) {
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

Node *assign() {
	Node *node = equality();
	if (consume("=")) {
		node = new_node(ND_ASSIGN, node, assign());
	}
	return node;
}

Node *expr() {
	Node *node = assign();
	return node;
}

Node *stmt() {
	Node *node;
	int control_id = consume_control_flow();
	if (control_id != -1) {
		switch (control_id)
		{
		case 0: // return
			// fprintf(stderr, "%s\n", token->str);
			node = new_node(ND_RETURN, expr(), NULL);
			expect(";");
			break;
		case 1: // if
			expect("(");
			node = new_node_if(expr(), NULL, NULL);
			expect(")");
			node->then_stmt = stmt();
			int next_control_id = is_control_flow();
			if (next_control_id == 2) {
				node->else_stmt = stmt();
			}
			break;
		case 3: // while
			expect("(");
			node = new_node(ND_WHILE, expr(), NULL);
			expect(")");
			node->rhs = stmt();
			break;
		case 4: // for
			node = new_node_for(NULL, NULL, NULL);
			expect("(");
			if (!consume(";")) {
				node->init = expr();
				expect(";");
			}
			if (!consume(";")) {
				node->condition = expr();
				expect(";");
			}
			if (!consume(")")) {
				node->loop = expr();
				expect(")");
			}
			node->then_stmt = stmt();
			break;
		default:
			break;
		}
		return node;
	}
	if (consume(";")) {
		// 何も式が書かれなかった場合
		node = NULL;
		return node;
	}
	if (consume("{")) {
		// ブロック
		node = new_node(ND_BLOCK, NULL, NULL);
		Node *now = node;
		while (!consume("}")) {
			Node *statement = stmt();
			now->next = statement;
			now = statement;
		}
		now->next = NULL;
		return node;
	}
	// ただの式
	node = expr();
	expect(";");
	return node;
}

Node *func_def() {
	if (!is_ident()) error_at(token->str, "関数名を宣言してください\n");
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_FUNCDEF;
	node->offset = func_id;
	node->funcname = strndup(token->str, token->len);

	// この関数内でのローカル変数の集合の初期化
	locals[func_id] = calloc(1, sizeof(LVar));
	locals[func_id]->len = 0;
	locals[func_id]->name = "";
	locals[func_id]->next = NULL;
	locals[func_id]->offset = 0;

	expect_ident();

	// 引数を解析
	expect("(");
	Node *now_arg = node;
	while (!consume(")")) {
		Node *arg = new_node_lvar(token);
		now_arg->next_arg = arg;
		now_arg = arg;
		consume_ident();
		consume(",");
	}
	now_arg->next_arg = NULL;

	// 関数本体の解析
	expect("{");
	Node *now = node;
	while (!consume("}")) {
		Node *statement = stmt();
		now->next_stmt = statement;
		now = statement;
	}
	now->next_stmt = NULL;

	func_id++;
	return node;
}

void program() {
	int idx = 0;
	while (!at_eof()) {
		code[idx] = func_def();
		idx++;
	}
	code[idx] = NULL;
}