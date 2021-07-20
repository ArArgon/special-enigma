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
        if (currentBlock != nullptr)
            newBB(currentBlock, false);

        // buildIntGraph CFG
        for (const auto& bb : basicBlocks) {
            // auto& stmts = bb->getStatements();
            const std::vector<typename BasicBlock::BBStatement>& stmts = bb->getStatements();
            if (stmts.empty())
                continue;
            auto& stmt = (stmts.end() - 1)->statement;
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

//        auto set_diff = [] (const varSet& a, const varSet& b) {
//            // return a - b;
//            varSet ans;
//            std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ans, ans.begin()));
//            return ans;
//        };
//
//        auto set_union_to = [] (varSet& target, const varSet& b) {
//            target.insert(b.begin(), b.end());
//        };
//
//        auto set_union = [] (const varSet& a, const varSet& b) {
//            varSet ans = a;
//            ans.insert(b.begin(), b.end());
//            return ans;
//        };

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
                auto &cur = stmts[i], &last = stmts[i - 1];
                cur.live = Util::set_union(Util::set_diff(last.live, cur.def), cur.use);
            }
        }
    }

    BasicBlock::BBStatement BasicBlock::procRawStatement(
            IntermediateRepresentation::Statement &stmt) {
        BasicBlock::BBStatement ans;
        ans.statement = std::shared_ptr<IntermediateRepresentation::Statement> (&stmt);

        switch (stmt.getStmtType()) {
            case IntermediateRepresentation::ADD:
            case IntermediateRepresentation::MUL:
            case IntermediateRepresentation::DIV:
            case IntermediateRepresentation::MOD:
            case IntermediateRepresentation::SUB: {
                /*
                * opr %dest, %opr1, %opr2
                * */
            }
                break;
            case IntermediateRepresentation::CALL:
                break;
            case IntermediateRepresentation::RETURN:
                break;
            case IntermediateRepresentation::ALLOCA:
                break;
            case IntermediateRepresentation::CMP_EQ:
            case IntermediateRepresentation::CMP_NE:
            case IntermediateRepresentation::CMP_UGE:
            case IntermediateRepresentation::CMP_ULE:
            case IntermediateRepresentation::CMP_SGE:
            case IntermediateRepresentation::CMP_SLE:
            case IntermediateRepresentation::CMP_SGT:
            case IntermediateRepresentation::CMP_SLT:
                break;

            case IntermediateRepresentation::LSH:
            case IntermediateRepresentation::RSH:
            case IntermediateRepresentation::OR:
            case IntermediateRepresentation::AND:
            case IntermediateRepresentation::XOR:
            case IntermediateRepresentation::NOT:
            case IntermediateRepresentation::PHI:
                break;
            default:
                break;
        }

        return ans;
    }
}