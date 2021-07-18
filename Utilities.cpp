//
// Created by 廖治平 on 7/18/21.
//

#include "Utilities.h"

namespace Backend::Util {
    template<class NodeType>
    void Graph<NodeType>::addEdge(const NodeType& a, const NodeType& b)  {
        if (!containsNode(a))
            newNode(a);
        if (!containsNode(b))
            newNode(b);
        size_t id_a = nodeToId[a], id_b = nodeToId[b];
        G[id_a].insert(id_b);
    }

    template<class NodeType>
    void Graph<NodeType>::addBiEdge(const NodeType &a, const NodeType &b)  {
        addEdge(a, b);
        addEdge(b, a);
    }

    template<class NodeType>
    std::set<NodeType> Graph<NodeType>::getSuccessor(const NodeType &node) {
        std::set<NodeType> ans;
        size_t id = nodeToId[node];

        for (auto& suc : G[id])
            ans.insert(idToNode[suc]);

        return ans;
    }

    template<class NodeType>
    size_t Graph<NodeType>::getNodeDegree(const NodeType &node) const {
        return degree.at(nodeToId.at(node));
    }

    template<class NodeType>
    bool Graph<NodeType>::containsNode(const NodeType &node) {
        return nodeToId.count(node) != 0;
    }

    template<class NodeType>
    void Graph<NodeType>::newNode(const NodeType &node) {
        if (G.size() > cnt)
            G.resize(2 * G.size());
        size_t id = ++cnt;
        nodeToId[id] = node;
        idToNode[node] = id;
        degree[id] = 0;
    }

    BasicBlock::BBStatement BasicBlock::procRawStatement(
            const IntermediateRepresentation::Statement &stmt) {
        BasicBlock::BBStatement ans;
        ans.statement = stmt;

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
