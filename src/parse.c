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

static Var *lvar_list;
Var *gvar_list;
Function *func_list;

////////////////////////////////////////////////////////////////////////////
// variable tool
////////////////////////////////////////////////////////////////////////////

Function *find_func(char *name) {
	for (Function *now = func_list; now; now = now->next) {
		if (now->name != NULL && strcmp(name, now->name) == 0) {
			return now;
		}
	}
	// error("そんな関数はない\n");
	return NULL;
}

/**
 * @brief tok->strと一致するようなグローバル変数を探す
 * 
 * @param tok 
 * @return Var* 
 */
static Var *find_gvar(Token *tok) {
	for (Var *now = gvar_list; now; now = now->next) {
		if (tok->len == now->len && memcmp(tok->str, now->name, tok->len) == 0) {
			return now;
		}
	}
	return NULL;
}

/**
 * @brief tok->strと一致するようなローカル変数を探す
 * 
 * @param tok 
 * @return LVar* 
 */
static Var *find_lvar(Token *tok) {
	for (Var *now = lvar_list; now; now = now->next) {
		if (tok->len == now->len && memcmp(tok->str, now->name, tok->len) == 0) {
			return now;
		}
	}
	return NULL;
}
/**
 * @brief グローバル変数リストに加えるだけ
 * 
 * @param tok 
 * @param type 
 * @return int 
 */
static int add_gvar(Token *tok, Type *type) {
	Var *gvar = find_gvar(tok);
	if (gvar) error_at(tok->str, "変数名がかぶってます(add_gvar)\n");
	gvar = calloc(1, sizeof(Var));
	gvar->name = strndup(tok->str, tok->len);
	gvar->len = tok->len;
	if (gvar_list->offset == 0 && gvar_list->type->_sizeof == 0) gvar->offset = 8;
	else gvar->offset = gvar_list->offset + gvar_list->type->_sizeof;
	gvar->next = gvar_list;
	gvar->type = type;
	gvar_list = gvar;
	return gvar->offset;
}

/**
 * @brief ローカル変数リストに加えるだけ
 * 
 * @param tok 
 * @param type 
 * @return int 
 */
static int add_lvar(Token *tok, Type *type) {
	Var *lvar = find_lvar(tok);
	if (lvar) error_at(tok->str, "変数名がかぶってます(add_lvar)\n");
	lvar = calloc(1, sizeof(Var));
	lvar->name = tok->str;
	lvar->len = tok->len;
	if (lvar_list->offset == 0 && lvar_list->type->_sizeof == 0) lvar->offset = 8;
	else lvar->offset = lvar_list->offset + lvar_list->type->_sizeof;
	lvar->next = lvar_list;
	lvar->type = type;
	lvar_list = lvar;
	return lvar->offset;
}

static void add_func(Function *func) {
	Function *f = find_func(func->name);
	if (f) error("関数名がかぶってます(add_func)\n");
	func->next = func_list;
	func_list = func;
}

/**
 * @brief tok->strという変数のオフセットを計算し、ノードを作成
 * 
 * @param tok 
 * @return Node* 
 */
static Node *new_node_var(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	Var *var = find_lvar(tok);
	if (var) {
		node->kind = ND_LVAR;
		node->offset = var->offset;
		node->type = var->type;
		return node;
	} 
	var = find_gvar(tok);
	if (var) {
		node->kind = ND_GVAR;
		node->type = var->type;
		node->var_name = var->name;
		return node;
	} else error("知らない変数です\n");
	return NULL;
}

/**
 * @brief 変数宣言と同時に初期化もするようなコードに対する処理
 * 
 * @param tok 
 * @param type 
 * @return Node* 
 */
static Node *new_node_lvar_dec(Token *tok, Type *type) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	node->offset = add_lvar(tok, type);
	node->type = type;
	return node;
}

////////////////////////////////////////////////////////////////////////////
// new node tool
////////////////////////////////////////////////////////////////////////////

static void set_node_kind(Node *node, NodeKind kind) {
	node->kind = kind;
}

