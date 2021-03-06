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
#include <exception>
#include <memory>
#include <utility>

namespace Backend::Flow {
    class BasicBlock {
    public:
        using varSet = std::set<IntermediateRepresentation::IROperand>;
        struct BBStatement {
            IntermediateRepresentation::Statement* statement;
            varSet def, use, live;
            bool init = false;

            void replaceUse(const IntermediateRepresentation::IROperand& oldVar, const IntermediateRepresentation::IROperand& newVar);
            void replaceDef(const IntermediateRepresentation::IROperand& oldVar, const IntermediateRepresentation::IROperand& newVar);
        };
    private:
        varSet def, use;

    public:
        std::vector<BBStatement> statements;
        varSet liveIn, liveOut;

        static BBStatement procRawStatement(IntermediateRepresentation::Statement& stmt);

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
        IntermediateRepresentation::Function* sourceFunc;
        std::vector<bb_ptr_t> basicBlocks;
        ControlFlowGraph cfg;

        void analyzeBasicBlocks();
        void analyzeLiveness();

    public:

        explicit Flow(IntermediateRepresentation::Function* sourceFunc) : sourceFunc(std::move(sourceFunc)) {
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
