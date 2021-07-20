//
// Created by 廖治平 on 5/18/21.
//

#ifndef SYSYBACKEND_IRTYPES_H
#define SYSYBACKEND_IRTYPES_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

namespace IntermediateRepresentation {

    enum IRDataType {
        i1, i4, i8, i16, i32,   // types of integers
        str, t_void, label = t_void
    };

    typedef IRDataType IRType;

    enum IRStmtType {
        BR,         // branching
        /**
         * br label  label
         *
         * // conditional branching
         * br <else> %condition, <if true>, <if false>
         * */

        ADD,        // addition
        /**
         * add i32  %dest, i32 %opr1, i32 %opr2
         * %dest = %opr1 + %opr2;
         * */

        MUL,        // multiplication
        /**
         * mul i32  %dest, i32 %opr1, i32 %opr2
         * %dest = %opr1 * %opr2;
         * */

        DIV,        // division
        /**
         * div i32  %dest, i32 %opr1, i32 %opr2
         * %dest = %opr1 / %opr2;
         * */

        MOD,        // residue
        /**
         * mov  i32 %dest, i32 %opr1, i32 %opr2
         * %dest = %opr1 % %opr2;
         * */

        SUB,        // subtraction
        /**
         * sub i32  %dest, i32 %opr1, i32 %opr2
         * %dest = %opr1 - %opr2;
         * */

        CALL,       // function call
        /**
         * call void func <return_var> par1, par2, par3, ...
         *
         * If 'func' returns void, 'return_var' will hold the place but be **ignored** by IRTranslator.
         * */


        RETURN,     // return from a function
        ALLOCA,     // allocating stack
        /**
         * alloca i32 %dest;
         * */

        MOV,        // move or assign. %source is not allowed in SSA form
        /*
         * mov i32 %dest, i32 %source
         * mov i32 %dest, i32 <imm>
         * */

        LABEL,      // label
        LOAD,       // load to variable
        STORE,      // save to allocated space
        CMP_EQ,     // comparison (equal)
        CMP_NE,     // comparison (notequal)
        CMP_UGE,    // comparison (unsigned, greaterequal)
        CMP_ULE,    // comparison (unsigned, lesseuqal)
        CMP_SGE,    // comparison (signed, greatereuqal)
        CMP_SLE,    // comparison (signed, lessequal)
        CMP_SGT,    // comparison (signed, greater)
        CMP_SLT,    // comparison (signed, less)
        GLB_CONST,  // global constant
        GLB_VAR,    // global variable
        /**
         * (glb_var i32), %dest, i32 <val>
         * */

        LSH, RSH,   // bits shifting
        OR, AND, XOR, NOT, // boolean operators
        PHI         // phi function
        /**
         * 5 arguments:
         * phi  <dest_var> [ <varA>, <blockA> ], [ <varB>, <blockB> ]
         *
         * where block{ A | B } are **labels** (blocks);
         * '[', ']' are presented here as semantic separators, and they do **NOT** appear in IR.
         * Arguments inside a bracket pair denote a variable and the branch jumped from.
         * */
    };

    enum IROpType {
        ImmVal, Var, Null
    };

    class NamingUtil {
    public:
        static size_t counter;

        static std::string nextVarName() {
            return std::string("VAR_") + std::to_string(counter++);
        }

        static std::string nextLblName() {
            return std::string("LBL_") + std::to_string(counter++);
        }

        static std::string nextHeapLblName() {
            return std::string("LBL_HP_") + std::to_string(counter++);
        }
    };

    class IROperand {
    private:
        IROpType irOpType;
        IRDataType irValType;
        int32_t Value;
        std::string strValue;
        std::string varName;
        bool isPointer = false;

    public:
        std::string toString() const {
            return std::to_string(irOpType) + "#" + std::to_string(irValType) + "#" +
                   std::to_string(Value) + "#" + strValue + "#" + varName + "#" + std::to_string(isPointer);
        }

        bool operator<(const IROperand &rhs) const {
            return toString() < rhs.toString();
        }

        bool operator>(const IROperand &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const IROperand &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const IROperand &rhs) const {
            return !(*this < rhs);
        }

        bool operator== (const IROperand& b) const {
            return toString() == b.toString();
        }

        const std::string &getStrValue() const {
            return strValue;
        }

        void setStrValue(const std::string &strValue) {
            IROperand::strValue = strValue;
        }

        IROpType getIrOpType() const {
            return irOpType;
        }

        [[deprecated]]
        IRDataType getIrType() const {
            return irValType;
        }

        IRDataType getIrDataType() const {
            return irValType;
        }

        void setIrType(IRDataType irType) {
            IROperand::irValType = irType;
        }

        int32_t getValue() const {
            return Value;
        }

        void setValue(int32_t value) {
            Value = value;
        }

        const std::string &getVarName() const {
            return varName;
        }

        void setVarName(const std::string &varName) {
            IROperand::varName = varName;
        }

        bool getIsPointer() const {
            return isPointer;
        }

        // set pointer
        void setIsPointer(bool isPointer) {
            IROperand::isPointer = isPointer;
        }

        // Immediate Number
        IROperand(IRDataType irValType, int32_t value) : irValType(irValType), Value(value), irOpType(ImmVal), varName("") { }

        // String
        IROperand(std::string strValue) : strValue(std::move(strValue)), irValType(str), irOpType(ImmVal), varName("") { }

        // Variable (specify whether it's a pointer)
        IROperand(IRDataType irValType, std::string varName,
                  bool isPointer) : irValType(irValType), Value(0), varName(std::move(varName)), isPointer(isPointer), irOpType(Var) {
            // if it's anonymous, a generated name will be assigned.
            if (varName.empty())
                this->varName = std::move(NamingUtil::nextVarName());
        }

