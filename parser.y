%{

#include "nodes.hpp"
#include "output.hpp"

// bison declarations
extern int yylineno;
extern int yylex();

void yyerror(const char*);

// root of the AST, set by the parser and used by other parts of the compiler
std::shared_ptr<ast::Node> program;

using namespace std;

// TODO: Place any additional declarations here

#define DYNAMIC_CAST(var_type, value) std::dynamic_pointer_cast<ast::var_type>(value)
#define MAKE_SHARED_0_ARGS(out_type) std::make_shared<ast::out_type>()
#define MAKE_SHARED_1_ARGS(out_type, value1) std::make_shared<ast::out_type>(value1)
#define MAKE_SHARED_2_ARGS(out_type, type1, value1, value2) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1), value2)
#define MAKE_SHARED_3_ARGS(out_type, type1, value1, type2, value2, value3) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1), DYNAMIC_CAST(type2,value2), value3)

#define MAKE_SHARED_1_ARGS_LAST_DYNAMIC(out_type, type1, value1) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1))
#define MAKE_SHARED_2_ARGS_LAST_DYNAMIC(out_type, type1, value1, type2, value2) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1), DYNAMIC_CAST(type2,value2))
#define MAKE_SHARED_3_ARGS_LAST_DYNAMIC(out_type, type1, value1, type2, value2, type3, value3) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1), DYNAMIC_CAST(type2,value2), DYNAMIC_CAST(type3,value3))
#define MAKE_SHARED_4_ARGS_LAST_DYNAMIC(out_type, type1, value1, type2, value2, type3, value3, type4, value4) std::make_shared<ast::out_type>(DYNAMIC_CAST(type1,value1), DYNAMIC_CAST(type2,value2), DYNAMIC_CAST(type3,value3), DYNAMIC_CAST(type4,value4))
%}

// TODO: Define tokens here
%token VOID
%token INT
%token BYTE
%token BOOL
%token TRUE
%token FALSE
%token RETURN
%token WHILE
%token BREAK
%token CONTINUE
%token SC
%token COMMA
%token LPAREN
%token RPAREN
%token LBRACE
%token RBRACE
%token LBRACK
%token RBRACK
%token ID
%token NUM
%token NUM_B
%token STRING

// TODO: Define precedence and associativity here
%nonassoc IF
%nonassoc ELSE
%right ASSIGN
%left OR
%left AND
%left RELOP_EQ RELOP_NE RELOP_LE RELOP_GE RELOP_LT RELOP_GT
%left BINOP_ADD BINOP_SUB
%left BINOP_MUL BINOP_DIV
%right NOT
%left LPAREN RPAREN
%left LBRACE RBRACE
%%

// While reducing the start variable, set the root of the AST
Program:  Funcs { program = $1; }
;
Funcs:  {$$ = MAKE_SHARED_0_ARGS(Funcs);}
        |FuncDecl Funcs {$$ = $2; DYNAMIC_CAST(Funcs, $$)->push_front(DYNAMIC_CAST(FuncDecl,$1));}
;
FuncDecl: RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE 
                {$$ = MAKE_SHARED_4_ARGS_LAST_DYNAMIC(FuncDecl, ID, $2, Type, $1, Formals, $4, Statements, $7);}
;
RetType: Type {$$ = $1;}
        |VOID {$$ = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::VOID);}
;
Formals: {$$ = MAKE_SHARED_0_ARGS(Formals);}
        |FormalsList {$$ = $1;}
;
FormalsList:FormalDecl {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(Formals, Formal, $1);}
            |FormalDecl COMMA FormalsList {$$ = $3; DYNAMIC_CAST(Formals, $$)->push_front(DYNAMIC_CAST(Formal, $1));}
;
FormalDecl: Type ID {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(Formal, ID, $2, Type, $1);}
;
Statements:Statement {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(Statements, Statement, $1);}
            |Statements Statement {$$ = $1; DYNAMIC_CAST(Statements, $$)->push_back(DYNAMIC_CAST(Statement, $2));}
