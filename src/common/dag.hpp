#include <cassert>

#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thrown by Dag::topological_sort, if the graph is not a DAG.
NOTF_EXCEPTION_TYPE(no_dag_error);

//====================================================================================================================//

/// A Direct Acyclic Graph.
///
/// Useful for topological sorting of nodes that should form a DAG. The class does not operate on the actual nodes, but
/// only of integrals representing an nodes each. This is done for performance and generalizability.
/// The class makes extensive use of vectors, but doesn't give up memory once it aquired it. This way, while the first
/// few times the DAG might still allocate memory, on overage it should not allocate anymore.
///
/// You'll get the most out of it, if you store the Dag close to where it is needed and then re-use it as often as
/// possible. Note however, that the `topological_sort` method is destructive - after calling it, you need to `reinit`
/// the Dag and add new edges.
template<typename T>
class Dag {

    /// Index type used to identify vertices.
    using index_t = T;

    /// Index type must be usable as a vector index.
    static_assert(std::is_integral<index_t>::value && std::is_unsigned<index_t>::value, "DAG index type must be an "
                                                                                        "unsigned integral");

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default constructor.
    /// @param vertex_count     Number of vertices in the graph.
    Dag(const index_t vertex_count = 0) : m_edges(), m_vertex_count(0) { reinit(vertex_count); }

    /// Clears the current DAG and reserves a given number of vertices in the new graph.
    /// Does not deallocate any memory.
    /// @param vertex_count     Number of vertices in the graph.
    void reinit(const index_t vertex_count)
    {
        // clear the old graph
        assert(m_indegrees.size() == m_edges.size());
        assert(m_vertex_count <= m_edges.size());
        for (index_t vertex = 0, end = m_vertex_count; vertex < end; ++vertex) {
            m_edges[vertex].clear();
            m_indegrees[vertex] = 0;
        }
        m_result.clear();
        m_free_vertices.clear();

        // make space for the new
        if (vertex_count > m_vertex_count) {
            m_edges.resize(vertex_count, {});
            m_indegrees.resize(vertex_count, 0);
        }
        m_result.reserve(vertex_count);

        m_vertex_count = vertex_count;
    }

    /// Adds a new edge to the graph.
    /// @param origin           Origin vertex.
    /// @param target           Target vertex.
    /// @throws out_of_range    If the `origin` or `target` vertex index is larger than the largest index in the graph.
    void add_edge(const index_t origin, const index_t target)
    {
        if (origin >= m_vertex_count || target >= m_vertex_count) {
            notf_throw(out_of_bounds, "Vertex index is larger than the largest index in the graph");
        }
        m_edges[origin].push_back(target);
        ++m_indegrees[target];
    }

    /// Performs a topological sort on the given graph.
    /// Note that this method is destructive! It will return the result (unless a cyclic dependecy was detected), but
    /// it will have modified the internal state of the Dag, so you need to reconstruct it before calling this method
    /// again.
    /// @throws no_dag_error    If a cyclic dependency was detected during sort.
    /// @returns                A topologically sorted sequence of vertices in the graph.
    const std::vector<index_t>& topological_sort()
    {
        // enqueue all vertices with zero indegree
        for (index_t vertex = 0; vertex < m_vertex_count; ++vertex) {
            if (m_indegrees[vertex] == 0) {
                m_free_vertices.push_back(vertex);
            }
        }

        index_t visited_count = 0;
        while (!m_free_vertices.empty()) {

            // take a free vertex and append it to the solution
            const index_t vertex = m_free_vertices.back();
            m_free_vertices.pop_back();
            m_result.push_back(vertex);

            // remove the removed vertex as indegree of all of its children
            for (const index_t& outdegree : m_edges[vertex]) {
                if (--m_indegrees[outdegree] == 0) {

                    // enqueue all newly freed children
                    m_free_vertices.push_back(outdegree);
                }
            }
            ++visited_count;
        }

        // check if there is a cycle in the graph
        if (visited_count != m_vertex_count) {
            notf_throw(no_dag_error, "Caught cyclic dependency during topological sort");
        }

        return m_result;
    }

    /// Returns the result of the last topological sort.
    /// Will be invalid, if `reinit` has been called since.
    const std::vector<index_t>& last_result() const { return m_result; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The index of the outer vector is the origin of the edge, entries in the inner vector are edge targets.
    std::vector<std::vector<index_t>> m_edges;

    /// Number of edges connected into each vertex.
    std::vector<index_t> m_indegrees;

    /// Helper variable for `topological_sort`.
    std::vector<index_t> m_free_vertices;

    /// Holds the result of a `topological_sort`.
    std::vector<index_t> m_result;

    /// Number of vertices in the graph.
    /// This is separate variable so that we never clear the outer vector in `m_edges`, only the inner vectors.
    index_t m_vertex_count;
};

NOTF_CLOSE_NAMESPACE