        // Variable (not a pointer)
        IROperand(IRDataType irValType, std::string varName)
                : irValType(irValType), Value(0), varName(std::move(varName)), irOpType(Var) {
            // if it's anonymous, a generated name will be assigned.
            if (varName.empty())
                this->varName = std::move(NamingUtil::nextVarName());
        }

        // Empty constructor
        IROperand() : irOpType(Null), Value(0), varName("") { }
    };

    class Statement {
        IRStmtType stmtType;
        IRDataType dataType;

        // Ops[0]: Destination
        // Ops[1], [2], ..., [n]: operand 1, 2, 3, ...
        // size >= 1
        std::vector<IROperand> Ops;
        std::string label_name;
    public:

        auto& atOperands(const size_t pos) {
            return Ops[pos];
        }

        auto& operator[] (const size_t pos) {
            return Ops[pos];
        }
        Statement() = default;

        // Label
        explicit Statement(std::string labelName) : stmtType(LABEL), dataType(t_void), label_name(std::move(labelName)) { }

        // Normal statement:
        // 1. Statement type;
        // 2. operands
        Statement(IRStmtType stmtType, IRDataType irDataType, std::vector<IROperand> ops) : stmtType(stmtType), dataType(irDataType), Ops(std::move(ops)) { }

        // Variadic-style statement constructor:
        // 1. Statement Type
        // 2. Statement data type
        // 3. Destination variable
        // 4, 5, 6, ..., N: arguments
        template<typename ...Args>
        Statement(IRStmtType stmtType1, IRDataType irDataType, Args&&... args) : stmtType(stmtType1), dataType(irDataType) {
            (Ops.push_back(std::forward<Args>(args)), ...);
        }

        IRDataType getDataType() const {
            return dataType;
        }

        void setDataType(IRDataType dataType) {
            Statement::dataType = dataType;
        }

        IRStmtType getStmtType() const {
            return stmtType;
        }

        void setStmtType(IRStmtType stmtType) {
            Statement::stmtType = stmtType;
        }

        const std::vector<IROperand> &getOps() const {
            return Ops;
        }

        void setOps(const std::vector<IROperand> &ops) {
            Ops = ops;
        }

        const std::string &getLabelName() const {
            return label_name;
        }

        void setLabelName(const std::string &labelName) {
            label_name = labelName;
        }
    };

    typedef std::vector<Statement> IRSequence;

    class Function {
        std::string funName;
        IRDataType returnType;
        std::vector<Statement> statements;
        std::vector<IROperand> parameters;

    public:
        auto& operator[] (const size_t pos) {
            return statements[pos];
        }

        auto& atStatements(const size_t pos) {
            return statements[pos];
        }

        auto& atParameters(const size_t pos) {
            return parameters[pos];
        }

        Function(std::string funName) : funName(std::move(funName)), returnType(t_void) { }

        Function(std::string funName, IRDataType returnType) : funName(std::move(funName)), returnType(returnType) { }

        Function() = default;

        IRDataType getReturnType() const {
            return returnType;
        }

        void setReturnType(IRDataType returnType) {
            Function::returnType = returnType;
        }

        void insertStatement(const Statement& statement) {
            statements.push_back(statement);
        }

        template<class ...Args>
        void insert(Args&& ... args) {
            (statements.push_back(std::forward<Args>(args)), ...);
        }

        friend Function& operator<< (Function& func, const Statement& statement) {
            func.insertStatement(statement);
            return func;
        }

        template<class ...Args>
        void insertParam(Args&& ...args) {
            (parameters.template emplace_back(std::forward<Args>(args)), ...);
        }

        const std::string &getFunName() const {
            return funName;
        }

        void setFunName(const std::string &funName) {
            Function::funName = funName;
        }

        const std::vector<Statement> &getStatements() const {
            return statements;
        }

        auto &getRefStatements() {
            return statements;
        }

        void setStatements(const std::vector<Statement> &statements) {
            Function::statements = statements;
        }

        const std::vector<IROperand> &getParameters() const {
            return parameters;
        }

        void setParameters(const std::vector<IROperand> &parameters) {
            Function::parameters = parameters;
        }
    };

    class IRProgram {
        std::vector<Statement> global;
        std::vector<Function> functions;


    public:
        // 1. functions, 2. global variables
        IRProgram(std::vector<Statement> global, std::vector<Function> functions) : global(std::move(global)), functions(std::move(functions)) { }
        // default constructor
        IRProgram() = default;

        auto& atGlobal(const size_t pos) {
            return global[pos];
        }

        auto& atFunctions(const size_t pos) {
            return functions[pos];
        }

        // Insert functions
        // eg: insert(func1, func2, func3, ..., funcN)
        template<class ...Args>
        void insert(Args&&... args) {
            (functions.emplace_back(args), ...);
        }

        // Insert globals
        // eg: insert(global1, global2, global3, ..., paramN)
        template<class ...Args>
        void insert(const Statement& glb, Args... args) {
            global.push_back(glb);
            (global.push_back(std::forward<Args>(args)), ...);
        }

        const std::vector<Statement> &getGlobal() const {
            return global;
        }

        void setGlobal(const std::vector<Statement> &global) {
            IRProgram::global = global;
        }

        const std::vector<Function> &getFunctions() const {
            return functions;
        }

        void setFunctions(const std::vector<Function> &functions) {
            IRProgram::functions = functions;
        }
    };
}

#endif //SYSYBACKEND_IRTYPES_H
