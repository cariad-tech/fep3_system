
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_system_state_tree.h"

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <array>
#include <numeric>
#include <vector>

namespace {
struct Vertex {
    Vertex(int i = 0) : index(i)
    {
    }
    bool explored = false;
    int parent = -1;
    int index;
};

} // namespace

namespace fep3 {
/// @brief taken from https://www.geeksforgeeks.org/shortest-path-unweighted-graph/
/// from author https://auth.geeksforgeeks.org/user/ab_gupta/articles
/// with some adaptations
/// Copyright https://www.geeksforgeeks.org/legal/copyright-information/
/// dfs approach to find the transition steps.
std::list<fep3::rpc::ParticipantState> getStateTransitionPath(fep3::rpc::ParticipantState src,
                                                              fep3::rpc::ParticipantState dest)
{
    if (src == dest)
        return {dest};

    static constexpr size_t number_vertices = 7;
    static const std::array<std::vector<int>, number_vertices> adjacency_lists = {
        {{},        // undefined 0
         {},        // unreachable 1
         {1, 3},    // unloaded 2
         {2, 4},    // loaded 3
         {3, 5, 6}, // initialized 4
         {4, 6},    // paused 5
         {4, 5}}};  // running 6

    // a queue to maintain queue of vertices whose
    // adjacency list is to be scanned as per normal
    // DFS algorithm
    std::list<int> queue;

    // stores the
    // information whether ith vertex is reached
    // at least once in the Breadth first search
    std::vector<Vertex> dist(number_vertices);

    // now source is first to be visited and
    // distance from source to itself should be 0
    dist[src].explored = true;
    queue.push_back(src);

    // standard BFS algorithm
    bool path_found = false;
    while (!queue.empty()) {
        int front_node_idx = queue.front();
        queue.pop_front();
        for (size_t neighbor_idx = 0; neighbor_idx < adjacency_lists[front_node_idx].size();
             neighbor_idx++) {
            int neighbor_node = adjacency_lists[front_node_idx][neighbor_idx];
            if (dist[neighbor_node].explored == false) {
                dist[neighbor_node].explored = true;
                dist[neighbor_node].parent = front_node_idx;
                queue.push_back(neighbor_node);

                // We stop BFS when we find
                // destination.
                if (neighbor_node == dest) {
                    path_found = true;
                    break;
                }
            }
        }
    }

    if (path_found) {
        std::list<fep3::rpc::ParticipantState> path;
        fep3::rpc::ParticipantState crawl = dest;
        path.push_back(crawl);
        while (dist[crawl].parent != -1) {
            crawl = static_cast<fep3::rpc::ParticipantState>(dist[crawl].parent);
            path.push_front(crawl);
        }

        return path;
    }
    else
        return {};
}
} // namespace fep3
