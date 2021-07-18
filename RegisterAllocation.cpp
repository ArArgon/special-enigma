//
// Created by 廖治平 on 7/18/21.
//

#include <cassert>
#include "RegisterAllocation.h"

template<size_t registerCount>
void Backend::RegisterAllocation::ColourAllocator<registerCount>::analyzeBasicBlocks() {
    bb_ptr_t currentBlock = std::make_shared<BasicBlock>(BasicBlock());
    std::unordered_map<std::string, bb_ptr_t> label_map;

    // divide basic blocks
    auto newBB = [&] (bb_ptr_t& block, bool createAnother = true) {
        basicBlocks.push_back(block);
        cfg.newNode(block);
        if (createAnother)
            block = std::make_shared<BasicBlock>(BasicBlock());
    };
    const auto& seq = baseType::sourceFunc.getStatements();
    for (const IntermediateRepresentation::Statement& ins : seq) {
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

    // build CFG
    for (const auto& bb : basicBlocks) {
        // auto& stmts = bb->getStatements();
        const std::vector<typename BasicBlock::BBStatement>& stmts = bb->getStatements();
        if (stmts.empty())
            continue;
        const IntermediateRepresentation::Statement& stmt = (stmts.end() - 1)->statement;
        if (stmt.getStmtType() == IntermediateRepresentation::BR) {
            auto& ops = stmt.getOps();
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

template<size_t registerCount>
void Backend::RegisterAllocation::ColourAllocator<registerCount>::analyzeLiveness() {
    // calc live sets for basic blocks
    using varSet = std::set<IntermediateRepresentation::IROperand>;

    auto set_diff = [] (const varSet& a, const varSet& b) {
        // return a - b;
        varSet ans;
        std::set_difference(a.begin(), a.end(), b.begin(), b.end(), ans.begin());
        return ans;
    };

    bool change = true;

    while (change) {
        change = false;
        for (auto& bb : basicBlocks) {
            varSet tmpLiveIn = bb->getUse();
            auto&& successors = cfg.getSuccessor(bb);

            for (const bb_ptr_t& suc : successors) {
                // LiveOut[bb] U= LiveIn[suc]
                auto& sucLiveIn = suc->getLiveIn();
                bb->liveOut.insert(sucLiveIn.begin(), sucLiveIn.end());
            }

            // LiveIn[bb] = use[bb] U ( LiveOut[bb] - def[bb] )
            tmpLiveIn += set_diff(bb->getLiveOut(), bb->getDef());
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
        stmts[size - 1].live = stmts[size - 1].use + set_diff(bb->getLiveOut(), stmts[size - 1].def);
        for (int i = size - 2; i >= 0; i--) {
            auto &cur = stmts[i], &last = stmts[i - 1];
            cur.live = set_diff(last.live, cur.def) + cur.use;
        }
    }
}

template<size_t registerCount>
void Backend::RegisterAllocation::ColourAllocator<registerCount>::coalesce() {

}
