/**
 * @file codegen.c
 * @author Takamasa Naruse
 * @brief 抽象構文木からアセンブリを出力
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"
#include <stdio.h>

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません\n");
	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", node->offset);
	printf("  push rax\n");
}
// TODO:
int Label_id = 0;

void gen(Node *node) {
	switch (node->kind)
	{
	case ND_NUM:
		printf("  push %d\n", node->val);
		return;
	case ND_LVAR:
		gen_lval(node);
		printf("  pop rax\n");
		printf("  mov rax, [rax]\n");
		printf("  push rax\n");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		printf("  pop rdi\n");
		printf("  pop rax\n");
		printf("  mov [rax], rdi\n");
		printf("  push rdi\n");
		return;
	case ND_RETURN:
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  mov rsp, rbp\n");
		printf("  pop rbp\n");
		printf("  ret\n");
		return;
	case ND_IF:
		gen(node->condition);
		printf("  pop rax\n");
		printf("  cmp rax, 0\n");
		if (node->else_stmt == NULL) {
			printf("  je .Lend%d\n", Label_id);
			if (node->then_stmt != NULL) gen(node->then_stmt);
			printf(".Lend%d:\n", Label_id);
		} else {
			printf("  je .Lelse%d\n", Label_id);
			if (node->then_stmt != NULL) gen(node->then_stmt);
			printf("  jmp .Lend%d\n", Label_id);
			printf(".Lelse%d:\n", Label_id);
			if (node->else_stmt != NULL) gen(node->else_stmt);
			printf(".Lend%d:\n", Label_id);
		}
		Label_id++;
		return;
	case ND_WHILE:
		printf(".Lbegin%d:\n", Label_id);
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  cmp rax, 0\n");
		printf("  je .Lend%d\n", Label_id);
		if (node->rhs != NULL) gen(node->rhs);
		printf("  jmp .Lbegin%d\n", Label_id);
		printf(".Lend%d:\n", Label_id);
		Label_id++;
		return;
	case ND_FOR:
		if (node->init != NULL) gen(node->init);
		printf(".Lbegin%d:\n", Label_id);
		if (node->condition != NULL) {
			gen(node->condition);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je .Lend%d\n", Label_id);
		}
		if (node->then_stmt != NULL) gen(node->then_stmt);
		if (node->loop != NULL) gen(node->loop);
		printf("  jmp .Lbegin%d\n", Label_id);
		printf(".Lend%d:\n", Label_id);
		Label_id++;
		return;
	default:
		break;
	}
	if (node->kind == ND_GE) {
		gen(node->rhs);
		gen(node->lhs);
		node->kind = ND_LE;
	} else if (node->kind == ND_GT) {
		gen(node->rhs);
		gen(node->lhs);
		node->kind = ND_LT;
	} else {
		gen(node->lhs);
		gen(node->rhs);
	}
	printf("  pop rdi\n");
	printf("  pop rax\n");
	switch (node->kind)
	{
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("  imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("  cqo\n");
		printf("  idiv rdi\n");
		break;
	case ND_EQ:
		printf("  cmp rax, rdi\n");
		printf("  sete al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_NEQ:
		printf("  cmp rax, rdi\n");
		printf("  setne al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LE:
		printf("  cmp rax, rdi\n");
		printf("  setle al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LT:
		printf("  cmp rax, rdi\n");
		printf("  setl al\n");
		printf("  movzb rax, al\n");
		break;
	default:
		break;
	}
	printf("  push rax\n");
}
