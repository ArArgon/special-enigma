//
// Created by 廖治平 on 7/19/21.
//

#include "Flow.h"


namespace Backend::Flow {

    void Flow::analyzeBasicBlocks() {
        bb_ptr_t currentBlock = std::make_shared<BasicBlock>(BasicBlock());
        std::unordered_map<std::string, bb_ptr_t> label_map;

        // divide basic blocks
        auto newBB = [&] (bb_ptr_t& block, bool createAnother = true) {
            basicBlocks.push_back(block);
            cfg.newNode(block);
            if (createAnother)
                block = std::make_shared<BasicBlock>(BasicBlock());
        };
        auto& seq = sourceFunc->getRefStatements();
        for (IntermediateRepresentation::Statement& ins : seq) {
            switch (ins.getStmtType()) {
                case IntermediateRepresentation::LABEL:
                    label_map[ins.getOps().at(0).getStrValue()] = currentBlock;
                    newBB(currentBlock);
                    currentBlock->appendStatement(ins);
                    break;
                case IntermediateRepresentation::BR:
                case IntermediateRepresentation::RETURN:
                    currentBlock->appendStatement(ins);
                    newBB(currentBlock);
                    break;
                default:
                    currentBlock->appendStatement(ins);
            }
        }
        if (currentBlock != nullptr && !currentBlock->getStatements().empty())
            newBB(currentBlock, false);

        // buildIntGraph CFG
        for (const auto& bb : basicBlocks) {
            // auto& stmts = bb->getStatements();
            const std::vector<typename BasicBlock::BBStatement>& stmts = bb->getStatements();
            if (stmts.empty())
                continue;
            auto& stmt = (stmts.end() - 1)->statement;
            if (!cfg.containsNode(bb))
                cfg.newNode(bb);
            if (stmt->getStmtType() == IntermediateRepresentation::BR) {
                auto& ops = stmt->getOps();
                if (ops.size() == 1) {
                    // br label  label
                    auto lbl = ops.at(0);
                    cfg.addEdge(bb, label_map[lbl.getStrValue()]);
                } else {
                    // br %condition, label1, label2
                    auto lbl1 = ops.at(1), lbl2 = ops.at(2);
                    cfg.addEdge(bb, label_map[lbl1.getStrValue()]);
                    cfg.addEdge(bb, label_map[lbl2.getStrValue()]);
                }
            }
        }
    }

    void Flow::analyzeLiveness() {
        // calc live sets for basic blocks
        using varSet = std::set<IntermediateRepresentation::IROperand>;

        bool change = true;

        while (change) {
            change = false;
            for (auto& bb : basicBlocks) {
                varSet tmpLiveIn = bb->getUse();
                auto&& successors = cfg.getNeighbours(bb);

                for (const bb_ptr_t& suc : successors) {
                    // LiveOut[bb] U= LiveIn[suc]
                    auto& sucLiveIn = suc->getLiveIn();
                    bb->liveOut.insert(sucLiveIn.begin(), sucLiveIn.end());
                }

                // LiveIn[bb] = use[bb] U ( LiveOut[bb] - def[bb] )
                Util::set_union_to(tmpLiveIn, Util::set_diff(bb->getLiveOut(), bb->getDef()));
                // tmpLiveIn += set_diff(bb->getLiveOut(), bb->getDef());
                change = tmpLiveIn != bb->getLiveIn();
                if (change)
                    bb->setLiveIn(tmpLiveIn);
            }
        }

        // liveness of every statement (inside every basic block)
        for (auto& bb : basicBlocks) {
            auto& stmts = bb->statements;
            int size = static_cast<int>(stmts.size());
            if (!size)
                continue;
            stmts[size - 1].live = Util::set_union(stmts[size - 1].use, Util::set_diff(bb->getLiveOut(), stmts[size - 1].def));
            for (int i = size - 2; i >= 0; i--) {
                auto &cur = stmts[i], &last = stmts[i + 1];
                cur.live = Util::set_union(Util::set_diff(last.live, cur.def), cur.use);
            }
        }
    }

