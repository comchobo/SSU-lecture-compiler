#include <stdio.h>
#include <stdlib.h>
#include "type.h"
FILE* fout;
extern int syntax_err;
extern int semantic_err;
extern A_NODE* root;
extern FILE* yyin;
void initialize();
void print_sem_ast();
void print_ast();
void code_generation();
void main(int argc, char* argv[])
{
	char s[10];
	printf("출력할 파일명을 작성하세요\n");
	scanf("%s", s);
	if ((yyin = fopen(s, "r")) == NULL) {
		printf("can not open input file: %s\n", argv[argc - 1]);
		exit(1);
	}
	char t[10];
	printf("출력할 파일명을 작성하세요\n");
	scanf("%s", t);
	if ((fout = fopen(t, "w")) == NULL) {
		printf("cannot open output file");
		exit(1);
	}

	printf("\nstart syntax analysis\n");
	initialize();
	yyparse();
	//print_ast(root);
	if (syntax_err) exit(1);
	printf("\nstart semantic analysis\n");
	semantic_analysis(root);
	if (semantic_err) exit(1);
	printf("\nstart code generation\n");
	code_generation(root);
	exit(0);
}

