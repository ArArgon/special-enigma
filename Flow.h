//
// Created by 廖治平 on 7/19/21.
//

#ifndef SYSYBACKEND_FLOW_H
#define SYSYBACKEND_FLOW_H

#include "Utilities.h"
#include <map>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <utility>

namespace Backend::Flow {
    class BasicBlock {
    public:
        using var_ptr_t = std::shared_ptr<IntermediateRepresentation::IROperand>;
        using varSet = std::set<IntermediateRepresentation::IROperand>;
        struct BBStatement {
            std::shared_ptr<IntermediateRepresentation::Statement> statement;
            varSet def, use, live;
        };
    private:
        varSet def, use;

        static BBStatement procRawStatement(IntermediateRepresentation::Statement& stmt);

    public:
        std::vector<BBStatement> statements;
        varSet liveIn, liveOut;

        BasicBlock() = default;

        explicit BasicBlock(IntermediateRepresentation::IRSequence& sequence) {
            for (auto& ins : sequence)
                appendStatement(ins);
        }

        void appendStatement(IntermediateRepresentation::Statement& statement) {
            BBStatement tmpStmt = procRawStatement(statement);
            for (auto& var : tmpStmt.use) {
                if (!def.count(var)) {
                    // this variable is used and not reassigned in this BB before.
                    use.insert(var);
                }
            }
            def.insert(tmpStmt.def.begin(), tmpStmt.def.end());
            statements.push_back(tmpStmt);
        }

        const varSet &getDef() const {
            return def;
        }

        const varSet &getUse() const {
            return use;
        }

        const varSet &getLiveIn() const {
            return liveIn;
        }

        const varSet &getLiveOut() const {
            return liveOut;
        }

        void setLiveIn(const varSet &liveIn) {
            BasicBlock::liveIn = liveIn;
        }

        void setLiveOut(const varSet &liveOut) {
            BasicBlock::liveOut = liveOut;
        }

        const std::vector<BBStatement> &getStatements() const {
            return statements;
        }
    };

    using bb_ptr_t = std::shared_ptr<BasicBlock>;
    using ControlFlowGraph = Util::Graph<bb_ptr_t>;

    class Flow {
        std::shared_ptr<IntermediateRepresentation::Function> sourceFunc;
        std::vector<bb_ptr_t> basicBlocks;
        ControlFlowGraph cfg;

        void analyzeBasicBlocks();
        void analyzeLiveness();

    public:

        explicit Flow(IntermediateRepresentation::Function& sourceFunc) : sourceFunc(&sourceFunc) {
            analyzeBasicBlocks();
            analyzeLiveness();
        }

        const std::vector<bb_ptr_t> &getBasicBlocks() const {
            return basicBlocks;
        }

        const ControlFlowGraph &getCfg() const {
            return cfg;
        }

    };
}


#endif //SYSYBACKEND_FLOW_H
