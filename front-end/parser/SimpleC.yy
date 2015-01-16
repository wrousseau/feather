%{
#include "SyntaxTree.h"
int yylex (void);
void yyerror (char const * szError) { std::cerr << szError << std::endl; }

Program::ParseContext parseContext;

enum TokenType
   {  TEOF, TType=258, TIf, TElse, TReturn, TDo, TWhile, TOpenParen, TCloseParen, TOpenBrace,
      TCloseBrace, TOpenBracket, TCloseBracket, TComma, TSemicolon, TStar, TAnd, TColon, TPlus,
      TMinus, TDivide, TLessOrEqual, TGreaterOrEqual, TEqual, TLess, TGreater, TAssign,
      TExpression, TIdentifier, TFunctionExpression
   };

%}

%union {
  std::string* identifier;
  VirtualType* type;
  VirtualExpression* expression;
  VirtualInstruction* instruction;
  Function* function;
}

%token <identifier> IDENT 285
%token <type> TYPE 258
%token <type> STAR 272
%token <expression> EXPR 284
%token <expression> FUN_EXPR 286

%token IF 259
%token ELSE 260
%token RETURN 261
%token DO 262
%token WHILE 263
%token OPENPAREN 264
%token CLOSEPAREN 265
%token OPENBRACE 266
%token CLOSEBRACE 267
%token OPENBRACKET 268
%token CLOSEBRACKET 269
%token COMMA 270
%token SEMICOLON 271
%token AND 273
%token COLON 274
%token PLUS 275
%token MINUS 276
%token DIVIDE 277
%token LESSOREQUAL 278
%token GREATEROREQUAL 279
%token EQUAL 280
%token LESS 281
%token GREATER 282
%token ASSIGN 283

%type  <expression> primary_expression
%type  <expression> expression_list
%type  <expression> postfix_expression
%type  <expression> pm_expression
%type  <expression> multiplicative_expression
%type  <expression> additive_expression
%type  <expression> relational_expression
%type  <expression> expression
%type  <instruction> statement
%type  <instruction> statement_seq
%type  <instruction> end_if_statement
%type  <function> parameter_declaration_list
%type  <function> function_call
%type  <identifier> direct_declarator
%type  <type> empty_function_type
%type  <identifier> declarator

%right '='
%left '-' '+'
%left '*' '/'

%% /* The grammar follows.  */

translation_unit:   /* empty */ {}
                  | declaration_seq {}
;

declaration_seq :   declaration {}
                  | declaration declaration_seq {}
;

declaration :   TYPE declarator SEMICOLON { parseContext.addGlobalDeclarationStatement($1, *$2); delete $2; $2 = NULL; }
              | TYPE declarator OPENBRACE CLOSEBRACE { parseContext.setEmptyFunction(*$2); delete $2; $2 = NULL; }
              | TYPE declarator OPENBRACE { parseContext.pushFunction(*$2); delete $2; $2 = NULL; } statement_seq CLOSEBRACE { parseContext.popFunction(); }
;

declarator :   direct_declarator { $$ = $1; }
             | STAR { $<type>0 = new PointerType($<type>0); $1 = $<type>0; } declarator { $$ = $3; }
;

direct_declarator :   IDENT { $$ = $1; }
                    | direct_declarator function_call OPENPAREN CLOSEPAREN { $$ = $1; }
                    | direct_declarator function_call OPENPAREN parameter_declaration_list CLOSEPAREN { $$ = $1; }
                    | direct_declarator OPENBRACKET expression CLOSEBRACKET { $$ = $1; }
                    | OPENPAREN empty_function_type declarator CLOSEPAREN { $$ = $3; }
;

empty_function_type :   /* empty */
                      { $$ = $<type>-1; /* function pointer */ }

function_call :   /* empty */
                { $$ = &parseContext.addFunction(*$<identifier>0); }

parameter_declaration_list :   TYPE declarator { $$ = $<function>-1; $$->signature().addNewParameter(*$2, $1); delete $2; $2 = NULL; }
                             | parameter_declaration_list COMMA TYPE declarator { $$ = $1; $$->signature().addNewParameter(*$4, $3); delete $4; $4 = NULL;}
;

statement_seq :   statement { $$ = $1; }
                | statement statement_seq { $$ = $2; }
;

