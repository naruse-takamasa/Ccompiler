/**
 * @file codegen.c
 * @author Takamasa Naruse
 * @brief output assembly langurage
 * @version 0.1
 * @date 2020-03-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "SverigeCC.h"

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static int Label_id = 0;
static int Total_offset;

static void gen(Node *node);

static void load() {
	printf("  pop rax\n");
	printf("  mov rax, [rax]\n");
	printf("  push rax\n");
}

static void store() {
	printf("  pop rdi\n");
	printf("  pop rax\n");
	printf("  mov [rax], rdi\n");
	printf("  push rdi\n");
}

/**
 * @brief push variable address
 * 
 * @param node 
 */
static void addr_gen(Node *node) {
	if (node->kind == ND_DEREF) {
		gen(node->lhs);
		return;
	}
	if (node->kind != ND_LVAR) {
		error("代入の左辺値が変数ではありません\n");
	}
	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", Total_offset + 8);
	printf("  add rax, %d\n", node->offset);
	printf("  push rax\n");
}

static void gen(Node *node) {
	switch (node->kind)
	{
	case ND_NUM:
		printf("  push %d\n", node->val);
		return;
	case ND_LVAR:
		addr_gen(node);
		if (node->type->ty != TP_ARRAY) load();
		return;
	case ND_ASSIGN:
		addr_gen(node->lhs);
		gen(node->rhs);
		store();
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
		if (node->else_stmt) {
			printf("  je .Lend%d\n", Label_id);
			if (node->then_stmt) gen(node->then_stmt);
			printf(".Lend%d:\n", Label_id);
		} else {
			printf("  je .Lelse%d\n", Label_id);
			if (node->then_stmt) gen(node->then_stmt);
			printf("  jmp .Lend%d\n", Label_id);
			printf(".Lelse%d:\n", Label_id);
			if (node->else_stmt) gen(node->else_stmt);
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
		if (node->rhs) gen(node->rhs);
		printf("  jmp .Lbegin%d\n", Label_id);
		printf(".Lend%d:\n", Label_id);
		Label_id++;
		return;
	case ND_FOR:
		if (node->init) gen(node->init);
		printf(".Lbegin%d:\n", Label_id);
		if (node->condition) {
			gen(node->condition);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je .Lend%d\n", Label_id);
		}
		if (node->then_stmt) gen(node->then_stmt);
		if (node->loop) gen(node->loop);
		printf("  jmp .Lbegin%d\n", Label_id);
		printf(".Lend%d:\n", Label_id);
		Label_id++;
		return;
	case ND_BLOCK:
		for (Node *now = node->next; now ; now = now->next) {
			gen(now);
			printf("  pop rax\n");
		}
		// main関数で"  pop rax"が必ず実行されるので、for文で全部"  pop rax"するとマズい.
		// だから、"  push rax"して直近に取り出されたやつだけまたpushしてある.
		printf("  push rax\n");
		return;
	case ND_FUNCALL:
		{
			int arg_count = 0;
			for (Node *now = node->next; now ; now = now->next) {
				gen(now);
				arg_count++;
			}
			for (int i = arg_count - 1; i >= 0; i--) {
				printf("  pop %s\n", argreg[i]);
			}
			// 仕様上rspが16の倍数で関数をcallしなくてはならない
			printf("  mov rax, rsp\n");
			printf("  and rax, 15\n");
			printf("  jnz .Lcall%d\n", Label_id);
			printf("  call %s\n", node->funcname);
			printf("  jmp .Lend%d\n", Label_id);
			printf(".Lcall%d:\n", Label_id);
			printf("  sub rsp, 8\n");
			printf("  mov rax, 0\n");
			printf("  call %s\n", node->funcname);
			printf("  add rsp, 8\n");
			printf(".Lend%d:\n", Label_id);
			printf("  push rax\n");
			Label_id++;
		}
		return;
	case ND_ADDR:
		addr_gen(node->lhs);
		return;
	case ND_DEREF:
		gen(node->lhs);
		if (node->type->ty != TP_ARRAY) load();
		return;
	case ND_NULL:
		return;
	default:
		break;
	}

	// 比較演算の記号のひっくり返し
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

	// ここ以降は算術演算と比較
	// 算術と比較は最後に必ずpushされる
	printf("  pop rdi\n");
	printf("  pop rax\n");
	switch (node->kind)
	{
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_PTR_ADD:
		if (node->lhs->type->ty == TP_ARRAY) {
			printf("  imul rdi, %d\n", node->lhs->type->ptr_to->_sizeof);
		} else printf("  imul rdi, %d\n", node->lhs->type->_sizeof);
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_PTR_SUB:
		printf("  imul rdi, %d\n", node->lhs->type->_sizeof);
		printf("  sub rax, rdi\n");
		break;
	case ND_PTR_DIFF:
		printf("  sub rax, rdi\n");
		printf("  cqo\n");
		printf("  mov rdi, %d\n", node->lhs->type->_sizeof);
		printf("  idiv rdi\n");
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

void func_gen(Function *func) {
	if (func == NULL) return;
	printf("%s:\n", func->name);

	// prologue
	// ローカル変数領域の確保
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	printf("  sub rsp, %d\n", func->total_offset);

	Total_offset = func->total_offset;
	// このアドレスの並びであってるのかな-??
	for (int i = 0; i < func->arg_count; i++) {
		printf("  mov rax, rbp\n");
		printf("  sub rax, %d\n", Total_offset);
		printf("  add rax, %d\n", i * 8);
		printf("  mov [rax], %s\n", argreg[i]);
	}

	for (Node *now = func->stmt; now; now = now->next_stmt) {
		gen(now);
	}

	// epilogue
	printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
	return;
}