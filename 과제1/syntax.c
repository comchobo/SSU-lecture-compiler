#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "y.tab.h"
extern char* yytext;
A_TYPE* int_type, * char_type, * void_type, * float_type, * string_type;
A_NODE* root;
A_ID* current_id = NIL;
int syntax_err = 0;
int line_no = 1;
int current_level = 0;
A_NODE* makeNode(NODE_NAME, A_NODE*, A_NODE*, A_NODE*);
A_NODE* makeNodeList(NODE_NAME, A_NODE*, A_NODE*);
A_ID* makeIdentifier(char*);
A_ID* makeDummyIdentifier();
A_TYPE* makeType(T_KIND);
A_SPECIFIER* makeSpecifier(A_TYPE*, S_KIND);
A_ID* searchIdentifier(char*, A_ID*);
A_ID* searchIdentifierAtCurrentLevel(char*, A_ID*);
A_SPECIFIER* updateSpecifier(A_SPECIFIER*, A_TYPE*, S_KIND);
void checkForwardReference();
void setDefaultSpecifier(A_SPECIFIER*);
A_ID* linkDeclaratorList(A_ID*, A_ID*);
A_ID* getIdentifierDeclared(char*);
A_TYPE* getTypeOfStructOrEnumRefIdentifier(T_KIND, char*, ID_KIND);
A_ID* setDeclaratorInit(A_ID*, A_NODE*);
A_ID* setDeclaratorKind(A_ID*, ID_KIND);
A_ID* setDeclaratorType(A_ID*, A_TYPE*);
A_ID* setDeclaratorElementType(A_ID*, A_TYPE*);
A_ID* setDeclaratorTypeAndKind(A_ID*, A_TYPE*, ID_KIND);
A_ID* setDeclaratorListSpecifier(A_ID*, A_SPECIFIER*);
A_ID* setFunctionDeclaratorSpecifier(A_ID*, A_SPECIFIER*);
A_ID* setFunctionDeclaratorBody(A_ID*, A_NODE*);
A_ID* setParameterDeclaratorSpecifier(A_ID*, A_SPECIFIER*);
A_ID* setStructDeclaratorListSpecifier(A_ID*, A_TYPE*);
A_TYPE* setTypeNameSpecifier(A_TYPE*, A_SPECIFIER*);
A_TYPE* setTypeElementType(A_TYPE*, A_TYPE*);
A_TYPE* setTypeField(A_TYPE*, A_ID*);
A_TYPE* setTypeExpr(A_TYPE*, A_NODE*);
A_TYPE* setTypeAndKindOfDeclarator(A_TYPE*, ID_KIND, A_ID*);
A_TYPE* setTypeStructOrEnumIdentifier(T_KIND, char*, ID_KIND);
BOOLEAN isNotSameFormalParameters(A_ID*, A_ID*);
BOOLEAN isNotSameType(A_TYPE*, A_TYPE*);
BOOLEAN isPointerOrArrayType(A_TYPE*);
void syntax_error(int a, char *s);
void initialize();
// make new node for syntax tree
A_NODE* makeNode(NODE_NAME n, A_NODE* a, A_NODE* b, A_NODE* c) {
	A_NODE* m;
	m = (A_NODE*)malloc(sizeof(A_NODE));
	m->name = n;
	m->llink = a;
	m->clink = b;
	m->rlink = c;
	m->type = NIL;
	m->line = line_no;
	m->value = 0;
	return (m);
}
A_NODE* makeNodeList(NODE_NAME n, A_NODE* a, A_NODE* b) {
	// 리스트 형태의 신택스 트리 마지막에 신택스 트리 b를 추가 연결
	A_NODE* m, * k;
	k = a;
	while (k->rlink)
		k = k->rlink;
	m = (A_NODE*)malloc(sizeof(A_NODE));
	m->name = k->name;
	m->llink = NIL;
	m->clink = NIL;
	m->rlink = NIL;
	m->type = NIL;
	m->line = line_no;
	m->value = 0;
	k->name = n;
	k->llink = b;
	k->rlink = m;
	return(a);
}
// make a new declarator for identifier
A_ID* makeIdentifier(char* s) {
	A_ID* id;
	id = malloc(sizeof(A_ID));
	id->name = s;
	id->kind = 0;
	id->specifier = 0;
	id->level = current_level;
	id->address = 0;
	id->init = NIL;
	id->type = NIL;
	id->link = NIL;
	id->line = line_no;
	id->value = 0;
	id->prev = current_id;
	current_id = id;
	return(id);
}
// make a new declarator for dummy identifier
A_ID* makeDummyIdentifier() {
	A_ID* id;
	id = malloc(sizeof(A_ID));
	id->name = "";
	id->kind = 0;
	id->specifier = 0;
	id->level = current_level;
	id->address = 0;
	id->init = NIL;
	id->type = NIL;
	id->link = NIL;
	id->line = line_no;
	id->value = 0;
	id->prev = 0;
	return(id);
}
// make a new type
A_TYPE* makeType(T_KIND k) {
	A_TYPE* t;
	t = malloc(sizeof(A_TYPE));
	t->kind = k;
	t->size = 0;
	t->local_var_size = 0;
	t->element_type = NIL;
	t->field = NIL;
	t->expr = NIL;
	t->check = FALSE;
	t->prt = FALSE;
	t->line = line_no;
	return(t);
}
// make a new specifier
A_SPECIFIER* makeSpecifier(A_TYPE* t, S_KIND s) {
	A_SPECIFIER* p;
	p = malloc(sizeof(A_SPECIFIER));
	p->type = t;
	p->stor = s;
	p->line = line_no;
	return(p);
}
A_ID* searchIdentifier(char* s, A_ID* id) {
	while (id) {
		if (strcmp(id->name, s) == 0) break;
		id = id->prev;
	}
	return(id);
}
A_ID* searchIdentifierAtCurrentLevel(char* s, A_ID* id) {
	while (id) {
		if (id->level < current_level) return(NULL);
		if ((strcmp(id->name, s) == 0))break;
		id = id->prev;
	}
	return(id);
}
void checkForwardReference() {
	A_ID* id;
	A_TYPE* t;
	id = current_id;
	while (id->level >= current_level) {
		t = id->type;
		if (id->kind == ID_NULL) syntax_error(31, id->name);
		// 현재의 level에서 이름의 종류가 정해지지 않았거나
		else if ((id->kind == ID_STRUCT || id->kind == ID_ENUM) && t->field == NULL) syntax_error(32, id->name);
		// 미완성 구조체 선언이 있는지 검사
		id = id->prev;
	}
}
// set default specifier
void setDefaultSpecifier(A_SPECIFIER* p) {
	p = malloc(sizeof(A_SPECIFIER));
	p->type = int_type;
	p->stor = S_AUTO;
	p->line = line_no;

}
// merge & update specifier
A_SPECIFIER* updateSpecifier(A_SPECIFIER* p, A_TYPE* t, S_KIND s) {
	if (t)
		if (p->type)
			if (p->type == t)
				;
			else
				syntax_error(24,"error");
		else
			p->type = t;
	if (s) {
		if (p->stor)
			if (s == p->stor);
			else
				syntax_error(24,"error");
		else
			p->stor = s;
	}
	return (p);
}
A_ID* linkDeclaratorList(A_ID* id1, A_ID* id2) {
	id1->link = id2;
	return (id1);
}
// check if the identifier is already declared in primary expression
A_ID* getIdentifierDeclared(char* s) {
	A_ID* id;
	id = searchIdentifier(s, current_id);
	if (id == NIL)
		syntax_error(13, s);
	return(id);
}
// get type of struct identifier
A_TYPE* getTypeOfStructOrEnumRefIdentifier(T_KIND k, char* s, ID_KIND kk) {
	A_TYPE* t;
	A_ID* id;
	id = searchIdentifier(s, current_id);
	if (id) {
		if (id->kind == kk && id->type->kind == k) {
			return(id->type);
		}
	}
	// make a new struct (or enum) identifier
	t = makeType(k);
	id = makeIdentifier(s);
	id->kind = kk;
	id->type = t;
	return(t);
}
// set declarator init (expression tree)
A_ID* setDeclaratorInit(A_ID* id, A_NODE* n) {
	id->init = n;
	return(id);
}
// set declarator kind
A_ID* setDeclaratorKind(A_ID* id, ID_KIND k) {
	if (searchIdentifier(id->name, id->prev)) syntax_error(12, id->name);
	else {
		id->kind = k;
		return (id);
	}
}
// set declarator type
A_ID* setDeclaratorType(A_ID* id, A_TYPE* t) {
	id->type = t;
	return(id);
}
// set declarator type (or element type)
A_ID* setDeclaratorElementType(A_ID* id, A_TYPE* t) {
	A_TYPE* tt;
	tt = id->type;
	if (id->type == NIL)
		id->type = t;
	else {
		tt = id->type;
		while (tt->element_type) {
			tt = tt->element_type;
		}
		tt->element_type = t;
	}
	return (id);
}
// set declarator element type and kind
A_ID* setDeclaratorTypeAndKind(A_ID* id, A_TYPE* t, ID_KIND k) {
	id = setDeclaratorElementType(id, t);
	id = setDeclaratorKind(id, k);
	return(id);
}
// check function declarator and return type
A_ID* setFunctionDeclaratorSpecifier(A_ID* id, A_SPECIFIER* p) {
	A_ID* a;
	if (p->stor) syntax_error(25, "error");
	setDefaultSpecifier(p);
	if (id->type->kind != T_FUNC) {
		syntax_error(25, "error");
		return(id); //함수가 아닌 경우 배제
	}
	else {
		id = setDeclaratorElementType(id, p->type);
		id->kind = ID_FUNC;
	}
	a = searchIdentifierAtCurrentLevel(id->name, id->prev);	//선언되어있었는지 검사
	if (a) {
		if (a->kind != ID_FUNC || a->type->expr) syntax_error(12, id->name); //함수선언이 아니었을 경우 에러
		else {
			if (isNotSameFormalParameters(a->type->field, id->type->field)) syntax_error(22, id->name);
			// 선언부 파라미터와 정의부 파라미터가 다를 경우 에러
			else if (isNotSameType(a->type->element_type, id->type->element_type)) syntax_error(26, a->name);
			// 선언부 리턴타입과 정의부 리턴타입이 다를 경우 에러
		}
	}
	a = id->type->field;
	while (a) {
		if (strlen(a->name)!=0) current_id = a; // 현재 id를 파라미터 심볼테이블로 이동시킨다.
		else if (a->type) syntax_error(23, "error");
		a = a->link;
	}
	return (id);

}
A_ID* setFunctionDeclaratorBody(A_ID* id, A_NODE* n) {
	id->type->expr = n;
	return(id);
}
// decide the type and kind of the declarator_list based on the storage class
A_ID* setDeclaratorListSpecifier(A_ID* id, A_SPECIFIER* p) {
	A_ID* a;
	setDefaultSpecifier(p);
	a = id;
	while (a) {
		if (strlen(a->name)!=0 &&
			searchIdentifierAtCurrentLevel(id->name, id->prev)) syntax_error(12,a->name);
		//중복선언 검사
		a = setDeclaratorElementType(a, p->type);
		if (p->stor == S_TYPEDEF) a->kind = ID_TYPE;
		else if (a->type->kind == T_FUNC) a->kind = ID_FUNC;
		else a->kind = ID_VAR;
		//각 타입에 맞게 명칭 타입 지정
		a->specifier = p->stor;
		if (a->specifier == S_NULL)
			a->specifier = S_AUTO;
		a = a->link;
	}
	return(id);
}
// set declarator_list type and kind
A_ID* setParameterDeclaratorSpecifier(A_ID* id, A_SPECIFIER* p) {
	if (searchIdentifierAtCurrentLevel(id->name, id->prev)) syntax_error(12, id->name);
	//중복체크
	if (p->stor || p->type == void_type) syntax_error(14, "ERROR");
	// 파라미터는 보이드여서도 안 되고 저장장소 지정자가 등장해서도 안 된다.
	setDefaultSpecifier(p);
	id = setDeclaratorElementType(id, p->type);
	id->kind = ID_PARM;
	return(id);
}
A_ID* setStructDeclaratorListSpecifier(A_ID* id, A_TYPE* t) {
	A_ID* a;
	a = id;
	while (a) {
		if (searchIdentifierAtCurrentLevel(a->name, a->prev)) syntax_error(12, a->name);
		//중복선언되어 있었을 경우 에러
		a = setDeclaratorElementType(a, t);
		a->kind = ID_FIELD;
		a = a->link;
		// 구조체 멤버는 하나의 declaratorSpecifier로 타입선언될 수 있으므로 반복한다.
	}
	return(id);
}
// set type name specifier
A_TYPE* setTypeNameSpecifier(A_TYPE* t, A_SPECIFIER* p) {
	// check storage class in type name
	if (p->stor)
		syntax_error(20,"error");
	setDefaultSpecifier(p);
	t = setTypeElementType(t, p->type);
	return(t);
}
// set type element type
A_TYPE* setTypeElementType(A_TYPE* t, A_TYPE* s) {
	A_TYPE* q;
	if (t == NIL) return s;
	q = t;
	while (q->element_type) q = q->element_type;
	q->element_type = s;

	return(t);
}
// set type field
A_TYPE* setTypeField(A_TYPE* t, A_ID* n) {
	t->field = n;
	return(t);
}
// set type initial value (expression tree)
A_TYPE* setTypeExpr(A_TYPE* t, A_NODE* n) {
	t->expr = n;
	return(t);
}
// set type of struct iIdentifier
A_TYPE* setTypeStructOrEnumIdentifier(T_KIND k, char* s, ID_KIND kk) {
	A_TYPE* t;
	A_ID* id, * a;
	// check redeclaration or forward declaration
	a = searchIdentifierAtCurrentLevel(s, current_id);
	if (a)
		if (a->kind == kk && a->type->kind == k)
			if (a->type->field)
				syntax_error(12, s);
			else
				return(a->type);
		else
			syntax_error(12, s);
	// make a new struct (or enum) identifier
	id = makeIdentifier(s);
	t = makeType(k);
	id->type = t;
	id->kind = kk;
	return(t);
}
// set type and kinf of identifier
A_TYPE* setTypeAndKindOfDeclarator(A_TYPE* t, ID_KIND k, A_ID* id) {
	if (searchIdentifierAtCurrentLevel(id->name, id->prev))
		syntax_error(12, id->name);
	id->type = t;
	id->kind = k;
	return(t);
}
// check function parameters with protype
BOOLEAN isNotSameFormalParameters(A_ID* a, A_ID* b) {
	if (a == NIL) // no parameters in prototype
		return(FALSE);
	while (a) {
		if (b == NIL || isNotSameType(a->type, b->type))
			return(TRUE);
		a = a->link;
		b = b->link;
	}
	if (b)
		return(TRUE);
	else
		return(FALSE);
}
BOOLEAN isNotSameType(A_TYPE* t1, A_TYPE* t2) {
	if (isPointerOrArrayType(t1) || isPointerOrArrayType(t2))
		return (isNotSameType(t1->element_type, t2->element_type));
	else
		return (t1 != t2);
}
void initialize() {
	// primitive data types 설정
	int_type = setTypeAndKindOfDeclarator(
		makeType(T_ENUM), ID_TYPE, makeIdentifier("int"));
	float_type = setTypeAndKindOfDeclarator(
		makeType(T_ENUM), ID_TYPE, makeIdentifier("float"));
	char_type = setTypeAndKindOfDeclarator(
		makeType(T_ENUM), ID_TYPE, makeIdentifier("char"));
	void_type = setTypeAndKindOfDeclarator(
		makeType(T_VOID), ID_TYPE, makeIdentifier("void"));
	string_type = setTypeElementType(makeType(T_POINTER), char_type);
	int_type->size = 4; int_type->check = TRUE;
	float_type->size = 4; float_type->check = TRUE;
	char_type->size = 1; char_type->check = TRUE;
	void_type->size = 0; void_type->check = TRUE;
	string_type->size = 4; string_type->check = TRUE;
	// printf(char *, ...) library function
	setDeclaratorTypeAndKind(
		makeIdentifier("printf"),
		setTypeField(
			setTypeElementType(makeType(T_FUNC), void_type),
			linkDeclaratorList(
				setDeclaratorTypeAndKind(makeDummyIdentifier(), string_type, ID_PARM),
				setDeclaratorKind(makeDummyIdentifier(), ID_PARM))),
		ID_FUNC);
	// scanf(char *, ...) library function
	setDeclaratorTypeAndKind(
		makeIdentifier("scanf"),
		setTypeField(
			setTypeElementType(makeType(T_FUNC), void_type),
			linkDeclaratorList(
				setDeclaratorTypeAndKind(makeDummyIdentifier(), string_type, ID_PARM),
				setDeclaratorKind(makeDummyIdentifier(), ID_PARM))),
		ID_FUNC);
	// malloc(int) library function
	setDeclaratorTypeAndKind(
		makeIdentifier("malloc"),
		setTypeField(
			setTypeElementType(makeType(T_FUNC), string_type),
			setDeclaratorTypeAndKind(makeDummyIdentifier(), int_type, ID_PARM)),
		ID_FUNC);
}
void syntax_error(int i, char* s) {
	syntax_err++;
	printf("line %d: syntax error: ", line_no);
	switch (i) {
	case 11: printf("illegal referencing struct or union identifier %s", s);
		break;
	case 12: printf("redeclaration of identifier %s", s); break;
	case 13: printf("undefined identifier %s", s); break;
	case 14: printf("illegal type specifier in formal parameter"); break;
	case 20: printf("illegal storage class in type specifiers"); break;
	case 21: printf("illegal function declarator"); break;
	case 22: printf("conflicting parm type in prototype function %s", s);
		break;
	case 23: printf("empty parameter name"); break;
	case 24: printf("illegal declaration specifiers"); break;
	case 25: printf("illegal function specifiers"); break;
	case 26: printf("illegal or conflicting return type in function %s", s);
		break;
	case 31: printf("undefined type for identifier %s", s); break;
	case 32: printf("incomplete forward reference for identifier %s", s);
		break;
	default: printf("unknown"); break;
	}
	if (strlen(yytext) == 0)
		printf(" at end\n");
	else
		printf(" near %s\n", yytext);
}