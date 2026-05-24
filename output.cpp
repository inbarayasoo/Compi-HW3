#include "output.hpp"
#include <iostream>

#define DYNAMIC_CAST(var_type, value) std::dynamic_pointer_cast<var_type>(value)
#define SHARED_PTR(var_type) std::shared_ptr<ast::var_type>
#define MAKE_SHARED_0_ARGS(out_type) std::make_shared<ast::out_type>()
#define MAKE_SHARED_1_ARGS(out_type, value1) std::make_shared<ast::out_type>(value1)
#define MAKE_SHARED_2_ARGS(out_type, value1, value2) std::make_shared<ast::out_type>(value1, value2)
#define MAKE_SHARED_3_ARGS(out_type, value1, value2, value3) std::make_shared<ast::out_type>(value1, value2, value3)
#define MAKE_SHARED_4_ARGS(out_type, value1, value2, value3, value4) std::make_shared<ast::out_type>(value1, value2, value3, value4)


namespace output
{
  int curScopeOffset = 0;
  bool isNotDecl = true;
  bool is_func = false;
  bool is_function_arg = false; // Track if we're evaluating function arguments
  ast::BuiltInType funcReturnType;
  bool is_assignment_target = false; // Track if we're evaluating assignment target
  bool in_assignment = false; // Track if we're in any part of assignment
  bool is_array_access = false; // Track if we're accessing array for dereferencing
  std::vector<std::shared_ptr<ast::FuncDecl>> globalVariableDeclList;
  std::vector<int> variableCountInScopeList;
  std::vector<std::shared_ptr<ast::Node>> frameVar;
  int getArraySize(const std::shared_ptr<ast::Exp>& sizeExp);
  
    /* Helper functions */

  static std::string toString(ast::BuiltInType type)
  {
    switch (type)
    {
      case ast::BuiltInType::INT:
        return "int";
      case ast::BuiltInType::BOOL:
        return "bool";
      case ast::BuiltInType::BYTE:
        return "byte";
      case ast::BuiltInType::VOID:
        return "void";
      case ast::BuiltInType::STRING:
        return "string";
      default:
        return "unknown";
    }
  }

  /*aux functions*/
  void validateByte(int line, int value) {
    if (value >= 256 || value < 0) {errorByteTooLarge(line, value);}
  }

