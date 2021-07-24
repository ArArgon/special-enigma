//
// Created by 廖治平 on 7/18/21.
//

#include "Utilities.h"

namespace Backend::Util {
    template<class NodeType>
    void Graph<NodeType>::addEdge(const NodeType& a, const NodeType& b)  {
        // a -> b
        if (!containsNode(a))
            newNode(a);
        if (!containsNode(b))
            newNode(b);
        int id_a = nodeToId[a], id_b = nodeToId[b];
        adjSet.template emplace(a, b);
        degree[a]++;
        G[id_a].insert(id_b);
        precursors[id_b].insert(id_a);
        successors[id_a].insert(id_b);
    }

    template<class NodeType>
    void Graph<NodeType>::addBiEdge(const NodeType &a, const NodeType &b)  {
        addEdge(a, b);
        addEdge(b, a);
    }

    template<class NodeType>
    std::set<NodeType> Graph<NodeType>::getNeighbours(const NodeType &node) {
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
        nodes.insert(node);
    }

    template<class NodeType>
    void Graph<NodeType>::setNodeDegree(const NodeType &node, size_t value) {
        degree[node] = value;
    }

    template<class NodeType>
    const std::unordered_set<NodeType> &Graph<NodeType>::getNodes() const {
        return nodes;
    }

    template<class NodeType>
    bool Graph<NodeType>::containsEdge(const NodeType &a, const NodeType &b) {
        return adjSet.template count({ a, b });
    }

    template<class NodeType>
    std::unordered_set<NodeType> Graph<NodeType>::getPrecursorsOf(const NodeType &node) const {
        std::unordered_set<NodeType> ans;
        int id_n = nodeToId.at(node);
        auto& n_pre = successors.at(id_n);
        for (auto& pre : n_pre)
            ans.insert(idToNode.at(pre));
        return ans;
    }

    template<class NodeType>
    std::unordered_set<NodeType> Graph<NodeType>::getSuccessorsOf(const NodeType &node) const {
        std::unordered_set<NodeType> ans;
        int id_n = nodeToId.at(node);
        auto& n_suc = successors.at(id_n);
        for (auto& suc : n_suc)
            ans.insert(idToNode.at(suc));
        return ans;
    }
}