    BasicBlock::BBStatement BasicBlock::procRawStatement(
            IntermediateRepresentation::Statement &stmt) {
        BasicBlock::BBStatement ans;
        ans.statement = std::shared_ptr<IntermediateRepresentation::Statement> (&stmt);
        auto& ops = stmt.getRefOps();
        // TODO process statement
        switch (stmt.getStmtType()) {
            case IntermediateRepresentation::ADD:
            case IntermediateRepresentation::MUL:
            case IntermediateRepresentation::DIV:
            case IntermediateRepresentation::MOD:
            case IntermediateRepresentation::SUB:
            case IntermediateRepresentation::MOV:
            case IntermediateRepresentation::CMP_EQ:
            case IntermediateRepresentation::CMP_NE:
            case IntermediateRepresentation::CMP_SGE:
            case IntermediateRepresentation::CMP_SLE:
            case IntermediateRepresentation::CMP_SGT:
            case IntermediateRepresentation::CMP_SLT:
            case IntermediateRepresentation::LSH:
            case IntermediateRepresentation::RSH:
            case IntermediateRepresentation::OR:
            case IntermediateRepresentation::AND:
            case IntermediateRepresentation::XOR:
            case IntermediateRepresentation::NOT:
            case IntermediateRepresentation::PARAM:
            case IntermediateRepresentation::GLB_ARR:
            case IntermediateRepresentation::GLB_VAR:
            case IntermediateRepresentation::GLB_CONST:
            case IntermediateRepresentation::STK_LOAD:
            case IntermediateRepresentation::LOAD:
            case IntermediateRepresentation::ALLOCA: {
                /*
                * opr %dest, %opr1, %opr2, ...
                * */
                if (ops[0].getIrOpType() != IntermediateRepresentation::Var)
                    throw std::runtime_error("Invalid IR: destination must be a variable. Entailed IR: " + stmt.toString());
                ans.def.insert(ops[0]);
                for (auto it = ops.begin() + 1; it != ops.end(); it++) {
                    if (it->getIrOpType() == IntermediateRepresentation::Var)
                        ans.use.insert(*it);
                }
            }
                break;
            case IntermediateRepresentation::CALL: {
                /*
                 * call     %dest, func, %1, %2, %3, %4, %5, ..., %n
                 * */
                if (ops[0].getIrOpType() == IntermediateRepresentation::Var)
                    ans.def.insert(ops[0]);
                int len = ops.size();
                for (int i = 2; i <= std::min(5, len - 1); i++) {
                    if (ops[i].getIrOpType() == IntermediateRepresentation::Var)
                        ans.use.insert(ops[i]);
                }
            }
                break;
            case IntermediateRepresentation::STK_STR:
            case IntermediateRepresentation::STORE:
            case IntermediateRepresentation::RETURN:
            case IntermediateRepresentation::BR:
                if (ops[0].getIrOpType() == IntermediateRepresentation::Var)
                    ans.use.insert(ops[0]);
                for (auto it = ops.begin() + 1; it != ops.end(); it++) {
                    if (it->getIrOpType() == IntermediateRepresentation::Var)
                        ans.use.insert(*it);
                }
                break;

            case IntermediateRepresentation::PHI:
            case IntermediateRepresentation::LABEL:
                break;
        }

        return ans;
    }