;
Statement: LBRACE Statements RBRACE {DYNAMIC_CAST(Statements, $2)->openExtraBraces = true;$$ = $2;}
            |Type ID SC {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(VarDecl, ID, $2, Type, $1);}
            |Type ID ASSIGN Exp SC {$$ = MAKE_SHARED_3_ARGS_LAST_DYNAMIC(VarDecl, ID, $2, Type, $1, Exp, $4);}
            |ID ASSIGN Exp SC {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(Assign, ID, $1, Exp, $3);}
            |ID LBRACK Exp RBRACK ASSIGN Exp SC {$$ = MAKE_SHARED_3_ARGS_LAST_DYNAMIC(ArrayAssign, ID, $1, Exp, $6 , Exp, $3);}
            |Type ID LBRACK Exp RBRACK SC {$$ = MAKE_SHARED_2_ARGS(VarDecl, ID, $2, std::make_shared<ast::ArrayType>(DYNAMIC_CAST(PrimitiveType, $1)->type, DYNAMIC_CAST(Exp, $4)));}
            |Call SC {$$ = $1;}
            |RETURN SC {$$ = MAKE_SHARED_0_ARGS(Return);}
            |RETURN Exp SC {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(Return, Exp, $2);}
            |IF LPAREN Exp RPAREN Statement %prec IF{$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(If, Exp, $3, Statement, $5);}
            |IF LPAREN Exp RPAREN Statement ELSE Statement {$$ = MAKE_SHARED_3_ARGS_LAST_DYNAMIC(If, Exp, $3, Statement, $5, Statement, $7);}
            |WHILE LPAREN Exp RPAREN Statement {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(While, Exp, $3, Statement, $5);}
            |BREAK SC {$$ = MAKE_SHARED_0_ARGS(Break);}
            |CONTINUE SC {$$ = MAKE_SHARED_0_ARGS(Continue);}
;        
Call:   ID LPAREN ExpList RPAREN {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(Call, ID, $1, ExpList, $3);}
        |ID LPAREN RPAREN {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(Call, ID, $1);}
;
ExpList:Exp {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(ExpList, Exp, $1);}
        |Exp COMMA ExpList {$$ = $3;
                            DYNAMIC_CAST(ExpList, $$)->push_front(DYNAMIC_CAST(Exp, $1));}
;
Type:   INT {$$ = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::INT);}
        |BYTE {$$ = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::BYTE);}
        |BOOL {$$ = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::BOOL);}
;
Exp:LPAREN Exp RPAREN {$$ = $2;}
        |Exp AND Exp {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(And, Exp, $1, Exp, $3);}
        |Exp OR Exp {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(Or, Exp, $1, Exp, $3);}
        |ID LBRACK Exp RBRACK {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(ArrayDereference, ID, $1, Exp, $3);}
        |Exp BINOP_ADD Exp {$$ = MAKE_SHARED_3_ARGS(BinOp, Exp, $1, Exp, $3, ast::BinOpType::ADD);}
        |Exp BINOP_SUB Exp {$$ = MAKE_SHARED_3_ARGS(BinOp, Exp, $1, Exp, $3, ast::BinOpType::SUB);}
        |Exp BINOP_MUL Exp {$$ = MAKE_SHARED_3_ARGS(BinOp, Exp, $1, Exp, $3, ast::BinOpType::MUL);}
        |Exp BINOP_DIV Exp {$$ = MAKE_SHARED_3_ARGS(BinOp, Exp, $1, Exp, $3, ast::BinOpType::DIV);}
        |ID {$$ = $1;}
        |Call {$$ = $1;}
        |NUM {$$ = $1;}
        |NUM_B {$$ = $1;}
        |STRING {$$ = $1;}
        |TRUE {$$ = MAKE_SHARED_1_ARGS(Bool, true);}
        |FALSE {$$ = MAKE_SHARED_1_ARGS(Bool, false);}
        |NOT Exp {$$ = MAKE_SHARED_1_ARGS_LAST_DYNAMIC(Not, Exp, $2);}
        |Exp RELOP_EQ Exp {$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::EQ) ;}
        |Exp RELOP_NE Exp{$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::NE);}
        |Exp RELOP_LT Exp {$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::LT) ;}
        |Exp RELOP_LE Exp {$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::LE) ;}
        |Exp RELOP_GT Exp {$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::GT) ;}
        |Exp RELOP_GE Exp {$$ = MAKE_SHARED_3_ARGS(RelOp, Exp, $1, Exp, $3, ast::RelOpType::GE) ;}
        |LPAREN Type RPAREN Exp {$$ = MAKE_SHARED_2_ARGS_LAST_DYNAMIC(Cast, Exp, $4, PrimitiveType, $2);}

;

// TODO: Define grammar here

%%

// TODO: Place any additional code here
void yyerror(const char * message)
{
        output::errorSyn(yylineno);
}
