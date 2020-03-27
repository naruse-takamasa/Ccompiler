/**
 * @file parse.c
 * @author Takamasa Naruse
 * @brief construct abstract syntax tree
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
		node->type = lvar->type;
	} else {
		error("%sは知らん\n", tok->str);
	}
	return node;
}

int add_lvar(Token *tok, Type *type) {
	LVar *lvar = find_lvar(tok);
	if (lvar) error_at(tok->str, "変数名がかぶってます(add_lvar)\n");
	lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->offset = lvar_list->offset + 8;
	lvar->next = lvar_list;
	lvar->type = type;
	lvar_list = lvar;
	return lvar->offset;
}

Node *new_node_lvar_dec(Token *tok, Type *type) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	node->offset = add_lvar(tok, type);
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

Type *new_type(TypeKind typekind, Type *ptr_to) {
	Type *type = calloc(1, sizeof(Type));
	type->ty = typekind;
	type->ptr_to = ptr_to;
	return type;
}

Node *expr();
Node *unary();

Node *primary() {
	if (consume_nxt("(")) {
		Node *node = expr();
		expect_nxt(")");
		return node;
	}
	if (is_ident()) {
		Token *now = token;
		expect_ident_nxt();
		if (consume_nxt("(")) {
			Node *node = calloc(1, sizeof(Node));
			node->kind = ND_FUNCALL;
			node->funcname = strndup(now->str, now->len);
			Node *now = node;
			while (!consume_nxt(")")) {
				Node *arg = expr();
				now->next = arg;
				now = arg;
				consume_nxt(",");
			}
			now->next = NULL;
			return node;
		}
		Node *node = new_node_lvar(now);
		return node;
	}
	// マジで????
	if (token->kind == TK_NUM) return new_node_set_num(expect_num_nxt());
	return unary();
}

Node *unary() {
	if (consume_nxt("+")) {
		Node *node = primary();
		return node;
	} else if (consume_nxt("-")) {
		Node *node = new_node_LR(ND_SUB, new_node_set_num(0), primary());
		return node;
	} else if (consume_nxt("*")) {
		Node *node = new_node_LR(ND_DEREF, unary(), NULL);
		return node;
	} else if (consume_nxt("&")) {
		Node *node = new_node_LR(ND_ADDR, unary(), NULL);
		return node;
	} else {
		return primary();
	}
}

Node *mul() {
	Node *node = unary();
	for (;;) {
		if (consume_nxt("*")) node = new_node_LR(ND_MUL, node, unary());
		else if (consume_nxt("/")) node = new_node_LR(ND_DIV, node, unary());
		else {
			return node;
		}
	}
}

Node *new_add(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (node1->type->ty == INT && node2->type->ty == INT) {
		return new_node_LR(ND_ADD, node1, node2);
	}
	if (node1->type->ty == INT && node2->type->ty == PTR) {
		return new_node_LR(ND_PTR_ADD, node2, node1);
	}
	if (node1->type->ty == PTR && node2->type->ty == INT) {
		return new_node_LR(ND_PTR_ADD, node1, node2);
	}
	error("何その式(new_add)\n");
	return new_node_LR(ND_PTR_ADD, node1, node2);
}

Node *new_sub(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (node1->type->ty == INT && node2->type->ty == INT) {
		return new_node_LR(ND_SUB, node1, node2);
	}
	if (node1->type->ty == PTR && node2->type->ty == PTR) {
		return new_node_LR(ND_PTR_DIFF, node1, node2);
	}
	if (node1->type->ty == PTR && node2->type->ty == INT) {
		return new_node_LR(ND_PTR_SUB, node1, node2);
	}
	error("何その式(new_add)\n");
	return new_node_LR(ND_PTR_SUB, node1, node2);
}

Node *add() {
	Node *node = mul();
	for (;;) {
		if (consume_nxt("+")) // node = new_node_LR(ND_ADD, node, mul());
		{node = new_add(node, mul());}
		else if (consume_nxt("-")) // node = new_node_LR(ND_SUB, node, mul());
		{node = new_sub(node, mul());}
		else return node;
	}
}

Node *relational() {
	Node *node = add();
	for (;;) {
		if (consume_nxt(">=")) node = new_node_LR(ND_GE, node, add());
		else if (consume_nxt("<=")) node = new_node_LR(ND_LE, node, add());
		else if (consume_nxt(">")) node = new_node_LR(ND_GT, node, add());
		else if (consume_nxt("<")) node = new_node_LR(ND_LT, node, add());
		else return node;
	}
}

Node *equality() {
	Node *node = relational();
	for(;;) {
		if (consume_nxt("==")) node = new_node_LR(ND_EQ, node, relational());
		else if (consume_nxt("!=")) node = new_node_LR(ND_NEQ, node, relational());
		return node;
	}
}

Node *assign() {
	Node *node = equality();
	if (consume_nxt("=")) node = new_node_LR(ND_ASSIGN, node, assign());
	return node;
}

Node *expr() {
	Node *node = assign();
	return node;
}

Node *declaration() {
	if (consume_d_type() == 0) return NULL;
	int type_id = get_d_type_id();
	Node *node = calloc(1, sizeof(Node));
	switch (type_id)
	{
	case 0:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = INT;
		break;
	default:
		error("その型は知らん\n");
		break;
	}
	next();
	while (consume_nxt("*")) {
		Type *now_type = new_type(PTR, node->type);
		node->type = now_type;
	}
	node->offset = add_lvar(token, node->type);
	expect_ident_nxt();
	if (consume_nxt(";")) {
		node->kind = ND_NULL;
		return node;
	}
	node->kind = ND_LVAR;

	consume_nxt("=");

	Node *r = equality();
	consume_nxt(";");
	return new_node_LR(ND_ASSIGN, node, r);
}

Node *stmt() {
	Node *node;
	// control flow
	if (consume_cntrl()) {
		int control_id = get_cntrl_id();
		next();
		switch (control_id)
		{
		case 0: // return
			node = new_node_LR(ND_RETURN, expr(), NULL);
			expect_nxt(";");
			break;
		case 1: // if
			expect_nxt("(");
			node = new_node_if(expr(), NULL, NULL);
			expect_nxt(")");
			node->then_stmt = stmt();
			int next_control_id = -1;
			if (consume_cntrl()) next_control_id = get_cntrl_id();
			if (next_control_id == 2) {
				node->else_stmt = stmt();
			}
			break;
		case 3: // while
			expect_nxt("(");
			node = new_node_LR(ND_WHILE, expr(), NULL);
			expect_nxt(")");
			node->rhs = stmt();
			break;
		case 4: // for
			node = new_node_for(NULL, NULL, NULL);
			expect_nxt("(");
			if (!consume_nxt(";")) {
				node->init = expr();
				expect_nxt(";");
			}
			if (!consume_nxt(";")) {
				node->condition = expr();
				expect_nxt(";");
			}
			if (!consume_nxt(")")) {
				node->loop = expr();
				expect_nxt(")");
			}
			node->then_stmt = stmt();
			break;
		default:
			break;
		}
		return node;
	}
	// declaration
	Node *dec = declaration();
	if (dec != NULL) return dec;
	// only ";"
	if (consume_nxt(";")) {
		node = NULL;
		return node;
	}
	// block
	if (consume_nxt("{")) {
		node = new_node_LR(ND_BLOCK, NULL, NULL);
		Node *now = node;
		while (!consume_nxt("}")) {
			Node *statement = stmt();
			now->next = statement;
			now = statement;
		}
		now->next = NULL;
		return node;
	}
	// expression
	node = expr();
	expect_nxt(";");
	return node;
}

Node *pre_stmt() {
	Node *node = stmt();
	type_analyzer(node);
	return node;
}

Function *func_def() {
	Function *func = calloc(1, sizeof(Function));

	if (consume_d_type(token) == 0) error_at(token->str, "型を宣言してください\n");
	next();

	if (!is_ident()) error_at(token->str, "関数名を宣言してください\n");
	func->name = strndup(token->str, token->len);
	next();

	// 引数を解析
	expect_nxt("(");
	Node **now_arg = &(func->next_arg);
	int arg_cnt = 0;
	while (!consume_nxt(")")) {
		int type_id = get_d_type_id();
		Type *now = calloc(1, sizeof(Type));
		switch (type_id)
		{
		case 0:
			now->ty = INT;
			break;
		default:
			break;
		}
		next();
		while (consume_nxt("*")) {
			Type *now_type = new_type(PTR, now);
			now = now_type;
		}
		is_ident();
		Node *arg = new_node_lvar_dec(token, now);
		arg->kind = ND_ARG;
		*now_arg = arg;
		now_arg = &(arg->next_arg);
		arg_cnt++;
		next();
		consume_nxt(",");
	}
	func->arg_count = arg_cnt;
	// 関数本体の解析
	expect_nxt("{");
	Node **now = &(func->next_stmt);
	while (!consume_nxt("}")) {
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