static Node *new_node_LR(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	set_node_kind(node, kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static Node *new_node_set_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	set_node_kind(node, ND_NUM);
	node->val = val;
	return node;
}

static Node *new_node_if(Node *condition, Node *then_stmt, Node *else_stmt) {
	Node *node = calloc(1, sizeof(Node));
	set_node_kind(node, ND_IF);
	node->condition = condition;
	node->then_stmt = then_stmt;
	node->else_stmt = else_stmt;
	return node;
}

static Node *new_node_for(Node *init, Node *condition, Node *loop) {
	Node *node = calloc(1, sizeof(Node));
	set_node_kind(node, ND_FOR);
	node->init = init;
	node->condition = condition;
	node->loop = loop;
	return node;
}

////////////////////////////////////////////////////////////////////////////
// construct abstract syntax tree
////////////////////////////////////////////////////////////////////////////
static Node *expr(void);
static Node *unary(void);
static Node *stmt(void);
static Node *pre_stmt(void);
static Node *new_add(Node *node1, Node *node2);

static Node *read_funcall(Token *name) {
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

static Type *read_ptr(Type *basetype) {
	Type *now = basetype;
	while (consume_nxt("*")) {
		Type *tmp = new_type(TP_PTR, now, 8);
		now = tmp;
	}
	return now;
}

static Type *read_array(Type *ty) {
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

static Node *read_basetype() {
	if (consume_d_type() == 0) return NULL;
	int type_id = get_d_type_id();
	Node *node = calloc(1, sizeof(Node));
	switch (type_id)
	{
	case 0: // int
		node->type = new_type(TP_INT, NULL, 8);
		break;
	case 1: // char
		node->type = new_type(TP_CHAR, NULL, 1);
		break;
	default:
		error("その型は知らん\n");
		break;
	}
	next();
	return node;
}

static Node *read_return(void) {
	Node *res = new_node_LR(ND_RETURN, expr(), NULL);
	expect_nxt(";");
	return res;
}

static Node *read_if(void) {
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

static Node *read_while(void) {
	expect_nxt("(");
	Node *res = new_node_LR(ND_WHILE, expr(), NULL);
	expect_nxt(")");
	res->rhs = stmt();
	return res;
}

static Node *read_for(void) {
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

static Node *read_cntrl_flow(void) {
	if (!consume_cntrl()) return NULL;
	int cntrl_id = get_cntrl_id();
	next();
	Node *node;
	switch (cntrl_id)
	{
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

static Node *read_block(void) {
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

static bool read_argument(Function *func) {
	if (!consume_nxt("(")) return false;
	Node **now_arg = &(func->arg);
	int arg_cnt = 0;
	while (!consume_nxt(")")) {
		Type *now = read_basetype()->type;
		now = read_ptr(now);
		expect_ident();
		Node *arg = new_node_lvar_dec(token, now);
		arg->kind = ND_ARG;
		*now_arg = arg;
		now_arg = &(arg->next_arg);
		arg_cnt++;
		next();
		consume_nxt(",");
	}
	func->arg_count = arg_cnt;
	return true;
}

static void read_stmt(Function *func) {
	expect_nxt("{");
	Node **now = &(func->stmt);
	while (!consume_nxt("}")) {
		Node *statement = pre_stmt();
		*now = statement;
		now = &(statement->next_stmt);
	}
}

static Node *primary(void) {
	// "(" expression ")"
	if (consume_nxt("(")) {
		Node *node = expr();
		expect_nxt(")");
		return node;
	}
	// function call or variable
	if (is_ident()) {
		Token *name = token;
		expect_ident_nxt();
		Node *funcall = read_funcall(name);
		if (funcall != NULL) return funcall;
		else return new_node_var(name);
	}
	// number
	// マジで????
	if (token->kind == TK_NUM) return new_node_set_num(expect_num_nxt());
	return unary();
}

static Node *postfix(void) {
	Node *node1 = primary();
	while (consume_nxt("[")) {
		Node *node2 = expr();
		expect_nxt("]");
		Node *add = new_add(node1, node2);
		node1 = new_node_LR(ND_DEREF, add, NULL);
	}
	return node1;
}

static Node *unary(void) {
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
		return postfix();
	}
}

static Node *mul(void) {
	Node *node = unary();
	for (;;) {
		if (consume_nxt("*")) node = new_node_LR(ND_MUL, node, unary());
		else if (consume_nxt("/")) node = new_node_LR(ND_DIV, node, unary());
		else return node;
	}
}

static Node *new_add(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (is_int(node1->type) && is_int(node2->type)) {
		return new_node_LR(ND_ADD, node1, node2);
	}
	if (is_int(node1->type) && is_ptr(node2->type)) {
		return new_node_LR(ND_PTR_ADD, node2, node1);
	}
	if (is_ptr(node1->type) && is_int(node2->type)) {
		return new_node_LR(ND_PTR_ADD, node1, node2);
	}
	if (is_array(node1->type) && is_int(node2->type)) {
		return new_node_LR(ND_PTR_ADD, node1, node2);
	}
	if (is_int(node1->type) && is_array(node2->type)) {
		return new_node_LR(ND_PTR_ADD, node2, node1);
	}
	error_at(token->str, "何その式(new_add)\n");
	return NULL;
}

static Node *new_sub(Node *node1, Node *node2) {
	type_analyzer(node1);
	type_analyzer(node2);
	if (is_int(node1->type) && is_int(node2->type)) {
		return new_node_LR(ND_SUB, node1, node2);
	}
	if (is_ptr(node1->type) && is_ptr(node2->type)) {
		return new_node_LR(ND_PTR_DIFF, node1, node2);
	}
	if (is_ptr(node1->type) && is_int(node2->type)) {
		return new_node_LR(ND_PTR_SUB, node1, node2);
	}
	error("何その式(new_sub)\n");
	return NULL;
}

static Node *add(void) {
	Node *node = mul();
	for (;;) {
		if (consume_nxt("+")) node = new_add(node, mul());
		else if (consume_nxt("-")) node = new_sub(node, mul());
		else return node;
	}
}

static Node *relational(void) {
	Node *node = add();
	for (;;) {
		if (consume_nxt(">=")) node = new_node_LR(ND_GE, node, add());
		else if (consume_nxt("<=")) node = new_node_LR(ND_LE, node, add());
		else if (consume_nxt(">")) node = new_node_LR(ND_GT, node, add());
		else if (consume_nxt("<")) node = new_node_LR(ND_LT, node, add());
		else return node;
	}
}

static Node *equality(void) {
	Node *node = relational();
	for(;;) {
		if (consume_nxt("==")) node = new_node_LR(ND_EQ, node, relational());
		else if (consume_nxt("!=")) node = new_node_LR(ND_NEQ, node, relational());
		return node;
	}
}

static Node *assign(void) {
	Node *node = equality();
	if (consume_nxt("=")) node = new_node_LR(ND_ASSIGN, node, assign());
	return node;
}

static Node *expr(void) {
	Node *node = assign();
	return node;
}

static Node *lvar_declaration(void) {
	Node *node = read_basetype();
	if (node == NULL) return NULL;
	// add pointer
	node->type = read_ptr(node->type);
	// variable name
	Token *var_name = consume_ident_nxt();
	node->type = read_array(node->type);
	if (consume_nxt(";")) {
		// declaration only
		add_lvar(var_name, node->type);
		node->kind = ND_NULL;
		return node;
	}
	// variable initialization
	expect_nxt("=");
	node = new_node_lvar_dec(var_name, node->type);
	Node *r = equality();
	consume_nxt(";");
	type_analyzer(r);
	return new_node_LR(ND_ASSIGN, node, r);
}

static Node *stmt(void) {
	Node *node;
	// control flow
	Node *cntrl = read_cntrl_flow();
	if (cntrl != NULL) return cntrl;
	// declaration
	Node *dec = lvar_declaration();
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

static Node *pre_stmt(void) {
	Node *node = stmt();
	type_analyzer(node);
	return node;
}

static Function *func_def(Type *base, char *name) {
	Function *func = calloc(1, sizeof(Function));
	func->name = name;
	func->type = base;
	// argument
	if (!read_argument(func)) return NULL;
	// statement
	read_stmt(func);
	// calcurate total offset
	func->total_offset = lvar_list->offset + lvar_list->type->_sizeof;
	add_func(func);
	return func;
}

static void gvar_declaration(Token *tok, Type *base) {
	base = read_array(base);
	add_gvar(tok, base);
	expect_nxt(";");
}

static Function *gvar_or_func_def(void) {
	Node *basetype = read_basetype();
	if (basetype == NULL) error_at(token->str, "型を宣言しろ\n");
	// read pointer
	basetype->type = read_ptr(basetype->type);
	expect_ident();
	// read name
	char *name = strndup(token->str, token->len);
	Token *tok = token;
	next();
	// function define
	Function *func = func_def(basetype->type, name);
	if (func != NULL) return func;
	// global variable
	gvar_declaration(tok, basetype->type);
	return NULL;
}

static void lvar_init(void) {
	lvar_list = calloc(1, sizeof(Var));
	lvar_list->type = calloc(1, sizeof(Type));
}

static void gvar_init(void) {
	gvar_list = calloc(1, sizeof(Var));
	gvar_list->type = calloc(1, sizeof(Type));
	gvar_list->is_write = true;
}

static void func_init(void) {
	func_list = calloc(1, sizeof(Function));
	func_list->type = calloc(1, sizeof(Type));
}

void program(void) {
	gvar_init();
	func_init();
	while (!at_eof()) {
		lvar_init();
		func_gen(gvar_or_func_def());
	}
}