    void BasicBlock::BBStatement::replaceUse(const IntermediateRepresentation::IROperand &oldVar,
                                             const IntermediateRepresentation::IROperand &newVar) {
        if (oldVar == newVar)
            return;
        if (oldVar.getIrOpType() != IntermediateRepresentation::Var)
            throw std::runtime_error("Unable to replace use: " + oldVar.getVarName() + " is not a variable");
        if (newVar.getIrOpType() != IntermediateRepresentation::Var)
            throw std::runtime_error("Unable to replace use: " + newVar.getVarName() + " is not a variable");
        if (!use.count(oldVar))
            throw std::runtime_error("Unable to replace use: " + oldVar.getVarName() + " doesn't present in the instruction");
        use.erase(oldVar);
        use.insert(newVar);
        auto& ops = statement->getRefOps();
        int opsCount = ops.size();
        switch (statement->getStmtType()) {
            // TODO
            case IntermediateRepresentation::BR:
                ops[0] = newVar;
                break;
            case IntermediateRepresentation::CALL: {
                for (int i = 2; i < opsCount; i++)
                    if (ops[i] == oldVar)
                        ops[i] = newVar;
            }
                break;
            case IntermediateRepresentation::STORE:
            case IntermediateRepresentation::STK_STR:
            case IntermediateRepresentation::ADD:
            case IntermediateRepresentation::MUL:
            case IntermediateRepresentation::DIV:
            case IntermediateRepresentation::MOD:
            case IntermediateRepresentation::SUB:
            case IntermediateRepresentation::RETURN:
            case IntermediateRepresentation::MOV:
            case IntermediateRepresentation::LOAD:
            case IntermediateRepresentation::CMP_EQ:
            case IntermediateRepresentation::CMP_NE:
            case IntermediateRepresentation::CMP_SGE:
            case IntermediateRepresentation::CMP_SLE:
            case IntermediateRepresentation::CMP_SGT:
            case IntermediateRepresentation::CMP_SLT:
            case IntermediateRepresentation::LSH:
            case IntermediateRepresentation::RSH:
            case IntermediateRepresentation::OR:
            case IntermediateRepresentation::AND:
            case IntermediateRepresentation::XOR:
            case IntermediateRepresentation::NOT: {
                /*
                 * ins %dest, %opr1, ...
                 * */
                for (int i = 1; i < opsCount; i++)
                    if (ops[i] == oldVar)
                        ops[i] = newVar;
            }
                break;
            default:
                throw std::runtime_error("Unable to replace def: " + std::to_string(statement->getStmtType()) + " has no use");
        }
    }

    void BasicBlock::BBStatement::replaceDef(const IntermediateRepresentation::IROperand &oldVar,
                                             const IntermediateRepresentation::IROperand &newVar) {
        if (oldVar.getIrOpType() != IntermediateRepresentation::Var)
            throw std::runtime_error("Unable to replace def: " + oldVar.getVarName() + " is not a variable");
        if (newVar.getIrOpType() != IntermediateRepresentation::Var)
            throw std::runtime_error("Unable to replace def: " + newVar.getVarName() + " is not a variable");
        if (!def.count(oldVar))
            throw std::runtime_error("Unable to replace def: " + oldVar.getVarName() + " doesn't present in the instruction");
        def.erase(oldVar);
        def.insert(newVar);
        auto& ops = statement->getRefOps();
        switch (statement->getStmtType()) {
            case IntermediateRepresentation::PARAM:
            case IntermediateRepresentation::LOAD:
            case IntermediateRepresentation::STK_LOAD:
            case IntermediateRepresentation::MOV:
            case IntermediateRepresentation::ADD:
            case IntermediateRepresentation::MUL:
            case IntermediateRepresentation::DIV:
            case IntermediateRepresentation::MOD:
            case IntermediateRepresentation::SUB:
            case IntermediateRepresentation::ALLOCA:
            case IntermediateRepresentation::CMP_EQ:
            case IntermediateRepresentation::CMP_NE:
            case IntermediateRepresentation::CMP_SGE:
            case IntermediateRepresentation::CMP_SLE:
            case IntermediateRepresentation::CMP_SGT:
            case IntermediateRepresentation::CMP_SLT:
            case IntermediateRepresentation::GLB_CONST:
            case IntermediateRepresentation::GLB_VAR:
            case IntermediateRepresentation::GLB_ARR:
            case IntermediateRepresentation::LSH:
            case IntermediateRepresentation::RSH:
            case IntermediateRepresentation::OR:
            case IntermediateRepresentation::AND:
            case IntermediateRepresentation::XOR:
            case IntermediateRepresentation::NOT: {
                /*
                 * ins  %dest, xxx
                 *
                 * ins  %replce, xxx
                 * */
                if (ops[0].getIrOpType() != IntermediateRepresentation::Var)
                    throw std::runtime_error("Unable to replace def: destination does not present in IR: '" + std::to_string(statement->getStmtType()) + "'.");
                ops[0] = newVar;
            }
                break;
            case IntermediateRepresentation::CALL: {
                /*
                 * call     %dest, func, %1, ...
                 * */
                if (ops[0].getIrOpType() != IntermediateRepresentation::Var)
                    throw std::runtime_error("Unable to replace def: function call '" + std::to_string(statement->getStmtType()) + "' has no return");
                ops[0] = newVar;
            }
                break;

            default:
                throw std::runtime_error("Unable to replace def: " + std::to_string(statement->getStmtType()) + " has no def");
        }
    }
}