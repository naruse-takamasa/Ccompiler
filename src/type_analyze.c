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
		node->type->ty = INT;
		return;
	case ND_ASSIGN:
	case ND_PTR_ADD:
	case ND_PTR_SUB:
		node->type = calloc(1, sizeof(Type));
		node->type = node->lhs->type;
		return;
	case ND_ADDR:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = PTR;
		node->type->ptr_to = node->lhs->type;
		return;
	case ND_DEREF:
		node->type = calloc(1, sizeof(Type));
		node->type->ty = PTR;
		if (node->lhs->type->ty == PTR) node->type->ty = PTR;
		else node->type->ty = INT;
		return;
	case ND_LVAR:
		return;
	default:
		break;
	}
}