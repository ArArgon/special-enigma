//
// Created by 廖治平 on 7/18/21.
//

#ifndef SYSYBACKEND_UTILITIES_H
#define SYSYBACKEND_UTILITIES_H

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <type_traits>
#include <map>
#include <set>
#include "IRTypes.h"

template<>
class std::hash<IntermediateRepresentation::IROperand> {
public:
    size_t operator() (const IntermediateRepresentation::IROperand& operand) const {
        return std::hash<std::string> { }(operand.toString());
    }
};

template<>
class std::hash<std::pair<IntermediateRepresentation::IROperand, IntermediateRepresentation::IROperand>> {
public:
    size_t operator() (const std::pair<IntermediateRepresentation::IROperand, IntermediateRepresentation::IROperand>& operand) const {
        return std::hash<std::string> { }(operand.first.toString() + "#" + operand.second.toString());
    }
};

namespace Backend::Util {

    template<class NodeType>
    class Graph {
        static constexpr size_t defaultSize = 200;

        size_t cnt = 0;
        std::vector<std::vector<size_t>*> G;
        std::unordered_set<NodeType> nodes;
        std::unordered_map<NodeType, int> nodeToId;
        std::unordered_map<int, NodeType> idToNode;
        std::unordered_map<int, std::unordered_set<int>> precursors, successors;
        std::unordered_map<size_t, int> degree;
        std::set<std::pair<NodeType, NodeType>> adjSet;

    public:
        Graph() {
            G.reserve(defaultSize);
        };

        explicit Graph(const std::vector<NodeType>& nodes) {
            G.reserve(nodes.size());
            for (auto& node : nodes)
                newNode(node);
        }

        bool containsEdge(const NodeType& a, const NodeType& b) {
            return adjSet.count({ a, b });
        }

        void newNode(const NodeType& node) {
            if (G.size() > cnt)
                G.resize(2 * G.size());
            size_t id = ++cnt;
            G[id] = new std::vector<size_t>;
            nodeToId[node] = id;
            idToNode[id] = node;
            degree[id] = 0;
            nodes.insert(node);
        }

        bool containsNode(const NodeType& node) {
            return nodeToId.count(node) != 0;
        }

        void insertAdj(const NodeType& a, const NodeType& b) {
            adjSet.template emplace(a, b);
        }

        void addEdge(const NodeType& a, const NodeType& b) {
            // a -> b
            if (!containsNode(a))
                newNode(a);
            if (!containsNode(b))
                newNode(b);
            int id_a = nodeToId[a], id_b = nodeToId[b];
            if (id_a == id_b)
                return;
            if (!containsEdge(a, b))
                adjSet.emplace(a, b);
            degree[id_a]++;
            G[id_a]->push_back(id_b);
            precursors[id_b].insert(id_a);
            successors[id_a].insert(id_b);
        }

        void addBiEdge(const NodeType& a, const NodeType& b) {
            addEdge(a, b);
            addEdge(b, a);
        }

        size_t getNodeDegree(const NodeType& node) const {
            return degree.at(nodeToId.at(node));
        }

        void setNodeDegree(const NodeType &node, size_t value) {
            degree[nodeToId.at(node)] = value;
        }

        std::unordered_set<NodeType> getNeighbours(const NodeType& node) {
            std::unordered_set<NodeType> ans;
            size_t id = nodeToId[node];

            for (auto& suc : *G[id])
                ans.insert(idToNode[suc]);

            return ans;
        }

        const std::unordered_set<NodeType> &getNodes() const {
            return nodes;
        }

        std::unordered_set<NodeType> getPrecursorsOf(const NodeType& node) const {
            std::unordered_set<NodeType> ans;
            int id_n = nodeToId.at(node);
            if (precursors.count(id_n)) {
                auto& n_pre = precursors.at(id_n);
                for (auto& pre : n_pre)
                    ans.insert(idToNode.at(pre));
            }
            return ans;
        }

        std::unordered_set<NodeType> getSuccessorsOf(const NodeType& node) const {
            std::unordered_set<NodeType> ans;
            int id_n = nodeToId.at(node);
            if (successors.count(id_n)) {
                auto& n_suc = successors.at(id_n);
                for (auto& suc : n_suc)
                    ans.insert(idToNode.at(suc));
            }
            return ans;
        }

        template<class T>
        void doFunc(const T& func) {
            func(this);
        }

        ~Graph() {
            for (auto& node : G)
                delete node;
        }
    };

    template<class NodeType, bool enableDepth = false>
    class DisjointSet {
        int cnt = 0;
        std::unordered_map<NodeType, int> nodeToId;
        std::unordered_map<int, NodeType> idToNode;
        std::unordered_map<int, int> depth;
        std::unordered_map<int, int> fa;

        int idFind(int u) {
            return fa[u] == u ? u : fa[u] = idFind(fa[u]);
        }

    public:
        NodeType find(const NodeType& u) {
            int id = nodeToId[u];
            return idToNode[idFind(id)];
        }

        int disjoint(const NodeType& u, const NodeType& v) {
            int idu = idFind(nodeToId[u]), idv = idFind(nodeToId[v]);
            if (idu != idv) {
                if (enableDepth) {
                    if (depth[idu] > depth[idv]) {
                        fa[idu] = idv;
                        return 1;
                    } else {
                        if (depth[idu] == depth[idv])
                            depth[idu]++;
                        fa[idv] = idu;
                        return 2;
                    }
                } else {
                    fa[idu] = idv;
                    return 1;
                }
            }
            return 0;
        }

        void addNode(const NodeType& u) {
            if (contains(u))
                return;
            int id = ++cnt;
            nodeToId[u] = id;
            idToNode[id] = u;
            fa[id] = id;
            if (enableDepth)
                depth[id] = 1;
        }

        bool contains(const NodeType& u) {
            return nodeToId.count(u) != 0;
        }

        DisjointSet() = default;

        explicit DisjointSet(const std::set<NodeType>& nodes) {
            for (auto& node : nodes)
                addNode(node);
        }

        explicit DisjointSet(const std::unordered_set<NodeType>& nodes) {
            for (auto& node : nodes)
                addNode(node);
        }
    };

    class StackScheme {
    private:
        using var_t = IntermediateRepresentation::IROperand;

        size_t currentStackSize;    // Bytes
        std::unordered_set<var_t> inStackVariables;
        std::unordered_map<var_t, size_t> variablePosition; // Also bytes
    public:
        size_t allocate(const var_t& opr, size_t size = 4);

        size_t getVariablePosition(const var_t& opr) const;

        bool isInStack(const var_t& opr) const;

        size_t getStackSize() const;

        std::map<std::string, size_t> getStackBrief() const;
    };

    template<class Set>
    inline auto set_diff(const Set& a, const Set& b) {
        // return a - b;
        Set ans = a;
        for (auto& var : b)
            if (ans.count(var))
                ans.erase(var);
        // std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ans, ans.begin()));
        return ans;
    }

    template<class Set>
    inline auto set_union_to(Set& target, const Set& b) {
        target.insert(b.begin(), b.end());
    }

    template<class Set>
    inline auto set_union(const Set& a, const Set& b) {
        Set ans = a;
        ans.insert(b.begin(), b.end());
        return ans;
    }

}

#endif //SYSYBACKEND_UTILITIES_H
