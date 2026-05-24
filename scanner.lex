%{

/* Declarations section */
#include "output.hpp"
#include <iostream>
#include <ostream>
#include "parser.tab.h"
#include <string>

%}

%option yylineno
%option noyywrap
digit   		    ([0-9])
letter  		    ([a-zA-Z])
digit_letter        ([a-zA-Z0-9])
hex_digit           ([0-9A-F])
whitespace		    ([\r\t\n ])

escape_sequence     ([\"nrt0\\])
valid_escape        (\\({escape_sequence}|x0[9aAdD]|x[2-6][0-9a-fA-F]|x7[0-9a-eA-E]))
string_char         ([^\\\"\n\r]|{valid_escape})

string              (\"({string_char})*\")
invalid_escape      (\"({string_char})*(\\[^\"nrt0x\\]|\\x[^\n\r\"]{0,2}))
unclosed_string     (\"({string_char})*)
%%

\\n         			                            {/*skip*/}
void 	                                            {return VOID;}
int                                                 {return INT;}
byte                                                {return BYTE;}
bool                                                {return BOOL;}
and                                                 {return AND;}
or                                                  {return OR;}
not                                                 {return NOT;}
true                                                {return TRUE;}
false 	                                            {return FALSE;}
return                                              {return RETURN;}
if                                                  {return IF;}
else                                                {return ELSE;}
while                                               {return WHILE;}
break                                               {return BREAK;}
continue                                            {return CONTINUE;}
;                                                   {return SC;}
, 	                                                {return COMMA;}
\(                                                  {return LPAREN;}
\)                                                  {return RPAREN;}
\{                                                  {return LBRACE;}
\}                                                  {return RBRACE;}
\[                                                  {return LBRACK;}
\]                                                  {return RBRACK;}
=                                                   {return ASSIGN;}
==               	                                {return RELOP_EQ;}
!=               	                                {return RELOP_NE;}
(<)                	                                {return RELOP_LT;}
(>)               	                                {return RELOP_GT;}
(<=)               	                                {return RELOP_LE;}
(>=)               	                                {return RELOP_GE;}
[*]                                                 {return BINOP_MUL;}
[\/]                                                {return BINOP_DIV;}
[+]                                                 {return BINOP_ADD;}
[-]                                                 {return BINOP_SUB;}

[a-zA-Z][a-zA-Z0-9]*                                { yylval = std::make_shared<ast::ID>(yytext); return ID; }
0|[1-9][0-9]*                                       { yylval = std::make_shared<ast::Num>(yytext); return NUM; }
0b|[1-9][0-9]*b                                     { yylval = std::make_shared<ast::NumB>(yytext); return NUM_B; }
\"([^\n\r\"\\]|\\[rnt"\\])+\"                       { yylval = std::make_shared<ast::String>(yytext); return STRING; }


{whitespace}                                        {/*ignore*/};
\/\/[^\r\n]*                                        {/*return COMMENT; ignore*/};
.                                                   {output::errorLex(yylineno);}		                                           
%%

