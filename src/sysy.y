%code requires {
  #include <memory>
  #include <string>
  #include "../include/ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include "../include/ast.hpp"

using namespace std;

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(unique_ptr<BaseAST> &ast, const char *s);


%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  ExpAST *exp_val;
  BlockAST *blk_list;
  VarDeclAST *var_list;
  ConstDeclAST *con_list;
  DefAST *def_ast;
  CompUnitAST *com_unit;
  FuncDefAST *func_defs;
  UnaryExpAST *unary_exps;
  FuncFParamAST *func_f_param;
  BTypeAST *func_type;
  LValAST *lval;
}

// 消除ELSE冲突
%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN LESS_EQ GREAT_EQ EQUAL NOT_EQUAL AND OR CONST IF WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt BlockItem
%type <def_ast> Decl ConstDecl VarDecl ConstDef VarDef 
%type <exp_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp ConstInitVal InitVal
%type <var_list> VarDefList
%type <con_list> ConstDefList
%type <blk_list> BlockItemList
%type <com_unit> DeclOrFuncDefList
%type <func_defs> FuncFParams
%type <unary_exps> FuncRParams
%type <func_type> BType
%type <func_f_param> FuncFParam
%type <int_val> Number
%type <str_val>  UnaryOp
%type <lval> LVal

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
CompUnit
  : DeclOrFuncDefList {
    ast = unique_ptr<CompUnitAST>($1);
  }
  ;

