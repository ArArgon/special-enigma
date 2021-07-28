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

namespace Backend::Util {

    template<class NodeType>
    class Graph {
        static constexpr size_t defaultSize = 200;

        size_t cnt = 0;
        std::vector<std::unordered_set<size_t>> G;
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

        Graph(const std::vector<NodeType>& nodes) {
            G.reserve(nodes.size());
            for (auto& node : nodes)
                newNode(node);
        }

        bool containsEdge(const NodeType& a, const NodeType& b);

        void newNode(const NodeType& node);

        bool containsNode(const NodeType& node);

        void addEdge(const NodeType& a, const NodeType& b);

        void addBiEdge(const NodeType& a, const NodeType& b);

        size_t getNodeDegree(const NodeType& node) const;

        void setNodeDegree(const NodeType &node, size_t value);

        std::set<NodeType> getNeighbours(const NodeType& node);

        const std::unordered_set<NodeType> &getNodes() const;

        std::unordered_set<NodeType> getPrecursorsOf(const NodeType& node) const;

        std::unordered_set<NodeType> getSuccessorsOf(const NodeType& node) const;

        template<class T>
        void doFunc(const T& func) {
            func(this);
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
            return fa[u] == u ? u : fa[u] = find(fa[u]);
        }

    public:
        NodeType find(const NodeType& u) {
            int id = nodeToId[u];
            return idToNode[idFind(id)];
        }

        int disjoint(const NodeType& u, const NodeType& v) {
            int idu = find(u), idv = find(v);
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

        DisjointSet(const std::set<NodeType>& nodes) {
            for (auto& node : nodes)
                addNode(node);
        }

        DisjointSet(const std::unordered_set<NodeType>& nodes) {
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
        size_t allocate(const var_t& opr, size_t size = 4) {
            if (size & 0b11) {
                // not 4-align
                return -1;
            }
            size_t pos = currentStackSize;
            inStackVariables.insert(opr);
            variablePosition[opr] = pos;
            currentStackSize += size;
            return pos;
        }

        size_t getVariablePosition(const var_t& opr) const {
            return inStackVariables.count(opr) ? variablePosition.at(opr) : -1;
        }

        bool isInStack(const var_t& opr) const {
            return inStackVariables.count(opr) > 0;
        }

        size_t getStackSize() const {
            return currentStackSize;
        }
    };

    template<class Set>
    inline auto set_diff(const Set& a, const Set& b) {
        // return a - b;
        Set ans;
        std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ans, ans.begin()));
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
