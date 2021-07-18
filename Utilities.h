//
// Created by 廖治平 on 7/18/21.
//

#ifndef SYSYBACKEND_UTILITIES_H
#define SYSYBACKEND_UTILITIES_H

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <map>
#include <set>
#include "IRTypes.h"

namespace Backend::Util {
    class BasicBlock {
    public:
        using varSet = std::set<IntermediateRepresentation::IROperand>;
        struct BBStatement {
            IntermediateRepresentation::Statement statement;
            varSet def, use, live;
        };
    private:
        varSet def, use;

        static BBStatement procRawStatement(const IntermediateRepresentation::Statement& stmt);

    public:
        std::vector<BBStatement> statements;
        varSet liveIn, liveOut;

        BasicBlock() = default;

        explicit BasicBlock(const IntermediateRepresentation::IRSequence& sequence) {
            for (auto& ins : sequence)
                appendStatement(ins);
        }

        void appendStatement(const IntermediateRepresentation::Statement& statement) {
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

    template<class NodeType>
    class Graph {
        static constexpr size_t defaultSize = 200;

        size_t cnt = 0;
        std::vector<std::unordered_set<size_t>> G;
        std::unordered_map<NodeType, int> nodeToId;
        std::unordered_map<int, NodeType> idToNode;
        std::unordered_map<size_t, int> degree;

    public:
        Graph() {
            G.reserve(defaultSize);
        };

        Graph(const std::vector<NodeType>& nodes) {
            G.reserve(nodes.size());
            for (auto& node : nodes)
                newNode(node);
        }

        void newNode(const NodeType& node);

        bool containsNode(const NodeType& node);

        void addEdge(const NodeType& a, const NodeType& b);

        void addBiEdge(const NodeType& a, const NodeType& b);

        size_t getNodeDegree(const NodeType& node) const;

        std::set<NodeType> getSuccessor(const NodeType& node);
    };
}

#endif //SYSYBACKEND_UTILITIES_H
