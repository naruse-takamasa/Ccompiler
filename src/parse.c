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

LVar *lvar_list;

LVar *find_lvar(Token *tok) {
	for (LVar *now = lvar_list; now; now = now->next) {
		if (tok->len == now->len && memcmp(tok->str, now->name, tok->len) == 0) {
			return now;
		}
	}
	return NULL;
}

Node *new_node_LR(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_set_num(int val) {
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
		error("%sは知らん\n", tok->str);
	}
	return node;
}

Node *new_node_lvar_declaration(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	LVar *lvar = find_lvar(tok);
	if (lvar) error_at(tok->str, "変数名がかぶってます\n");
	lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->offset = lvar_list->offset + 8;
	lvar->next = lvar_list;
	lvar_list = lvar;
	node->offset = lvar->offset;
	return node;
}

int add_lvar(Token *tok) {
	fprintf(stderr, "add lvar : %s\n", tok->str);
	LVar *lvar = find_lvar(tok);
	if (lvar) error_at(tok->str, "変数名がかぶってます(add_lvar)\n");
	lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->offset = lvar_list->offset + 8;
	fprintf(stderr, "offset : %d\n", lvar->offset);
	lvar->next = lvar_list;
	lvar_list = lvar;
	return lvar->offset;
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
	if (consume_next("(")) {
		Node *node = expr();
		expect_next(")");
		return node;
	}
	if (is_ident()) {
		Token *now = token;
		expect_ident_next();
		if (consume_next("(")) {
			Node *node = calloc(1, sizeof(Node));
			node->kind = ND_FUNCALL;
			node->funcname = strndup(now->str, now->len);
			Node *now = node;
			while (!consume_next(")")) {
				Node *arg = expr();
				now->next = arg;
				now = arg;
				consume_next(",");
			}
			now->next = NULL;
			return node;
		}
		Node *node = new_node_lvar(now);
		return node;
	}
	// マジで????
	if (token->kind == TK_NUM) return new_node_set_num(expect_num_next());
	return unary();
}

Node *unary() {
	if (consume_next("+")) {
		Node *node = primary();
		return node;
	} else if (consume_next("-")) {
		Node *node = new_node_LR(ND_SUB, new_node_set_num(0), primary());
		return node;
	} else if (consume_next("*")) {
		Node *node = new_node_LR(ND_DEREF, unary(), NULL);
		return node;
	} else if (consume_next("&")) {
		Node *node = new_node_LR(ND_ADDR, unary(), NULL);
		return node;
	} else {
		return primary();
	}
}

Node *mul() {
	Node *node = unary();
	for (;;) {
		if (consume_next("*")) node = new_node_LR(ND_MUL, node, unary());
		else if (consume_next("/")) node = new_node_LR(ND_DIV, node, unary());
		else {
			return node;
		}
	}
}

Node *add() {
	Node *node = mul();
	for (;;) {
		if (consume_next("+")) {
			node = new_node_LR(ND_ADD, node, mul());
		}
		else if (consume_next("-")) node = new_node_LR(ND_SUB, node, mul());
		else return node;
	}
}

Node *relational() {
	Node *node = add();
	for (;;) {
		if (consume_next(">=")) node = new_node_LR(ND_GE, node, add());
		else if (consume_next("<=")) node = new_node_LR(ND_LE, node, add());
		else if (consume_next(">")) node = new_node_LR(ND_GT, node, add());
		else if (consume_next("<")) node = new_node_LR(ND_LT, node, add());
		else return node;
	}
}

Node *equality() {
	Node *node = relational();
	for(;;) {
		if (consume_next("==")) node = new_node_LR(ND_EQ, node, relational());
		else if (consume_next("!=")) node = new_node_LR(ND_NEQ, node, relational());
		return node;
	}
}

Node *assign() {
	Node *node = equality();
	if (consume_next("=")) {
		node = new_node_LR(ND_ASSIGN, node, assign());
	}
	return node;
}

Node *expr() {
	Node *node = assign();
	return node;
}