DeclOrFuncDefList
  : DeclOrFuncDefList FuncDef{
    auto ast = $1;
    ast->func_defs.emplace_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | DeclOrFuncDefList Decl{
    auto ast = $1;
    ast->decls.emplace_back(unique_ptr<DefAST>($2));
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitAST();
    ast->func_defs.emplace_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitAST();
    ast->decls.emplace_back(unique_ptr<DefAST>($1));
    $$ = ast;
  }
  ;

FuncDef
  : BType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BTypeAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BType IDENT '(' FuncFParams ')' Block {
    auto ast = $4;
    ast->func_type = unique_ptr<BTypeAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParams ',' FuncFParam {
    auto ast = $1;
    ast->func_f_params.emplace_back(unique_ptr<FuncFParamAST>($3));
    $$ = ast;
  }
  | FuncFParam {
    auto ast = new FuncDefAST();
    ast->func_f_params.emplace_back(unique_ptr<FuncFParamAST>($1));
    $$ = ast;
  }
  ;

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    ast->tag = BTypeAST::INT;
    $$ = ast;
  }
  | VOID {
    auto ast = new BTypeAST();
    ast->tag = BTypeAST::VOID;
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    $$ = $2;
  }
  ;

BlockItemList
  : BlockItemList BlockItem {
    auto ast = $1;
    ast->block_items.emplace_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | {
    auto ast = new BlockAST();
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<DefAST>($1);
    ast->stmt = nullptr;
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->decl = nullptr;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<DefAST>($1);
    ast->var_decl = nullptr;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->const_decl = nullptr;
    ast->var_decl = unique_ptr<DefAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto ast = $3;
    ast->btype = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDefList ',' ConstDef {
    auto ast = $1;
    ast->const_defs.emplace_back(unique_ptr<DefAST>($3));
    $$ = ast;
  }
  | ConstDef {
    auto ast = new ConstDeclAST();
    ast->const_defs.emplace_back(unique_ptr<DefAST>($1));
    $$ = ast;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<ExpAST>($3);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
    auto ast = $2;
    ast->btype = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDefList
  : VarDefList ',' VarDef {
    auto ast = $1;
    ast->var_defs.emplace_back(unique_ptr<DefAST>($3));
    $$ = ast;
  }
  | VarDef {
    auto ast = new VarDeclAST();
    ast->var_defs.emplace_back(unique_ptr<DefAST>($1));
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = nullptr;
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<ExpAST>($3);
    $$ = ast;
  }
  ;

Stmt
  : IF '(' Exp ')' Stmt %prec LOWER_THEN_ELSE {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = nullptr;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::ASSIGN;
    ast->lval = unique_ptr<LValAST>($1);
    ast->exp = unique_ptr<ExpAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::EXP;
    ast->exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::EXP;
    ast->exp = nullptr;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->tag = StmtAST::BLOCK;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::RETURN;
    ast->lval = nullptr;
    ast->exp = unique_ptr<ExpAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::RETURN;
    ast->lval = nullptr;
    ast->exp = nullptr;
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::WHILE;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::CONTINUE;
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')'{
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<ExpAST>($2); //表示该PrimaryExp为 (Exp)
    ast->lval = nullptr;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->exp = nullptr;
    ast->lval = unique_ptr<LValAST>($1); //表示该PrimaryExp为 LVal
    $$ = ast;
  }
  | Number { 
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    ast->exp = nullptr;                //表示该PrimaryExp为Number
    ast->lval = nullptr;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp{ 
      auto ast = new UnaryExpAST();
      ast->primary_exp = unique_ptr<ExpAST>($1);
      ast->unary_exp = nullptr;          //表示该 UnaryExp为 PrimaryExp
      $$ = ast;
  }
  | UnaryOp UnaryExp{
      auto ast = new UnaryExpAST();
      ast->unary_op = *unique_ptr<string>($1);
      ast->unary_exp = unique_ptr<ExpAST>($2); //表示该UnaryExp为 OP+Exp
      ast->primary_exp = nullptr;  
      $$ = ast;
  }
  | IDENT '(' ')' {
      auto ast = new UnaryExpAST();
      ast->ident = *unique_ptr<string>($1);
      ast->unary_exp = nullptr; 
      ast->primary_exp = nullptr;  
      $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
      auto ast = $3;
      ast->ident = *unique_ptr<string>($1);
      ast->unary_exp = nullptr; 
      ast->primary_exp = nullptr; 
      $$ = ast;
  }
  ;

FuncRParams
  : FuncRParams ',' Exp {
    auto ast = $1;
    ast->exps.emplace_back(unique_ptr<ExpAST>($3));
    $$ = ast;
  }
  | Exp {
    auto ast = new UnaryExpAST();
    ast->exps.emplace_back(unique_ptr<ExpAST>($1));
    $$ = ast;
  }
  ;

UnaryOp
  : '+'{ $$ = new string("+"); }
  | '-'{ $$ = new string("-"); }
  | '!'{ $$ = new string("!"); }
  ;

MulExp
  : UnaryExp{
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp{
    auto ast = new MulExpAST();
    ast->unary_exp = nullptr;
    ast->mul_exp_1 = unique_ptr<ExpAST>($1);
    ast->unary_exp_2 = unique_ptr<ExpAST>($3);
    ast->mul_op = "*";
    $$ = ast;
  }
  | MulExp '/' UnaryExp{
    auto ast = new MulExpAST();
    ast->unary_exp = nullptr;
    ast->mul_exp_1 = unique_ptr<ExpAST>($1);
    ast->unary_exp_2 = unique_ptr<ExpAST>($3);
    ast->mul_op = "/";
    $$ = ast;
  }
  | MulExp '%' UnaryExp{
    auto ast = new MulExpAST();
    ast->unary_exp = nullptr;
    ast->mul_exp_1 = unique_ptr<ExpAST>($1);
    ast->unary_exp_2 = unique_ptr<ExpAST>($3);
    ast->mul_op = "%";
    $$ = ast;
  }
  ;

AddExp 
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = nullptr;
    ast->add_exp_1 = unique_ptr<ExpAST>($1);
    ast->mul_exp_2 = unique_ptr<ExpAST>($3);
    ast->add_op = "+";
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = nullptr;
    ast->add_exp_1 = unique_ptr<ExpAST>($1);
    ast->mul_exp_2 = unique_ptr<ExpAST>($3);
    ast->add_op = "-";
    $$ = ast;
  }
  ;

RelExp
  : AddExp{
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp{
    auto ast = new RelExpAST();
    ast->add_exp = nullptr;
    ast->rel_exp_1 = unique_ptr<ExpAST>($1);
    ast->add_exp_2 = unique_ptr<ExpAST>($3);
    ast->rel_op = "<";
    $$ = ast;
  }
  | RelExp '>' AddExp{
    auto ast = new RelExpAST();
    ast->add_exp = nullptr;
    ast->rel_exp_1 = unique_ptr<ExpAST>($1);
    ast->add_exp_2 = unique_ptr<ExpAST>($3);
    ast->rel_op = ">";
    $$ = ast;
  }
  | RelExp LESS_EQ AddExp{
    auto ast = new RelExpAST();
    ast->add_exp = nullptr;
    ast->rel_exp_1 = unique_ptr<ExpAST>($1);
    ast->add_exp_2 = unique_ptr<ExpAST>($3);
    ast->rel_op = "<=";
    $$ = ast;
  }
  | RelExp GREAT_EQ AddExp{
    auto ast = new RelExpAST();
    ast->add_exp = nullptr;
    ast->rel_exp_1 = unique_ptr<ExpAST>($1);
    ast->add_exp_2 = unique_ptr<ExpAST>($3);
    ast->rel_op = ">=";
    $$ = ast;
  }
  ;

EqExp 
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | EqExp EQUAL RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = nullptr;
    ast->eq_exp_1 = unique_ptr<ExpAST>($1);
    ast->rel_exp_2 = unique_ptr<ExpAST>($3);
    ast->eq_op = "==";
    $$ = ast;
  }
  | EqExp NOT_EQUAL RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = nullptr;
    ast->eq_exp_1 = unique_ptr<ExpAST>($1);
    ast->rel_exp_2 = unique_ptr<ExpAST>($3);
    ast->eq_op = "!=";
    $$ = ast;
  }
  ;

LAndExp 
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = nullptr;
    ast->l_and_exp_1 = unique_ptr<ExpAST>($1);
    ast->eq_exp_2 = unique_ptr<ExpAST>($3);
    $$ = ast;
  }
  ;

LOrExp 
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->l_and_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->l_and_exp = nullptr;
    ast->l_or_exp_1 = unique_ptr<ExpAST>($1);
    ast->l_and_exp_2 = unique_ptr<ExpAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<ExpAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    extern int yylineno;    // defined and maintained in lex
    extern char *yytext;    // defined and maintained in lex
    int len=strlen(yytext);
    int i;
    char buf[512]={0};
    for (i=0;i<len;++i)
    {
        sprintf(buf,"%s%d ",buf,yytext[i]);
    }
    fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);

}
