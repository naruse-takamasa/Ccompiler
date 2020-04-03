/**
 * @file type_analyze.c
 * @author Takamasa Naruse
 * @brief analyze variable type
 * @version 0.1
 * @date 2020-03-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "SverigeCC.h"

Type *new_type(TypeKind typekind, Type *ptr_to, int sz) {
	Type *type = calloc(1, sizeof(Type));
	type->ty = typekind;
	type->ptr_to = ptr_to;
	type->_sizeof = sz;
	return type;
}

bool is_int(Type *type) {
	return type->ty == TP_INT || type->ty == TP_CHAR;
}

bool is_ptr(Type *type) {
	return type->ty == TP_PTR;
}

bool is_array(Type *type) {
	return type->ty == TP_ARRAY;
}

bool is_char(Type *type) {
	return type->ty == TP_CHAR;
}

void type_analyzer(Node *node) {
	if (node == NULL) return;
	type_analyzer(node->lhs);
	type_analyzer(node->rhs);
	type_analyzer(node->condition);
	type_analyzer(node->then_stmt);
	type_analyzer(node->else_stmt);
	type_analyzer(node->init);
	type_analyzer(node->loop);
	type_analyzer(node->next_arg);
	type_analyzer(node->next_stmt);
	type_analyzer(node->next);

	switch (node->kind)
	{
	case ND_ADD:
	case ND_SUB:
	case ND_DIV:
	case ND_MUL:
	case ND_EQ:
	case ND_NEQ:
	case ND_LE:
	case ND_LT:
	case ND_GE:
	case ND_GT:
	case ND_FUNCALL:
	case ND_PTR_DIFF:
	case ND_NUM:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = TP_INT;
		node->type->_sizeof = 8;
		return;
	case ND_ASSIGN:
	case ND_PTR_ADD:
	case ND_PTR_SUB:
		node->type = node->lhs->type;
		return;
	case ND_ADDR:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = TP_PTR;
		node->type->ptr_to = node->lhs->type;
		node->type->_sizeof = 8;
		return;
	case ND_DEREF:
		node->type = node->lhs->type->ptr_to;
		return;
	default:
		break;
	}
}