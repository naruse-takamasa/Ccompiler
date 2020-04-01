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
	} else error("%sは知らん\n", tok->str);
	return node;
}

/**
 * @brief 変数リストに加えるだけ
 * 
 * @param tok 
 * @param type 
 * @return int 
 */
int add_lvar(Token *tok, Type *type) {
	LVar *lvar = find_lvar(tok);
	if (lvar) error_at(tok->str, "変数名がかぶってます(add_lvar)\n");
	lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	if (lvar_list->offset == 0 && lvar_list->type->_sizeof == 0) lvar->offset = 8;
	else lvar->offset = lvar_list->offset + lvar_list->type->_sizeof;
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

void set_node_kind(Node *node, NodeKind kind) {
	node->kind = kind;
}

Type *new_type(TypeKind typekind, Type *ptr_to, int sz) {
	Type *type = calloc(1, sizeof(Type));
	type->ty = typekind;
	type->ptr_to = ptr_to;
	type->_sizeof = sz;
	return type;
}

Node *expr(void);
Node *unary(void);
Node *stmt(void);

Node *read_funcall(Token *name) {
	if (!consume("(")) return NULL;
	next();
	Node *node = calloc(1, sizeof(Node));
	set_node_kind(node, ND_FUNCALL);
	node->funcname = strndup(name->str, name->len);
	Node *now = node;
	while (!consume_nxt(")")) {
		Node *arg = expr();
		now->next = arg;
		now = arg;
		consume_nxt(",");
	}
	return node;
}

Node *primary(void) {
	if (consume_nxt("(")) {
		Node *node = expr();
		expect_nxt(")");
		return node;
	}
	if (is_ident()) {
		Token *name = token;
		expect_ident_nxt();
		Node *funcall = read_funcall(name);
		if (funcall != NULL) return funcall;
		else return new_node_lvar(name);
	}
	// number
	// マジで????
	if (token->kind == TK_NUM) return new_node_set_num(expect_num_nxt());
	return unary();
}

Node *unary(void) {
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
	} else if (consume_nxt("sizeof")) {
		Node *node = unary();
		type_analyzer(node);
		return new_node_set_num(node->type->_sizeof);
	} else {
		return primary();
	}
}

Node *mul(void) {
	Node *node = unary();
	for (;;) {
		if (consume_nxt("*")) node = new_node_LR(ND_MUL, node, unary());
		else if (consume_nxt("/")) node = new_node_LR(ND_DIV, node, unary());
		else return node;
	}
}

Node *new_add(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (node1->type->ty == TP_INT && node2->type->ty == TP_INT) {
		return new_node_LR(ND_ADD, node1, node2);
	}
	if (node1->type->ty == TP_INT && node2->type->ty == TP_PTR) {
		return new_node_LR(ND_PTR_ADD, node2, node1);
	}
	if (node1->type->ty == TP_PTR && node2->type->ty == TP_INT) {
		return new_node_LR(ND_PTR_ADD, node1, node2);
	}
	if (node1->type->ty == TP_ARRAY && node2->type->ty == TP_INT) {
		return new_node_LR(ND_PTR_ADD, node1, node2);
	}
	if (node1->type->ty == TP_INT && node2->type->ty == TP_ARRAY) {
		return new_node_LR(ND_PTR_ADD, node2, node1);
	}
	error_at(token->str, "何その式(new_add)\n");
	return NULL;
}

Node *new_sub(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (node1->type->ty == TP_INT && node2->type->ty == TP_INT) {
		return new_node_LR(ND_SUB, node1, node2);
	}
	if (node1->type->ty == TP_PTR && node2->type->ty == TP_PTR) {
		return new_node_LR(ND_PTR_DIFF, node1, node2);
	}
	if (node1->type->ty == TP_PTR && node2->type->ty == TP_INT) {
		return new_node_LR(ND_PTR_SUB, node1, node2);
	}
	error("何その式(new_sub)\n");
	return NULL;
}

Node *add(void) {
	Node *node = mul();
	for (;;) {
		if (consume_nxt("+")) node = new_add(node, mul());
		else if (consume_nxt("-")) node = new_sub(node, mul());
		else return node;
	}
}

Node *relational(void) {
	Node *node = add();
	for (;;) {
		if (consume_nxt(">=")) node = new_node_LR(ND_GE, node, add());
		else if (consume_nxt("<=")) node = new_node_LR(ND_LE, node, add());
		else if (consume_nxt(">")) node = new_node_LR(ND_GT, node, add());
		else if (consume_nxt("<")) node = new_node_LR(ND_LT, node, add());
		else return node;
	}
}

Node *equality(void) {
	Node *node = relational();
	for(;;) {
		if (consume_nxt("==")) node = new_node_LR(ND_EQ, node, relational());
		else if (consume_nxt("!=")) node = new_node_LR(ND_NEQ, node, relational());
		return node;
	}
}

Node *assign(void) {
	Node *node = equality();
	if (consume_nxt("=")) node = new_node_LR(ND_ASSIGN, node, assign());
	return node;
}

Node *expr(void) {
	Node *node = assign();
	return node;
}