statement :   SEMICOLON { $$ = &parseContext.getLastInstruction(); }
            | expression SEMICOLON { $$ = &(new ExpressionInstruction())->setExpression($1); parseContext.addNewInstruction($$); }
            | OPENBRACE CLOSEBRACE { $$ = &parseContext.getLastInstruction(); }
            | OPENBRACE { parseContext.pushBlock(); } statement_seq CLOSEBRACE { $$ = &parseContext.closeBlock(*$3); }
            | IF OPENPAREN expression CLOSEPAREN { parseContext.pushIfThen($3); } end_if_statement { $$ = $6; }
            | WHILE OPENPAREN expression CLOSEPAREN { parseContext.pushWhileLoop($3); } statement { $$ = &parseContext.closeWhileLoop(*$6); }
            | DO { parseContext.pushDoLoop(); } statement WHILE OPENPAREN expression CLOSEPAREN SEMICOLON { $$ = &parseContext.closeDoLoop(*$3, $6); }
            | TYPE declarator SEMICOLON { parseContext.addDeclarationStatement($1, *$2); $$ = &parseContext.getLastInstruction(); delete $2; $2 = NULL; }
            | RETURN expression SEMICOLON { $$ = &(new ReturnInstruction())->setResult($2); parseContext.addNewReturnInstruction((ReturnInstruction*) $$); }
;

end_if_statement :   statement ELSE { parseContext.setToIfElse(*$1); } statement { $$ = &parseContext.closeIfElse(*$1, *$4); }
                   | statement { $$ = &parseContext.closeIfThen(*$1); }

expression :   pm_expression ASSIGN expression { $$ = &(new AssignExpression())->setLValue($1).setRValue($3); }
             | relational_expression { $$ = $1; }
;

relational_expression :   additive_expression { $$ = $1; }
                        | relational_expression LESS additive_expression { $$ = &(new ComparisonExpression())->setOperator(ComparisonExpression::OCompareLess).setFst($1).setSnd($3); }
                        | relational_expression LESSOREQUAL additive_expression { $$ = &(new ComparisonExpression())->setOperator(ComparisonExpression::OCompareLessOrEqual).setFst($1).setSnd($3); }
                        | relational_expression EQUAL additive_expression { $$ = &(new ComparisonExpression())->setOperator(ComparisonExpression::OCompareEqual).setFst($1).setSnd($3); }
                        | relational_expression GREATEROREQUAL additive_expression { $$ = &(new ComparisonExpression())->setOperator(ComparisonExpression::OCompareGreaterOrEqual).setFst($1).setSnd($3); }
                        | relational_expression GREATER additive_expression { $$ = &(new ComparisonExpression())->setOperator(ComparisonExpression::OCompareGreater).setFst($1).setSnd($3); }
;

additive_expression :   multiplicative_expression { $$ = $1; }
                      | additive_expression PLUS multiplicative_expression { $$ = &(new BinaryOperatorExpression())->setOperator(BinaryOperatorExpression::OPlus).setFst($1).setSnd($3); }
                      | additive_expression MINUS multiplicative_expression { $$ = &(new BinaryOperatorExpression())->setOperator(BinaryOperatorExpression::OMinus).setFst($1).setSnd($3); }
;

multiplicative_expression :   pm_expression { $$ = $1; }
                            | multiplicative_expression STAR pm_expression { $$ = &(new BinaryOperatorExpression())->setOperator(BinaryOperatorExpression::OTimes).setFst($1).setSnd($3); }
                            | multiplicative_expression DIVIDE pm_expression { $$ = &(new BinaryOperatorExpression())->setOperator(BinaryOperatorExpression::ODivide).setFst($1).setSnd($3); }
;

pm_expression :   postfix_expression { $$ = $1; }
                | STAR pm_expression { $$ = &(new DereferenceExpression())->setSubExpression($2); }
                | AND pm_expression { $$ = &(new ReferenceExpression())->setSubExpression($2); }
                | PLUS pm_expression { $$ = $2; }
                | MINUS pm_expression { $$ = &(new UnaryOperatorExpression())->setOperator(UnaryOperatorExpression::OMinus).setSubExpression($2); }
;

postfix_expression :   primary_expression { $$ = $1; }
                     | FUN_EXPR OPENPAREN expression_list CLOSEPAREN { $$ = $1; }
                     | FUN_EXPR OPENPAREN CLOSEPAREN { $$ = $1; }
;

primary_expression :   EXPR { $$ = $1; }
                     | OPENPAREN expression CLOSEPAREN { $$ = $2; }
;

expression_list :   expression_list COMMA expression { assert(dynamic_cast<const FunctionCallExpression*>($1)); $$ = $1; ((FunctionCallExpression*) $$)->addArgument($3); }
                  | expression { assert(dynamic_cast<const FunctionCallExpression*>($<expression>-1)); $$ = $<expression>-1; ((FunctionCallExpression*) $$)->addArgument($1); }
;

/* End of grammar.  */

%%


