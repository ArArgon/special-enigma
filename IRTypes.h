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
        i4, i8, i16, i32,
        str, t_void
    };

    enum IRStmtType {
        ALLOCA,
        BR,
        ADD,
        MUL,
        CALL,
        LABEL
    };

    enum IROpType {
        ImmVal, Var, Null
    };

    class IROperand {
    private:
        IROpType irOpType;
        IRType irValType;
        std::shared_ptr<int64_t> Value;
        std::shared_ptr<std::string> varName;
        bool isPointer;

    public:
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

        void setIsPointer(bool isPointer) {
            IROperand::isPointer = isPointer;
        }

        IROperand(IRType irType, std::shared_ptr<int64_t> value, std::shared_ptr<std::string> varName)
                : irValType(irType), Value(std::move(value)), varName(std::move(varName)), isPointer(false) { }

        IROperand() : irOpType(Null), Value(nullptr), varName(nullptr) { }

        IROperand(IRType irType, const std::shared_ptr<int64_t> &value, const std::shared_ptr<std::string> &varName,
                  bool isPointer) : irValType(irType), Value(value), varName(varName), isPointer(isPointer) { }
    };

    class Statement {
        IRStmtType stmtType;

        // Ops[0]: Destination
        // Ops[1], [2], ..., [n]: operand 1, 2, 3, ...
        // size >= 1
        std::vector<IROperand> Ops;
        std::string label_name;
    public:
        Statement(std::string labelName) : stmtType(LABEL), label_name(std::move(labelName)) { }

        Statement(IRStmtType stmtType, std::vector<IROperand> ops) : stmtType(stmtType), Ops(std::move(ops)) { }

        template<typename ...Args>
        Statement(IRStmtType stmtType1, const IROperand& dest, Args... args) : stmtType(stmtType1) {
            Ops.template emplace_back(dest, args...);
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
    };

    class IRProgram {
        std::vector<IROperand> global;
        std::vector<Function> functions;

        void insert() { };
    public:
        IRProgram(std::vector<IROperand> global, std::vector<Function> functions) : global(std::move(global)), functions(std::move(functions)) { }
        IRProgram() = default;

        template<class ...Args>
        void insert(Function param, Args... args) {
            functions.push_back(param);
            insert(args...);
        }

        template<class ...Args>
        void insert(IROperand param, Args... args) {
            global.push_back(param);
            insert(args...);
        }
    };
}
#endif //SYSYBACKEND_IRTYPES_H