// TODO:
Node *declaration() {
	if (consume_data_type_next() == 0) return NULL;
	int type_id = get_type_id();
	Node *node = calloc(1, sizeof(Node));
	switch (type_id)
	{
	case 0:
		node->type->ty = INT;
		break;
	default:
		break;
	}
	bool is_ptr = false;
	while (consume_next("*")) {
		is_ptr = true;
		Type *now_type = calloc(1, sizeof(Type));
		now_type->ty = PTR;
		now_type->ptr_to = node->type;
		node->type = now_type;
	}
	node->offset = add_lvar(token);
	expect_ident_next();
	if (consume_next(";")) {
		node->kind = ND_NULL;
		return node;
	}
	node->kind = ND_LVAR;
	consume_next("=");
	if (is_ptr) {
		Node *r = equality();
		consume_next(";");
		return new_node_LR(ND_DEREF_DEC, node, r);
	} else {
		Node *r = equality();
		consume_next(";");
		return new_node_LR(ND_ASSIGN, node, r);
	}
}

Node *stmt() {
	Node *node;
	if (consume_control_flow()) {
		int control_id = get_control_id();
		next();
		switch (control_id)
		{
		case 0: // return
			node = new_node_LR(ND_RETURN, expr(), NULL);
			expect_next(";");
			break;
		case 1: // if
			expect_next("(");
			node = new_node_if(expr(), NULL, NULL);
			expect_next(")");
			node->then_stmt = stmt();
			int next_control_id = -1;
			if (consume_control_flow()) next_control_id = get_control_id();
			if (next_control_id == 2) {
				node->else_stmt = stmt();
			}
			break;
		case 3: // while
			expect_next("(");
			node = new_node_LR(ND_WHILE, expr(), NULL);
			expect_next(")");
			node->rhs = stmt();
			break;
		case 4: // for
			node = new_node_for(NULL, NULL, NULL);
			expect_next("(");
			if (!consume_next(";")) {
				node->init = expr();
				expect_next(";");
			}
			if (!consume_next(";")) {
				node->condition = expr();
				expect_next(";");
			}
			if (!consume_next(")")) {
				node->loop = expr();
				expect_next(")");
			}
			node->then_stmt = stmt();
			break;
		default:
			break;
		}
		return node;
	}
	// 変数宣言
	Node *dec = declaration();
	if (dec != NULL) return dec;
	if (consume_next(";")) {
		// 何も式が書かれなかった場合
		node = NULL;
		return node;
	}
	if (consume_next("{")) {
		// ブロック
		node = new_node_LR(ND_BLOCK, NULL, NULL);
		Node *now = node;
		while (!consume_next("}")) {
			Node *statement = stmt();
			now->next = statement;
			now = statement;
		}
		now->next = NULL;
		return node;
	}
	// ただの式
	node = expr();
	expect_next(";");
	return node;
}

Function *func_def() {
	Function *func = calloc(1, sizeof(Function));

	if (consume_data_type(token) == 0) error_at(token->str, "型を宣言してください\n");
	next();

	if (!is_ident()) error_at(token->str, "関数名を宣言してください\n");
	func->name = strndup(token->str, token->len);
	next();

	// 引数を解析
	expect_next("(");
	Node **now_arg = &(func->next_arg);
	int arg_cnt = 0;
	while (!consume_next(")")) {
		consume_data_type_next();
		consume_ident();
		Node *arg = new_node_lvar_declaration(token);
		arg->kind = ND_ARG;
		*now_arg = arg;
		now_arg = &(arg->next_arg);
		arg_cnt++;
		next();
		consume_next(",");
	}
	func->arg_count = arg_cnt;

	// 関数本体の解析
	expect_next("{");
	Node **now = &(func->next_stmt);
	while (!consume_next("}")) {
		Node *statement = stmt();
		*now = statement;
		now = &(statement->next_stmt);
	}
	func->total_offset = lvar_list->offset;
	return func;
}

void program() {
	while (!at_eof()) {
		lvar_list = calloc(1, sizeof(LVar));
		func_gen(func_def());
	}
}