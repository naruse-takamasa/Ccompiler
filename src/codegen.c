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

static char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static int Label_id = 0;
static int Total_offset;

static void gen(Node *node);

static void load(Type *type) {
	printf("  pop rax\n");
	if (type->_sizeof == 8) {
		printf("  mov rax, [rax]\n");
	} else {
		printf("  movsx rax, byte ptr [rax]\n");
	}
		printf("  push rax\n");
}

static void store(Type *type) {
	printf("  pop rdi\n");
	printf("  pop rax\n");
	if (type->_sizeof == 8) {
		printf("  mov [rax], rdi\n");
	} else {
		printf("  mov [rax], dil\n");
	}
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
	if (node->kind != ND_LVAR && node->kind != ND_GVAR) {
		error("代入の左辺値が変数ではありません\n");
	}
	if (node->kind == ND_GVAR) {
		printf("  push offset %s\n", node->var_name);
		return;
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
		if (node->type->ty != TP_ARRAY) load(node->type);
		return;
	case ND_GVAR:
		addr_gen(node);
		if (node->type->ty != TP_ARRAY) load(node->type);
		return;
	case ND_ASSIGN:
		addr_gen(node->lhs);
		gen(node->rhs);
		store(node->type);
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
		// だから、"  push rax"して直近に取り出されたやつだけまたpushする.
		printf("  push rax\n");
		return;
	case ND_FUNCALL:
		{
			int arg_count = 0;
			for (Node *now = node->next; now ; now = now->next) {
				gen(now);
				arg_count++;
			}
			// Function *func = find_func(node->funcname);
			// int arg1_cnt = 0, arg8_cnt = 0;
			// int arg_kind[100] = {};
			// int idx = 0;
			// for (Node *now = func->arg; now; now = now->next_arg) {
			// 	if (is_char(now->type)) {
			// 		arg_kind[idx] = 1;
			// 		arg1_cnt++;
			// 	} else {
			// 		arg_kind[idx] = 8;
			// 		arg8_cnt++;
			// 	}
			// 	idx++;
			// }
			// for (int i = idx - 1; i >= 0; i--) {
			// 	switch (arg_kind[i])
			// 	{
			// 	case 1:
			// 		printf("  pop %s\n", argreg1[arg1_cnt - 1]);
			// 		arg1_cnt--;
			// 		break;
			// 	case 8:
			// 		printf("  pop %s\n", argreg8[arg8_cnt - 1]);
			// 		arg8_cnt--;
			// 		break;
			// 	default:
			// 		error("なにその型(gen)\n");
			// 		break;
			// 	}
			// }
			for (int i = arg_count - 1; i >= 0; i--) {
				// TODO:intとcharに対応すること
				printf("  pop %s\n", argreg8[i]);
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
		if (node->type->ty != TP_ARRAY) load(node->type);
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

static void gvar_gen() {
	printf(".data\n");
	for (Var *now = gvar_list; now->is_write == false; now = now->next) {
		printf("%s:\n", now->name);
		printf("  .zero %d\n", now->type->_sizeof);
		now->is_write = true;
	}
}

static int calc_align(int offset, int align) {
	return (offset + align - 1) & ~(align - 1);
}

void func_gen(Function *func) {
	if (func == NULL) {
		gvar_gen();
		return;
	}
	func->total_offset = calc_align(func->total_offset, 8);
	printf(".text\n");
	printf(".global %s\n", func->name);
	printf("%s:\n", func->name);

	// prologue
	// ローカル変数領域の確保
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	printf("  sub rsp, %d\n", func->total_offset);

	Total_offset = func->total_offset;
	// このアドレスの並びであってるのかな-??
	// for (int i = 0; i < func->arg_count; i++) {
	// 	printf("  mov rax, rbp\n");
	// 	printf("  sub rax, %d\n", Total_offset);
	// 	printf("  add rax, %d\n", i * 8);
	// 	printf("  mov [rax], %s\n", argreg[i]);
	// }
	fprintf(stderr, "arg\n");
	int arg1_idx = 0, arg8_idx = 0;
	for (Node *now = func->arg; now; now = now->next_arg) {
		// TODO:
		printf("  mov rax, rbp\n");
		printf("  sub rax, %d\n", Total_offset + 8);
		printf("  add rax, %d\n", now->offset);
		if (is_char(now->type)) printf("  mov [rax], %s\n", argreg1[arg1_idx++]);
		else printf("  mov [rax], %s\n", argreg8[arg8_idx++]);
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