Type *read_array(Type *ty) {
	if (!consume_nxt("[")) return ty;
	Type *now = calloc(1, sizeof(Type));
	now->ty = TP_ARRAY;
	now->array_size = expect_num_nxt();
	consume_nxt("]");
	ty = read_array(ty);
	now->ptr_to = ty;
	now->_sizeof = now->array_size * ty->_sizeof;
	return now;
}

Node *declaration(void) {
	if (consume_d_type() == 0) return NULL;
	int type_id = get_d_type_id();
	Node *node = calloc(1, sizeof(Node));
	switch (type_id)
	{
	case 0:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = TP_INT;
		node->type->_sizeof = 8;
		break;
	default:
		error("その型は知らん\n");
		break;
	}
	next();
	while (consume_nxt("*")) {
		Type *now_type = new_type(TP_PTR, node->type, 8);
		node->type = now_type;
	}
	Token *var_name = consume_ident_nxt();
	node->type = read_array(node->type);
	if (consume_nxt(";")) {
		node->offset = add_lvar(var_name, node->type);
		node->kind = ND_NULL;
		return node;
	}
	expect_nxt("=");
	node->offset = add_lvar(var_name, node->type);
	set_node_kind(node, ND_LVAR);
	Node *r = equality();
	consume_nxt(";");
	type_analyzer(r);
	return new_node_LR(ND_ASSIGN, node, r);
}

Node *read_return(void) {
	Node *res = new_node_LR(ND_RETURN, expr(), NULL);
	expect_nxt(";");
	return res;
}

Node *read_if(void) {
	expect_nxt("(");
	Node *res = new_node_if(expr(), NULL, NULL);
	expect_nxt(")");
	res->then_stmt = stmt();
	int next_control_id = -1;
	if (consume_cntrl()) next_control_id = get_cntrl_id();
	if (next_control_id == 2) {
		res->else_stmt = stmt();
	}
	return res;
}

Node *read_while(void) {
	expect_nxt("(");
	Node *res = new_node_LR(ND_WHILE, expr(), NULL);
	expect_nxt(")");
	res->rhs = stmt();
	return res;
}

Node *read_for(void) {
	Node *res = new_node_for(NULL, NULL, NULL);
	expect_nxt("(");
	if (!consume_nxt(";")) {
		res->init = expr();
		expect_nxt(";");
	}
	if (!consume_nxt(";")) {
		res->condition = expr();
		expect_nxt(";");
	}
	if (!consume_nxt(")")) {
		res->loop = expr();
		expect_nxt(")");
	}
	res->then_stmt = stmt();
	return res;
}

Node *read_cntrl_flow(void) {
	if (!consume_cntrl()) return NULL;
	int cntrl_id = get_cntrl_id();
	next();
	Node *node;
	switch (cntrl_id) {
		case 0: // return
			node = read_return();
			break;
		case 1: // if
			node = read_if();
			break;
		case 3: // while
			node = read_while();
			break;
		case 4: // for
			node = read_for();
			break;
		default:
			error_at(token->str, "何その制御構文\n");
			break;
	}
	return node;
}

Node *read_block(void) {
	if (!consume("{")) return NULL;
	next();
	Node *res = new_node_LR(ND_BLOCK, NULL, NULL);
	Node *now = res;
	while (!consume_nxt("}")) {
		Node *statement = stmt();
		now->next = statement;
		now = statement;
	}
	now->next = NULL;
	return res;
}

Node *stmt(void) {
	Node *node;
	// control flow
	Node *cntrl = read_cntrl_flow();
	if (cntrl != NULL) return cntrl;
	// declaration
	Node *dec = declaration();
	if (dec != NULL) return dec;
	// block
	Node *block = read_block();
	if (block != NULL) return block;
	// only ";"
	if (consume_nxt(";")) {
		node = NULL;
		return node;
	}
	// expression
	node = expr();
	expect_nxt(";");
	return node;
}

Node *pre_stmt(void) {
	Node *node = stmt();
	type_analyzer(node);
	return node;
}

Function *func_def(void) {
	Function *func = calloc(1, sizeof(Function));
	if (consume_d_type(token) == 0) error_at(token->str, "型を宣言してください\n");
	next();

	if (!is_ident()) error_at(token->str, "関数名を宣言してください\n");
	func->name = strndup(token->str, token->len);
	next();

	// argument
	expect_nxt("(");
	Node **now_arg = &(func->next_arg);
	int arg_cnt = 0;
	while (!consume_nxt(")")) {
		int type_id = get_d_type_id();
		Type *now = calloc(1, sizeof(Type));
		switch (type_id)
		{
		case 0:
			now->ty = TP_INT;
			now->_sizeof = 8;
			break;
		default:
			error_at(token->str, "何その型\n");
			break;
		}
		next();
		while (consume_nxt("*")) {
			Type *now_type = new_type(TP_PTR, now, 8);
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

	// statement
	expect_nxt("{");
	Node **now = &(func->next_stmt);
	while (!consume_nxt("}")) {
		Node *statement = pre_stmt();
		*now = statement;
		now = &(statement->next_stmt);
	}

	// calcurate total offset
	func->total_offset = lvar_list->offset + lvar_list->type->_sizeof;

	return func;
}

void lvar_init(void) {
	lvar_list = calloc(1, sizeof(LVar));
	lvar_list->type = calloc(1, sizeof(Type));
}

void program(void) {
	while (!at_eof()) {
		lvar_init();
		func_gen(func_def());
	}
}