  bool matchVar(const std::shared_ptr<ast::Node>& var, ast::ID& node, bool is_func) {
    auto decl = DYNAMIC_CAST(ast::VarDecl, var);
    if (decl && decl->id->value == node.value) {
        // Check if it's a primitive type
        auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, decl->type);
        if (primitiveType) {
            node.type = primitiveType->type;
        } 
        else 
        {
            node.type = ast::BuiltInType::TBD; // Special marker for arrays
        }
        
        is_func ? errorDefAsVar(node.line, node.value) : void();
        return true;
    }
    return false;
  }

  bool matchFormal(const std::shared_ptr<ast::Node>& var, ast::ID& node, bool is_func) {
      auto formal = DYNAMIC_CAST(ast::Formal, var);
      if (formal && formal->id->value == node.value) {
          // Check if it's a primitive type
          auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, formal->type);
          if (primitiveType) {
              node.type = primitiveType->type;
          } else
          {
              node.type = ast::BuiltInType::TBD; // Special marker for arrays
          }
          
          is_func ? errorDefAsVar(node.line, node.value) : void();
          return true;
      }
      return false;
  }

  bool matchGlobalFunc(const std::shared_ptr<ast::FuncDecl>& func, ast::ID& node, bool is_func) {
    if (func->id->value == node.value) {
        if (!is_func) 
        {
          errorDefAsFunc(node.line, node.value);
        }
        return true;
    }
    return false;
  }

  bool isIntOrByte(ast::BuiltInType type) {
      return type == ast::BuiltInType::INT || type == ast::BuiltInType::BYTE;
  }

  ast::BuiltInType getResType(const std::shared_ptr<ast::Node> &left,
                                      const std::shared_ptr<ast::Node> &right) {
                                        
      auto left_exp = DYNAMIC_CAST(ast::Exp, left);
      auto right_exp = DYNAMIC_CAST(ast::Exp, right);

      return (left_exp && left_exp->type == ast::BuiltInType::BYTE 
              && right_exp && right_exp->type == ast::BuiltInType::BYTE) ?
        ast::BuiltInType::BYTE : ast::BuiltInType::INT;
  }

  bool isSameVar(const std::shared_ptr<ast::Node> &var, const std::string &name) {
      std::shared_ptr<ast::VarDecl> decl = DYNAMIC_CAST(ast::VarDecl, var);
      return decl && decl->id->value == name;
  }

  bool isSameFormal(const std::shared_ptr<ast::Node> &var, const std::string &name) {
      std::shared_ptr<ast::Formal> formal = DYNAMIC_CAST(ast::Formal, var);
      return formal && formal->id->value == name;
  }

  bool isDefinedAsVar(const std::vector<std::shared_ptr<ast::Node>> &vars, const std::string &name) {
      std::vector<std::shared_ptr<ast::Node>>::const_iterator it = vars.begin();
      while (it != vars.end()) {
          if (isSameVar(*it, name) || isSameFormal(*it, name)) {
              return true;
          }
          ++it;
      }
      return false;
  }

  bool isValidArg(ast::BuiltInType formalType, ast::BuiltInType argumentType) {
      return formalType == argumentType ||
            (formalType == ast::BuiltInType::INT && argumentType == ast::BuiltInType::BYTE);
  }

  bool checkValidSignature(const std::vector<std::shared_ptr<ast::Formal>> &formals_list,
                      const std::vector<std::shared_ptr<ast::Exp>> &args_list) {
      for (size_t i = 0; i < formals_list.size(); ++i) {
          const auto& formal_param = formals_list[i];
          const auto& arg_exp = args_list[i];     

          std::shared_ptr<ast::Type> formal_type_node = formal_param->type;
          ast::BuiltInType formal_built_in_type;

          auto p_type = DYNAMIC_CAST(ast::PrimitiveType, formal_type_node);
          if (p_type) {
              formal_built_in_type = p_type->type;
          } 

          if (!isValidArg(formal_built_in_type, arg_exp->type)) {
              return false;
          }
      }
      return true;
  }

  std::vector<std::string> getParamTypes(const std::vector<std::shared_ptr<ast::Formal>> &formals) {
      std::vector<std::string> types_vec;
      std::vector<std::shared_ptr<ast::Formal>>::const_iterator it = formals.begin();
      while (it != formals.end()) {
          auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, (*it)->type);
          if (primitiveType) {
              switch (primitiveType->type) {
                  case ast::BuiltInType::BOOL:   types_vec.push_back("BOOL"); break;
                  case ast::BuiltInType::INT:    types_vec.push_back("INT"); break;
                  case ast::BuiltInType::STRING: types_vec.push_back("STRING"); break;
                  case ast::BuiltInType::BYTE:   types_vec.push_back("BYTE"); break;
                  case ast::BuiltInType::VOID:   types_vec.push_back("VOID"); break;
              }
          } 
          else 
          {
              auto arrayType = DYNAMIC_CAST(ast::ArrayType, (*it)->type);
              if (arrayType) {
                  switch (arrayType->type) {
                      case ast::BuiltInType::BOOL:   types_vec.push_back("BOOL"); break;
                      case ast::BuiltInType::INT:    types_vec.push_back("INT"); break;
                      case ast::BuiltInType::BYTE:   types_vec.push_back("BYTE"); break;
                  }
              }
          }
          ++it;
      }
      return types_vec;
  }

  void validateFuncCall(const ast::Call &node, const std::shared_ptr<ast::FuncDecl> &func) {
      const std::vector<std::shared_ptr<ast::Formal>> &formals = func->formals->formals;
      const std::vector<std::shared_ptr<ast::Exp>> &args = node.args->exps;

      if (formals.size() != args.size() || !checkValidSignature(formals, args)) {
          std::vector<std::string> paramTypes = getParamTypes(formals);
          errorPrototypeMismatch(node.line, node.func_id->value, paramTypes);
      }
  }

  void enterBlockScope(ScopePrinter &printer) {
      printer.beginScope();
      printer.entryFrame();
  }

  void exitBlockScope(ScopePrinter &printer) {
      printer.endScope();
      printer.exitFrame();
  }

  void visitStatementInScope(std::shared_ptr<ast::Statement> stmt, ScopePrinter &printer) {
      if (stmt->openExtraBraces) {
          enterBlockScope(printer);
          stmt->accept(printer);
          exitBlockScope(printer);
      } else {
          stmt->accept(printer);
      }
  }

  bool checkReturnType(ast::BuiltInType expected_type, ast::BuiltInType cur_type) {
      return expected_type == cur_type || (expected_type == ast::BuiltInType::INT && cur_type == ast::BuiltInType::BYTE);
  }

  std::shared_ptr<ast::VarDecl> findVarDeclInFrame(const std::string &name,
                                                  const std::vector<std::shared_ptr<ast::Node>> &frameVar) {
      std::vector<std::shared_ptr<ast::Node>>::const_iterator it = frameVar.begin();
      while (it != frameVar.end()) {
          std::shared_ptr<ast::VarDecl> decl = DYNAMIC_CAST(ast::VarDecl, *it);
          if (decl && decl->id->value == name) return decl;
          ++it;
      }
      return nullptr;
  }

  std::shared_ptr<ast::Formal> findFormalInFrame(const std::string &name,
                                                const std::vector<std::shared_ptr<ast::Node>> &frameVar) {
      std::vector<std::shared_ptr<ast::Node>>::const_iterator it = frameVar.begin();
      while (it != frameVar.end()) {
          std::shared_ptr<ast::Formal> formal = DYNAMIC_CAST(ast::Formal, *it);
          if (formal && formal->id->value == name) return formal;
          ++it;
      }
      return nullptr;
  }


  bool isDeclaredAsFunction(const std::string &name,
                            const std::vector<std::shared_ptr<ast::FuncDecl>> &funcs) {
      std::vector<std::shared_ptr<ast::FuncDecl>>::const_iterator it = funcs.begin();
      while (it != funcs.end()) {
          if ((*it)->id->value == name) return true;
          ++it;
      }
      return false;
  }

  int getArraySize(const std::shared_ptr<ast::Exp>& sizeExp) {
      auto numNode = DYNAMIC_CAST(ast::Num, sizeExp);
      if (numNode) {
          return numNode->value;
      }
      
      auto numBNode = DYNAMIC_CAST(ast::NumB, sizeExp);
      if (numBNode) {
          return numBNode->value;
      }
      
      return 0; // Default fallback
  }

  ast::BuiltInType getArrayElementType(const std::string &name, 
                                    const std::vector<std::shared_ptr<ast::Node>> &frameVar) {
      // Search in frame variables
      for (const auto &var : frameVar) {
          auto decl = DYNAMIC_CAST(ast::VarDecl, var);
          if (decl && decl->id->value == name) {
              auto arrayType = DYNAMIC_CAST(ast::ArrayType, decl->type);
              if (arrayType) {
                  return arrayType->type;
              }
          }
          
          auto formal = DYNAMIC_CAST(ast::Formal, var);
          if (formal && formal->id->value == name) {
              auto arrayType = DYNAMIC_CAST(ast::ArrayType, formal->type);
              if (arrayType) {
                  return arrayType->type;
              }
          }
      }
      
      return ast::BuiltInType::TBD; // Not found or not an array
  }

    /* Error handling functions */

  void errorLex(int lineno) {
      std::cout << "line " << lineno << ": lexical error\n";
      exit(0);
  }

  void errorSyn(int lineno) {
      std::cout << "line " << lineno << ": syntax error\n";
      exit(0);
  }

  void errorUndef(int lineno, const std::string &id) {
      std::cout << "line " << lineno << ":" << " variable " << id << " is not defined" << std::endl;
      exit(0);
  }

  void errorDefAsFunc(int lineno, const std::string &id) {
      std::cout << "line " << lineno << ":" << " symbol " << id << " is a function" << std::endl;
      exit(0);
  }

  void errorDefAsVar(int lineno, const std::string &id) {
      std::cout << "line " << lineno << ":" << " symbol " << id << " is a variable" << std::endl;
      exit(0);
  }

  void errorDef(int lineno, const std::string &id) {
      std::cout << "line " << lineno << ":" << " symbol " << id << " is already defined" << std::endl;
      exit(0);
  }

  void errorUndefFunc(int lineno, const std::string &id) {
      std::cout << "line " << lineno << ":" << " function " << id << " is not defined" << std::endl;
      exit(0);
  }

  void errorMismatch(int lineno) {
      std::cout << "line " << lineno << ":" << " type mismatch" << std::endl;
      exit(0);
  }

  void errorPrototypeMismatch(int lineno, const std::string &id, std::vector<std::string> &paramTypes) {
      std::cout << "line " << lineno << ": prototype mismatch, function " << id << " expects parameters (";

      for (int i = 0; i < paramTypes.size(); ++i) {
          std::cout << paramTypes[i];
          if (i != paramTypes.size() - 1)
              std::cout << ",";
      }

      std::cout << ")" << std::endl;
      exit(0);
  }

  void errorUnexpectedBreak(int lineno) {
      std::cout << "line " << lineno << ":" << " unexpected break statement" << std::endl;
      exit(0);
  }

  void errorUnexpectedContinue(int lineno) {
      std::cout << "line " << lineno << ":" << " unexpected continue statement" << std::endl;
      exit(0);
  }

  void errorMainMissing() {
      std::cout << "Program has no 'void main()' function" << std::endl;
      exit(0);
  }

  void errorByteTooLarge(int lineno, const int value) {
      std::cout << "line " << lineno << ": byte value " << value << " out of range" << std::endl;
      exit(0);
  }

  void ErrorInvalidAssignArray(int lineno, const std::string &id_arr) {
      std::cout << "line " << lineno << ": invalid assignment to array " << id_arr << std::endl;
      exit(0);
  }
  /* ScopePrinter class */

  ScopePrinter::ScopePrinter() : indentLevel(0) {}

  std::string ScopePrinter::indent() const
  {
    std::string result;
    for (int i = 0; i < indentLevel; ++i)
    {
      result += "  ";
    }
    return result;
  }

  void ScopePrinter::beginScope()
  {
    indentLevel++;
    buffer << indent() << "---begin scope---" << std::endl;
  }

  void ScopePrinter::endScope()
  {
    buffer << indent() << "---end scope---" << std::endl;
    indentLevel--;
  }

  void ScopePrinter::emitVar(const std::string &id, const ast::BuiltInType &type,
                              int offset)
  {
    buffer << indent() << id << " " << toString(type) << " " << offset
            << std::endl;
  }

  void ScopePrinter::emitArr(const std::string &id, const ast::BuiltInType &type, int length , int offset ) {
    buffer << indent() << id << "[" << length << "]" << " " << toString(type) << " " << offset <<  std::endl;
  }

  void ScopePrinter::emitFunc(const std::string &id,
                              const ast::BuiltInType &returnType,
                              const std::vector<ast::BuiltInType> &paramTypes)
  {
    globalsBuffer << id << " " << "(";

    for (int i = 0; i < paramTypes.size(); ++i)
    {
      globalsBuffer << toString(paramTypes[i]);
      if (i != paramTypes.size() - 1)
        globalsBuffer << ",";
    }

    globalsBuffer << ")" << " -> " << toString(returnType) << std::endl;
  }

  std::ostream &operator<<(std::ostream &os, const ScopePrinter &printer)
  {
    os << "---begin global scope---" << std::endl;
    os << printer.globalsBuffer.str();
    os << printer.buffer.str();
    os << "---end global scope---" << std::endl;
    return os;
  }

  void ScopePrinter::entryFrame() {
    variableCountInScopeList.push_back(0);
  }

  void ScopePrinter::exitFrame() {
      if (!variableCountInScopeList.empty()) {
          for (int i = 0; i < variableCountInScopeList.back(); i++) {
            auto decl = DYNAMIC_CAST(ast::VarDecl, frameVar.back());
            if (decl) 
            {
                auto array_type = DYNAMIC_CAST(ast::ArrayType, decl->type);
                if (array_type) 
                {
                  curScopeOffset -= getArraySize(array_type->length);
                }
                else 
                {
                  curScopeOffset-=1;
                }
            }
            else
            {
              curScopeOffset -= 1;
            }
            frameVar.pop_back();
          }
          variableCountInScopeList.pop_back();
      }
  }

  //-------------------------------------------visitor-------------------------------//

  void ScopePrinter::visit(ast::Num &node)
  { 
    node.type = ast::BuiltInType::INT; 
  }

  void ScopePrinter::visit(ast::NumB &node)
  {
    validateByte(node.line, node.value);
    node.type = ast::BuiltInType::BYTE;
  }

  void ScopePrinter::visit(ast::String &node)
  {
    node.type = ast::BuiltInType::STRING;
  }

  void ScopePrinter::visit(ast::Bool &node)
  {
    node.type = ast::BuiltInType::BOOL;
  }

  void ScopePrinter::visit(ast::ID &node)
{
    node.valueExp = node.value;
    if (!isNotDecl) return;

    bool found = false;

    auto it = frameVar.begin();
    while (it != frameVar.end()) {
        found = matchVar(*it, node, is_func) || matchFormal(*it, node, is_func);
        if(found) {
            // Only throw immediate errors for arrays in non-assignment contexts
            if (!is_func && !is_assignment_target && !is_array_access && !is_function_arg && 
                !in_assignment && node.type == ast::BuiltInType::TBD) {
                // This is an array being used as a value in invalid context
                errorMismatch(node.line);
            }
            return;
        }
        ++it;
    }

    auto glob_it = globalVariableDeclList.begin();
    while (glob_it != globalVariableDeclList.end()) {
        found = matchGlobalFunc(*glob_it, node, is_func);
        if(found)
          return;
        ++glob_it;
    }

    is_func ? errorUndefFunc(node.line, node.value) : errorUndef(node.line, node.value);
}

  void ScopePrinter::visit(ast::BinOp &node)
  {
      node.left->accept(*this);
      node.right->accept(*this);
      (isIntOrByte(node.right->type) && isIntOrByte(node.left->type)) ? void() : errorMismatch(node.line);
      node.type = getResType(node.left, node.right);
  }

  void ScopePrinter::visit(ast::RelOp &node)
  {
      node.left->accept(*this);
      node.right->accept(*this);
      (isIntOrByte(node.right->type) && isIntOrByte(node.left->type)) ? void() : errorMismatch(node.line);
      node.type = ast::BuiltInType::BOOL;
  }

  void ScopePrinter::visit(ast::PrimitiveType &node) {}

  void ScopePrinter::visit(ast::Cast &node)
  {
    // visit nodes
    node.exp->accept(*this);
    node.target_type->accept(*this);
    (isIntOrByte(node.exp->type) && isIntOrByte(node.target_type->type)) ? void() : errorMismatch(node.line);
    node.type = node.target_type->type;
  }

  void ScopePrinter::visit(ast::Not &node)
  {
    node.exp->accept(*this);
    (node.exp->type == ast::BuiltInType::BOOL) ? void () : errorMismatch(node.line);
    node.type = ast::BuiltInType::BOOL;
  }

  void ScopePrinter::visit(ast::And &node)
  {
    node.left->accept(*this);
    node.right->accept(*this);
    (node.left->type == ast::BuiltInType::BOOL && node.right->type == ast::BuiltInType::BOOL) ? 
    void() : errorMismatch(node.line);
    node.type = ast::BuiltInType::BOOL;
  }

  void ScopePrinter::visit(ast::Or &node)
  {
    node.left->accept(*this);
    node.right->accept(*this);
    (node.left->type == ast::BuiltInType::BOOL && node.right->type == ast::BuiltInType::BOOL) ? 
    void() : errorMismatch(node.line);
    node.type = ast::BuiltInType::BOOL;
  }

  void ScopePrinter::visit(ast::ExpList &node) {
    std::vector<std::shared_ptr<ast::Exp>>::iterator it = node.exps.begin();
    while (it != node.exps.end()) {
        (*it)->accept(*this);
        ++it;
    }
  }

  void ScopePrinter::visit(ast::Call &node)
  {
    is_func = true;
    node.func_id->accept(*this);
    is_func = false;
    
    // Set flag when evaluating function arguments
    is_function_arg = true;
    node.args->accept(*this);
    is_function_arg = false;
    
    std::vector<std::shared_ptr<ast::FuncDecl>>::iterator it = globalVariableDeclList.begin();
    while (it != globalVariableDeclList.end()) {
        if ((*it)->id->value == node.func_id->value) {
            node.type = std::dynamic_pointer_cast<ast::PrimitiveType>((*it)->return_type)->type;
            validateFuncCall(node, *it);
            return;
        }
        ++it;
    }
    (isDefinedAsVar(frameVar, node.func_id->value)) ?  errorDefAsVar(node.line, node.func_id->value) : errorUndefFunc(node.line, node.func_id->value);
  }

  void ScopePrinter::visit(ast::Statements &node) {
    std::vector<std::shared_ptr<ast::Statement>>::iterator it = node.statements.begin();
    while (it != node.statements.end()) {
        visitStatementInScope(*it, *this);
        ++it;
    }
  }

  void ScopePrinter::visit(ast::Break &node)
  {
    (insideWhile == 0) ? errorUnexpectedBreak(node.line) : void () ;
  }

  void ScopePrinter::visit(ast::Continue &node)
  {
    (insideWhile == 0) ? errorUnexpectedContinue(node.line) : void ();
  }

  void ScopePrinter::visit(ast::Return &node) {
    node.exp ? node.exp->accept(*this) : void();

    bool valid =
        node.exp
        ? checkReturnType(funcReturnType, node.exp->type)
        : funcReturnType == ast::BuiltInType::VOID;

    valid ? void() : errorMismatch(node.line);
  }

  void ScopePrinter::visit(ast::If &node) {
    enterBlockScope(*this);
    node.condition->accept(*this);
    (node.condition->type == ast::BuiltInType::BOOL) ? void() : errorMismatch(node.condition->line);

    visitStatementInScope(node.then, *this);
    exitBlockScope(*this);

    if (node.otherwise) {
        enterBlockScope(*this);
        visitStatementInScope(node.otherwise, *this);
        exitBlockScope(*this);
    }
  }

  void ScopePrinter::visit(ast::While &node) {
    enterBlockScope(*this);
    insideWhile++;

    node.condition->accept(*this);
    (node.condition->type == ast::BuiltInType::BOOL) ? void() : errorMismatch(node.condition->line);

    visitStatementInScope(node.body, *this);

    insideWhile--;
    exitBlockScope(*this);
  }

  void ScopePrinter::visit(ast::VarDecl &node) {
      isNotDecl = false;
      node.id->accept(*this);
      isNotDecl = true;
      node.type->accept(*this);
      if (node.init_exp) node.init_exp->accept(*this);

      const std::string &name = node.id->value;

      isDefinedAsVar(frameVar, name) || isDeclaredAsFunction(name, globalVariableDeclList) ? errorDef(node.line, name) : void();

      // Check if it's an array type
      auto arrayType = DYNAMIC_CAST(ast::ArrayType, node.type);
      if (arrayType) {
          // Handle array declaration
          if (node.init_exp) {
              // Arrays cannot be initialized in declaration
              ErrorInvalidAssignArray(node.line, name);
          }
          
          // Get array size
          int arraySize = getArraySize(arrayType->length);
          
          // Validate array size is positive
          // if (arraySize <= 0) {
          //     errorMismatch(node.line);
          // }
          
          // Add array to symbol table
          frameVar.push_back(std::make_shared<ast::VarDecl>(node));
          if (!variableCountInScopeList.empty()) {
              variableCountInScopeList.back()++;
          }
          
          // Emit array variable with its base offset and size
          emitArr(name, arrayType->type, arraySize, curScopeOffset);
          
          // Increment curScopeOffset by array size to reserve space for all elements
          curScopeOffset += arraySize;
      } else {
          // Handle primitive type declaration (existing code)
          auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, node.type);
          if (primitiveType) {
              if (node.init_exp &&
                  !checkReturnType(primitiveType->type, node.init_exp->type)) {
                  errorMismatch(node.line);
              }
              
              frameVar.push_back(std::make_shared<ast::VarDecl>(node));
              if (!variableCountInScopeList.empty()) {
                  variableCountInScopeList.back()++;
              }
              emitVar(name, primitiveType->type, curScopeOffset++);
          }
      }
  }

  void ScopePrinter::visit(ast::Assign &node) {
    // Mark that we're in assignment context
    in_assignment = true;
    
    // Step 1: First check the right side (expression being assigned)
    node.exp->accept(*this);
    
    // Step 2: Then check the left side (assignment target)
    // Set flag to indicate we're checking assignment target
    is_assignment_target = true;
    node.id->accept(*this);
    is_assignment_target = false;
    
    // Clear assignment context
    in_assignment = false;

    const std::string &name = node.id->value;

    // Step 3: Check if trying to assign TO an array (immediate error)
    auto decl = findVarDeclInFrame(name, frameVar);
    auto formal = findFormalInFrame(name, frameVar);

    if (decl) {
        // Check if trying to assign to an array
        auto arrayType = DYNAMIC_CAST(ast::ArrayType, decl->type);
        if (arrayType) {
            // Immediate error for assigning to array
            ErrorInvalidAssignArray(node.line, name);
            return;
        }
        
        // Step 4: Check if right side was an array (deferred error)
        if (node.exp->type == ast::BuiltInType::TBD) {
            // Right side was an array
            errorMismatch(node.line);
            return;
        }
        
        // Step 5: Only after all checks, verify type compatibility
        auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, decl->type);
        if (primitiveType && !checkReturnType(primitiveType->type, node.exp->type)) {
            errorMismatch(node.line);
        }
    } else if (formal) {
        // Check if trying to assign to an array formal parameter
        auto arrayType = DYNAMIC_CAST(ast::ArrayType, formal->type);
        if (arrayType) {
            // Immediate error for assigning to array
            ErrorInvalidAssignArray(node.line, name);
            return;
        }
        
        // Step 4: Check if right side was an array (deferred error)
        if (node.exp->type == ast::BuiltInType::TBD) {
            // Right side was an array
            errorMismatch(node.line);
            return;
        }
        
        // Step 5: Only after all checks, verify type compatibility
        auto primitiveType = DYNAMIC_CAST(ast::PrimitiveType, formal->type);
        if (primitiveType && !checkReturnType(primitiveType->type, node.exp->type)) {
            errorMismatch(node.line);
        }
    } else {
        // Variable not found
        if (isDeclaredAsFunction(name, globalVariableDeclList)) {
            errorDefAsFunc(node.line, name);
        }
        errorUndef(node.line, name);
    }
}

  void ScopePrinter::visit(ast::Formal &node) {
    isNotDecl = false;
    node.id->accept(*this);
    isNotDecl = true;
    node.type->accept(*this);

    const std::string &name = node.id->value;

    isDefinedAsVar(frameVar, name) ? errorDef(node.id->line, name) : void();
    isDeclaredAsFunction(name, globalVariableDeclList) ? errorDef(node.id->line, name) : void();

    // Check if trying to use array as function parameter
    auto arrayType = DYNAMIC_CAST(ast::ArrayType, node.type);
    if (arrayType) {
        errorMismatch(node.id->line);
        return;
    }

    frameVar.push_back(std::make_shared<ast::Formal>(node));
    variableCountInScopeList.back()++;
    emitVar(name, DYNAMIC_CAST(ast::PrimitiveType, node.type)->type, --curScopeOffset);
  }

  void ScopePrinter::visit(ast::Formals &node) {
    std::vector<std::shared_ptr<ast::Formal>>::iterator it = node.formals.begin();
    while (it != node.formals.end()) {
        (*it)->accept(*this);
        ++it;
    }
    curScopeOffset = 0;
  }

  void ScopePrinter::visit(ast::FuncDecl &node) {
    enterBlockScope(*this);
    isNotDecl = false;
    node.id->accept(*this);
    isNotDecl = true;
    
    // Check if trying to use array as return type
    auto arrayReturnType = DYNAMIC_CAST(ast::ArrayType, node.return_type);
    if (arrayReturnType) {
        errorMismatch(node.id->line);
        return;
    }
    
    node.return_type->accept(*this);
    funcReturnType = DYNAMIC_CAST(ast::PrimitiveType, node.return_type)->type;
    node.formals->accept(*this);
    node.body->accept(*this);
    exitBlockScope(*this);
  }

  void ScopePrinter::visit(ast::Funcs &node)
{
  // print
  SHARED_PTR(ID) func_ID = MAKE_SHARED_1_ARGS(ID, "print");
  SHARED_PTR(PrimitiveType) func_returnType = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::VOID);
  SHARED_PTR(ID) func_formal_ID = MAKE_SHARED_1_ARGS(ID, "var");
  SHARED_PTR(PrimitiveType) func_formal_Type = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::STRING);
  SHARED_PTR(Formal) func_formal = MAKE_SHARED_2_ARGS(Formal, func_formal_ID, func_formal_Type);
  SHARED_PTR(Formals) func_formals = MAKE_SHARED_1_ARGS(Formals, func_formal);
  SHARED_PTR(Statements) func_statements = MAKE_SHARED_0_ARGS(Statements);
  SHARED_PTR(FuncDecl) func_FuncDecl = MAKE_SHARED_4_ARGS(FuncDecl, func_ID, func_returnType, func_formals, func_statements);

  globalVariableDeclList.push_back(func_FuncDecl);
  std::vector<ast::BuiltInType> paramTypes;
  for (auto formal : func_formals->formals)
  {
    paramTypes.push_back(DYNAMIC_CAST(ast::PrimitiveType, formal->type)->type);
  }
  emitFunc(func_ID->value, DYNAMIC_CAST(ast::PrimitiveType, func_returnType)->type, paramTypes);

  // printi
  func_ID = MAKE_SHARED_1_ARGS(ID, "printi");
  func_returnType = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::VOID);
  func_formal_ID = MAKE_SHARED_1_ARGS(ID, "var");
  func_formal_Type = MAKE_SHARED_1_ARGS(PrimitiveType, ast::BuiltInType::INT);
  func_formal = MAKE_SHARED_2_ARGS(Formal, func_formal_ID, func_formal_Type);
  func_formals = MAKE_SHARED_1_ARGS(Formals, func_formal);
  func_statements = MAKE_SHARED_0_ARGS(Statements);
  func_FuncDecl = MAKE_SHARED_4_ARGS(FuncDecl, func_ID, func_returnType, func_formals, func_statements);

  globalVariableDeclList.push_back(func_FuncDecl);
  std::vector<ast::BuiltInType> paramTypesi;
  for (auto formal : func_formals->formals)
  {
    paramTypesi.push_back(DYNAMIC_CAST(ast::PrimitiveType, formal->type)->type);
  }
  emitFunc(func_ID->value, DYNAMIC_CAST(ast::PrimitiveType, func_returnType)->type, paramTypesi);
  
  // First pass: Check for duplicate function names and add to global list
  auto func = node.funcs.begin();
  while (func != node.funcs.end())
  {
    // Check for duplicate function names (including with built-in functions)
    auto global_func = globalVariableDeclList.begin();
    while (global_func != globalVariableDeclList.end())
    {
      if ((*global_func)->id->value == (*func)->id->value)
      {
        errorDef((*func)->id->line, (*func)->id->value);
      }
      ++global_func;
    }

    globalVariableDeclList.push_back(*func);

    paramTypes.clear();
    auto formals = (*func)->formals->formals.begin();
    while (formals != (*func)->formals->formals.end())
    {
      paramTypes.push_back(DYNAMIC_CAST(ast::PrimitiveType, (*formals)->type)->type);
      ++formals;
    }

    emitFunc((*func)->id->value, DYNAMIC_CAST(ast::PrimitiveType, (*func)->return_type)->type, paramTypes);
    ++func;
  }
  
  // Second pass: Check for valid main function
  bool is_main = false;
  func = node.funcs.begin();
  while (func != node.funcs.end())
  {
    if ((*func)->id->value == "main")
    {
      if ((*func)->formals->formals.empty() &&
          DYNAMIC_CAST(ast::PrimitiveType, (*func)->return_type)->type == ast::BuiltInType::VOID)
      {
        is_main = true;
      }
    }
    ++func;
  }
  
  if (!is_main)
  {
    errorMainMissing();
  }

  for (auto it : node.funcs)
  {
    curScopeOffset = 0;
    it->accept(*this);
  }
  std::cout << *this;
}

  void ScopePrinter::visit(ast::ArrayType &node) {
    if (node.length) {
        node.length->accept(*this);
        
        if (!isIntOrByte(node.length->type)) {
            errorMismatch(node.length->line);
        }
        
        // Check if the size expression is a constant (Num or NumB)
        auto numNode = DYNAMIC_CAST(ast::Num, node.length);
        auto numBNode = DYNAMIC_CAST(ast::NumB, node.length);
        
        if (!numNode && !numBNode) {
            // Array size is not a constant - it's a variable or expression
            errorMismatch(node.length->line);
        }
    }
}

  void ScopePrinter::visit(ast::ArrayDereference &node) {
    is_array_access = true;
    node.id->accept(*this);
    is_array_access = false;
    
    node.index->accept(*this);
    
    if (!isIntOrByte(node.index->type)) {
        errorMismatch(node.index->line);
    }
    
    ast::BuiltInType elementType = getArrayElementType(node.id->value, frameVar);
    if (elementType == ast::BuiltInType::TBD) {
        errorMismatch(node.line);
        return;
    }
    
    node.type = elementType;
}

  void ScopePrinter::visit(ast::ArrayAssign &node) {
    is_array_access = true;
    node.id->accept(*this);
    is_array_access = false;
    
    node.index->accept(*this);
    
    node.exp->accept(*this);
    
    if (!isIntOrByte(node.index->type)) {
        errorMismatch(node.index->line);
    }
    
    ast::BuiltInType arrayElementType = getArrayElementType(node.id->value, frameVar);
    if (arrayElementType == ast::BuiltInType::TBD) {
        errorMismatch(node.line);
        return;
    }
    
    if (!checkReturnType(arrayElementType, node.exp->type)) {
        errorMismatch(node.line);
    }
}
} // namespace output