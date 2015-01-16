
%{
#include <FlexLexer.h>
#include <iostream>
#include "SyntaxTree.h"
using namespace std;
int mylineno = 0;

typedef union YYSTYPE {
  std::string* identifier;
  VirtualType* type;
  VirtualExpression* expression;
  VirtualInstruction* instruction;
  Function* function;
} YYSTYPE;

extern YYSTYPE yylval;
extern Program::ParseContext parseContext;

enum TokenType
   {  TEOF, TType=258, TIf, TElse, TReturn, TDo, TWhile, TOpenParen, TCloseParen, TOpenBrace,
      TCloseBrace, TOpenBracket, TCloseBracket, TComma, TSemicolon, TStar, TAnd, TColon, TPlus,
      TMinus, TDivide, TLessOrEqual, TGreaterOrEqual, TEqual, TLess, TGreater, TAssign,
      TExpression, TIdentifier, TFunctionExpression
   };

inline int getTokenIdentifier(const char* szText)
{  int uLocal = 0;
   Scope sScope;
   Scope::FindResult result = parseContext.scope().find(std::string(szText), uLocal, sScope);
   if (result == Scope::FRLocal) {
      yylval.expression = new LocalVariableExpression(std::string(szText), uLocal, sScope);
      return TExpression;
   }
   else {
      if (parseContext.m_function && (*parseContext.m_function).findParameters(std::string(szText), uLocal)) {
         yylval.expression = new ParameterExpression(std::string(szText), uLocal);
         return TExpression;
      };
      if (result == Scope::FRGlobal) {
         yylval.expression = new GlobalVariableExpression(std::string(szText), uLocal);
         return TExpression;
      };
   };
   if (parseContext.functions().find(Function(std::string(szText))) != parseContext.functions().end()) {
      yylval.expression = new FunctionCallExpression(const_cast<Function&>(*parseContext.functions().find(Function(std::string(szText)))));
      return TFunctionExpression;
   };
   yylval.identifier = new std::string(szText); 
   return TIdentifier;
};

%}

string  \"[^\n"]+\"

ws      [ \t]+

alpha   [A-Za-z]
dig     [0-9]
name    (alpha)({alpha}|{dig}|_)*
num1    {dig}+\.?([eE][-+]?{dig}+)?
num2    {dig}*\.{dig}+([eE][-+]?{dig}+)?
number  {num1}|{num2}

%%

{ws}    /* skip blanks and tabs */

"/*"    { int c;

          while((c = yyinput()) != 0)
            {
            if(c == '\n')
                ++mylineno;

            else if(c == '*')
                {
                if((c = yyinput()) == '/')
                    break;
                else
                    unput(c);
                }
            }
        }

void      std::cout << "type " << yytext << '\n'; yylval.type = NULL; return TType;
int       std::cout << "type " << yytext << '\n'; yylval.type = new BaseType(BaseType::VInt); return TType;
char      std::cout << "type " << yytext << '\n'; yylval.type = new BaseType(BaseType::VChar); return TType; 
if        std::cout << "keyword " << yytext << '\n'; return TIf;
else      std::cout << "keyword " << yytext << '\n'; return TElse;
return    std::cout << "keyword " << yytext << '\n'; return TReturn;
do        std::cout << "keyword " << yytext << '\n'; return TDo;
while     std::cout << "keyword " << yytext << '\n'; return TWhile;
"("       std::cout << "open_paren" << '\n'; return TOpenParen;
")"       std::cout << "close_paren" << '\n'; return TCloseParen;
"{"       std::cout << "open_brace" << '\n'; return TOpenBrace;
"}"       std::cout << "close_brace" << '\n'; return TCloseBrace;
"["       std::cout << "open_bracket" << '\n'; return TOpenBracket;
"]"       std::cout << "close_bracket" << '\n'; return TCloseBracket;
","       std::cout << "comma" << '\n'; return TComma;
";"       std::cout << "semicolon" << '\n'; return TSemicolon;
"*"       std::cout << "star" << '\n'; return TStar;
":"       std::cout << "colon" << '\n'; return TColon;
"+"       std::cout << "plus" << '\n'; return TPlus;
"-"       std::cout << "minus" << '\n'; return TMinus;
"/"       std::cout << "divide" << '\n'; return TDivide;
"<="      std::cout << "less_or_equal" << '\n'; return TLessOrEqual;
">="      std::cout << "greater_or_equal" << '\n'; return TGreaterOrEqual;
"=="      std::cout << "equal" << '\n'; return TEqual;
"<"       std::cout << "less" << '\n'; return TLess;
">"       std::cout << "greater" << '\n'; return TGreater;
"="       std::cout << "assign" << '\n'; return TAssign;

{dig}+\.?([eE][-+]?{dig}+)?|{dig}*\.{dig}+([eE][-+]?{dig}+)?  std::cout << "number " << yytext << '\n'; yylval.expression = new ConstantExpression(atoi(yytext)); return TExpression;

\n        mylineno++;

([A-Za-z])([A-Za-z]|[0-9]|_)*    std::cout << "name " << yytext << '\n'; return getTokenIdentifier(yytext);

\"[^\n"]+\"  std::cout << "string " << yytext << '\n'; yylval.expression = new StringExpression(std::string(yytext)); return TExpression;

%%
/*
{number}  std::cout << "number " << yytext << '\n'; yylval.expression = new ConstantExpression(atoi(yytext)); return TExpression;

\n        mylineno++;

{name}    std::cout << "name " << yytext << '\n'; return getTokenIdentifier(yytext);

{string}  std::cout << "string " << yytext << '\n'; yylval.expression = new StringExpression(std::string(yytext)); return TExpression;

int main( int argc, char** argv ) {
   // FlexLexer* lexer = new yyFlexLexer;
   while(yylex() != 0) // while(lexer->yylex() != 0)
       ;
   return 0;
}
*/
