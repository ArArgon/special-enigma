//
// Created by 廖治平 on 5/18/21.
//

#ifndef SYSYBACKEND_IRTYPES_H
#define SYSYBACKEND_IRTYPES_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>

namespace IntermediateRepresentation {

    enum IRType {
        i1, i4, i8, i16, i32,   // types of integers
        str, t_void
    };

    enum IRStmtType {
        BR,         // branching
        ADD,        // addition
        MUL,        // multiplication
        DIV,        // division
        SUB,        // subtraction
        CALL,       // function call
        RETURN,     // return from a function
        ALLOCA,     // allocating stack
        LABEL,      // label
        LOAD,       // load to variable
        STORE,      // save to allocated space
        CMP_EQ,     // comparison (equal)
        CMP_UGE,    // comparison (unsigned, greater)
        CMP_SGE,    // comparison (signed, greater)
        CMP_ULE,    // comparison (unsigned, less)
        CMP_SLE,    // comparison (signed, less)

        LSH, RSH,   // bits shifting
        OR, AND, XOR, NOT // boolean operators
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

    size_t NamingUtil::counter = 0;

    class IROperand {
    private:
        IROpType irOpType;
        IRType irValType;
        std::shared_ptr<int64_t> Value;
        std::string strValue;
        std::shared_ptr<std::string> varName;
        bool isPointer = false;

    public:
        const std::string &getStrValue() const {
            return strValue;
        }

        void setStrValue(const std::string &strValue) {
            IROperand::strValue = strValue;
        }

        IRType getIrType() const {
            return irValType;
        }

        void setIrType(IRType irType) {
            IROperand::irValType = irType;
        }

        const std::shared_ptr<int64_t> &getValue() const {
            return Value;
        }

        void setValue(const std::shared_ptr<int64_t> &value) {
            Value = value;
        }

        const std::shared_ptr<std::string> &getVarName() const {
            return varName;
        }

        void setVarName(const std::shared_ptr<std::string> &varName) {
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
        IROperand(IRType irValType, std::shared_ptr<int64_t> value) : irValType(irValType), Value(std::move(value)), irOpType(ImmVal), varName(nullptr) { }

        // String
        IROperand(std::string strValue) : strValue(std::move(strValue)), irValType(str), irOpType(ImmVal), varName(nullptr) { }

        // Variable (specify whether it's a pointer)
        IROperand(IRType irValType, std::shared_ptr<int64_t> value, std::shared_ptr<std::string> varName,
                  bool isPointer) : irValType(irValType), Value(std::move(value)), varName(std::move(varName)), isPointer(isPointer), irOpType(Var) {
            // if it's anonymous, a generated name will be assigned.
            if (varName == nullptr)
                this->varName = std::move(std::make_shared<std::string>(NamingUtil::nextVarName()));
        }

        // Variable (not a pointer)
        IROperand(IRType irValType, std::shared_ptr<int64_t> value, std::shared_ptr<std::string> varName)
                : irValType(irValType), Value(std::move(value)), varName(std::move(varName)), irOpType(Var) {
            // if it's anonymous, a generated name will be assigned.
            if (varName == nullptr)
                this->varName = std::move(std::make_shared<std::string>(NamingUtil::nextVarName()));
        }

        // Empty constructor
        IROperand() : irOpType(Null), Value(nullptr), varName(nullptr) { }
    };

    class Statement {
        IRStmtType stmtType;

        // Ops[0]: Destination
        // Ops[1], [2], ..., [n]: operand 1, 2, 3, ...
        // size >= 1
        std::vector<IROperand> Ops;
        std::string label_name;
    public:

        // Label
        explicit Statement(std::string labelName) : stmtType(LABEL), label_name(std::move(labelName)) { }

        // Normal statement:
        // 1. Statement type;
        // 2. operands
        Statement(IRStmtType stmtType, std::vector<IROperand> ops) : stmtType(stmtType), Ops(std::move(ops)) { }

        // Variadic-style statement constructor:
        // 1. Statement Type
        // 2. Destination variable
        // 3, 4, 5, ..., N: arguments
        template<typename ...Args>
        Statement(IRStmtType stmtType1, const IROperand& dest, Args... args) : stmtType(stmtType1) {
            Ops.emplace_back(dest, args...);
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
        std::vector<Statement> statements;
        std::vector<IROperand> parameters;

    public:

        Function(std::string funName) : funName(std::move(funName)) { }

        void insertStatement(const Statement& statement) {
            statements.push_back(statement);
        }

        template<class Args>
        void insert(Args args...) {
            statements.emplace_back(args);
        }

        friend Function& operator<< (Function& func, const Statement& statement) {
            func.insertStatement(statement);
            return func;
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
        std::vector<IROperand> global;
        std::vector<Function> functions;

        void insert() { };
    public:
        // 1. functions, 2. global variables
        IRProgram(std::vector<IROperand> global, std::vector<Function> functions) : global(std::move(global)), functions(std::move(functions)) { }

        // default constructor
        IRProgram() = default;

        // Insert functions
        // eg: insert(func1, func2, func3, ..., funcN)
        template<class ...Args>
        void insert(Function param, Args... args) {
            functions.push_back(param);
            insert(args...);
        }

        // Insert parameters
        // eg: insert(param1, param2, param3, ..., paramN)
        template<class ...Args>
        void insert(IROperand param, Args... args) {
            global.push_back(param);
            insert(args...);
        }

        const std::vector<IROperand> &getGlobal() const {
            return global;
        }

        void setGlobal(const std::vector<IROperand> &global) {
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
