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

/**
 * @brief 変数のアドレスを調べて、その値をpushする
 * 
 * @param node 
 */
void gen_lval(Node *node) {
	if (node->kind == ND_DEREF) return gen(node->lhs);
	if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません\n");
	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", node->offset);
	printf("  push rax\n");
}

char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
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
		// returnされた結果はraxにある.
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
			// rspが16の倍数かどうかの判定
			printf("  mov rax, rsp\n");
			printf("  and rax, 15\n");
			printf("  jnz .Lcall%d\n", Label_id);
			// rspが16の倍数だった場合
			printf("  call %s\n", node->funcname);
			printf("  jmp .Lend%d\n", Label_id);
			// rspが16の倍数ではなかった場合
			printf(".Lcall%d:\n", Label_id);
			printf("  sub rsp, 8\n");
			printf("  mov rax, 0\n");
			printf("  call %s\n", node->funcname);
			printf("  add rsp, 8\n");
			// 最後に呼び出した関数の結果をpushする
			printf(".Lend%d:\n", Label_id);
			printf("  push rax\n");
			Label_id++;
		}
		return;
	case ND_ADDR:
		gen_lval(node->lhs);
		return;
	case ND_DEREF:
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  mov rax, [rax]\n");
		printf("  push rax\n");
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

void func_gen(Function *func) {
	printf("%s:\n", func->name);

	// プロローグ
	// ローカル変数領域の確保
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	printf("  sub rsp, %d\n", func->total_offset);

	// 引数の値をローカル変数領域に移動
	// for (int i = 0; i < func_arg_count[node->offset]; i++) {
	// 	printf("  mov rax, rbp\n");
	// 	printf("  sub rax, %d\n", (i + 1) * 8);
	// 	printf("  mov [rax], %s\n", argreg[i]);
	// }
	for (int i = 0; i < func->arg_count; i++) {
		printf("  mov rax, rbp\n");
		printf("  sub rax, %d\n", (i + 1) * 8);
		printf("  mov [rax], %s\n", argreg[i]);
	}

	// 本文
	for (Node *now = func->next_stmt; now; now = now->next_stmt) {
		gen(now);
	}

	// エピローグ
	printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
	return;
}