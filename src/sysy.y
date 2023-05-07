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

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  std::string *str_op;
  BaseAST *ast_val;
  ExpAST *exp_val;
  BlockAST *blk_list;
  VarDeclAST *var_list;
  ConstDeclAST *con_list;
  LValAST *lval;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN LESS_EQ GREAT_EQ EQUAL NOT_EQUAL AND OR CONST IF ELSE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Decl ConstDecl BType ConstDef VarDecl VarDef BlockItem MatchedStmt OpenStmt OtherStmt
%type <lval> LVal
%type <var_list> VarDefList
%type <con_list> ConstDefList
%type <blk_list> BlockItemList
%type <exp_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstInitVal ConstExp InitVal
%type <int_val> Number
%type <str_op>  UnaryOp

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    $$ = $2;
  }
  ;

BlockItemList
  : BlockItem BlockItemList {
    auto ast = $2;
    ast->block_items.emplace_back(unique_ptr<BaseAST>($1));
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
    ast->decl = unique_ptr<BaseAST>($1);
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
    ast->const_decl = unique_ptr<BaseAST>($1);
    ast->var_decl = nullptr;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->const_decl = nullptr;
    ast->var_decl = unique_ptr<BaseAST>($1);
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
  : ConstDef  ',' ConstDefList {
    auto ast = $3;
    ast->const_defs.emplace_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstDef {
    auto ast = new ConstDeclAST();
    ast->const_defs.emplace_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
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
  : VarDef ',' VarDefList {
    auto ast = $3;
    ast->var_defs.emplace_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | VarDef {
    auto ast = new VarDeclAST();
    ast->var_defs.emplace_back(unique_ptr<BaseAST>($1));
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
  : MatchedStmt {
    $$ = $1;
  } 
  | OpenStmt {
    $$ = $1;
  }
  ;

// 若有IF则一定有匹配的ELSE
MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  } 
  | OtherStmt {
    $$ = $1;
  }
  ;

// IF后可以不接ELSE（一定在最后）
OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = nullptr;
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

OtherStmt
  : LVal '=' Exp ';' {
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

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
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
