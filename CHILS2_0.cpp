/* =====================================================================
 * HIGH-LEVEL EXECUTION FLOW (How the code runs from start to finish)
 * =====================================================================
 * 1. Start at main():
 * - The program reads N (coders) and M (conflicts), and builds the adjacency list.
 * - It creates P=4 independent "Solution" objects (teams).
 * * 2. Initialization:
 * - For each of the 4 teams, it calls greedy_init().
 * - greedy_init() shuffles the coders and blindly adds them to the team
 * as long as they don't conflict with anyone already added.
 * * 3. The Main Time Loop:
 * - The program enters a while loop that runs until the 5-minute timer is up.
 * * --- PHASE A: Full Graph Search ---
 * - It calls baseline_local_search() on all 4 teams using the entire graph.
 * Inside baseline_local_search():
 * a) It calls perturb() to violently shake up the team and escape local traps.
 * b) It calls neighborhood_swap() repeatedly to instantly swap out bad coders
 * for better ones using O(1) math.
 * c) It checks the new score. If the shake-up ruined the team, it backtracks.
 * * --- PHASE B: Extract the D-Core ---
 * - The program compares the 4 teams. It ignores the coders that all 4 teams
 * agree on (or all reject). It isolates the coders they *disagree* on into
 * a list called the "d_core".
 * * --- PHASE C: D-Core Targeted Search ---
 * - It calls baseline_local_search() AGAIN, but this time it only passes the
 * d_core list. The algorithm is restricted to only swapping those highly
 * contested coders, saving massive amounts of CPU time.
 * * 4. End of Program:
 * - The timer hits 4m 50s. The while loop breaks.
 * - It grabs the absolute best team out of the 4, formats the 1-based indices,
 * and prints the final score and roster.
 * ===================================================================== */

#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <random>

using namespace std;

// Fast I/O to handle massive input files (200,000 vertices) instantly
static auto _ = []()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    return 0;
}();

// Time management variables
const int TIME_LIMIT_MS = 290000; // 4 minutes 50 seconds
auto start_time = chrono::steady_clock::now();

// Checks if we need to kill the program to print the answer before the platform terminates us
bool is_time_up()
{
    auto now = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - start_time).count();
    return elapsed > TIME_LIMIT_MS;
}

// High-quality random number generator locked to seed 10 for reproducible debugging
mt19937 rng(10); // rng is random number generator

// Represents a single "Team" of coders
struct Solution
{
    vector<int> in_set;                       // 1 if coder is on the team, 0 if not
    long long total_weight;                   // Current total skill rating of the team
    vector<long long> neighbor_weight_in_set; // Sum of the skill ratings of a coder's enemies currently on the team
    vector<int> tight_count;                  // How many enemies a coder currently has on the team
};

// Global Graph Variables
int N, M;
vector<long long> W;     // Skill ratings
vector<vector<int>> adj; // Conflict graph (Adjacency list)

// Adds a coder to the team and updates the tracking arrays for all their enemies
void add_vertex(int u, Solution &sol)
{
    sol.in_set[u] = 1;
    sol.total_weight += W[u];

    // Tell all of u's enemies: "I am on the team now, update your math"
    for (int v : adj[u])
    {
        sol.neighbor_weight_in_set[v] += W[u];
        sol.tight_count[v]++;
    }
}

// Removes a coder from the team and updates the tracking arrays for all their enemies
void remove_vertex(int u, Solution &sol)
{
    sol.in_set[u] = 0;
    sol.total_weight -= W[u];

    // Tell all of u's enemies: "I am off the team, update your math"
    for (int v : adj[u])
    {
        sol.neighbor_weight_in_set[v] -= W[u];
        sol.tight_count[v]--;
    }
}

// Creates a randomized, valid starting point for a team
void greedy_init(Solution &sol)
{
    // Reset all team data
    sol.in_set.assign(N, 0);
    sol.total_weight = 0;
    sol.neighbor_weight_in_set.assign(N, 0);
    sol.tight_count.assign(N, 0);

    // Create a list of all coders [0, 1, 2... N-1] and shuffle them randomly
    vector<int> order(N);
    iota(order.begin(), order.end(), 0);
    shuffle(order.begin(), order.end(), rng);

    // Go down the randomized list. If a coder has 0 enemies currently on the team, add them.
    for (int u : order)
    {
        if (sol.tight_count[u] == 0)
        {
            add_vertex(u, sol);
        }
    }
}

// The core optimization engine: O(1) math check to swap in a high-value coder
bool neighborhood_swap(Solution &sol, const vector<int> &candidate_pool)
{
    bool improved = false;
    for (int u : candidate_pool)
    {
        // If 'u' is NOT on the team, but their skill is higher than ALL their enemies currently on the team combined
        if (!sol.in_set[u] && W[u] > sol.neighbor_weight_in_set[u])
        {

            // Find exactly which enemies are currently blocking 'u'
            vector<int> to_remove;
            for (int v : adj[u])
            {
                if (sol.in_set[v])
                {
                    to_remove.push_back(v);
                }
            }

            // Kick those enemies off the team
            for (int v : to_remove)
                remove_vertex(v, sol);

            // Add 'u' to the team
            add_vertex(u, sol);
            improved = true;
        }
    }
    return improved;
}

// Forces the algorithm to jump out of a "local maximum" trap by breaking things
void perturb(Solution &sol, const vector<int> &active_vertices)
{
    if (active_vertices.empty())
        return;

    // Pick a completely random coder from our active search area
    int u = active_vertices[rng() % active_vertices.size()];

    // If they are on the team,  kick them off
    if (sol.in_set[u])
    {
        remove_vertex(u, sol);
    }
    // If they aren't on the team,  force them on and kick off their enemies
    else
    {
        vector<int> to_remove;
        for (int v : adj[u])
        {
            if (sol.in_set[v])
                to_remove.push_back(v);
        }
        for (int v : to_remove)
            remove_vertex(v, sol);
        add_vertex(u, sol);
    }
}

// The main search loop. It tries to improve a team using a specific pool of candidates (Full Graph or D-Core)
void baseline_local_search(Solution &sol, int iterations, const vector<int> &active_vertices)
{
    vector<int> pool = active_vertices;
    for (int i = 0; i < iterations; ++i)
    {

        // Only check the clock every 128 iterations to save CPU time
        if ((i & 127) == 0 && is_time_up())
            break;

        long long prev_weight = sol.total_weight;
        vector<int> prev_in_set = sol.in_set; // Save a backup of the current team state

        // 1. Perturb (Shake things up)
        int changes = 1 + rng() % 3; // Make 1 to 3 random forced changes
        for (int c = 0; c < changes; c++)
            perturb(sol, active_vertices);

        // 2. Greedy Improve (Climb the hill)
        shuffle(pool.begin(), pool.end(), rng);
        
        while (neighborhood_swap(sol, pool))
        {
            // EMERGENCY STOP: Check the clock during heavy climbing
            if (is_time_up())
            {
                break;
            }
        }; // Keep swapping until no more easy gains exist

        // 3. Backtrack (Did we ruin the team?)
        if (sol.total_weight < prev_weight)
        {
            // Revert changes efficiently by only fixing what broke
            for (int v : active_vertices)
            {
                if (sol.in_set[v] && !prev_in_set[v])
                    remove_vertex(v, sol);
                else if (!sol.in_set[v] && prev_in_set[v])
                    add_vertex(v, sol);
            }
        }
    }
}

int main()
{
    // 1. Read input
    cin >> N >> M;

    W.resize(N);
    for (int i = 0; i < N; ++i)
        cin >> W[i];

    adj.resize(N);
    for (int i = 0; i < M; ++i)
    {
        int u, v;
        cin >> u >> v;
        --u;
        --v; // Convert 1-based hackathon index to 0-based C++ index
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    // Create a pool of every single coder to pass to the full graph search
    vector<int> full_graph(N);
    iota(full_graph.begin(), full_graph.end(), 0);

    // 2. Initialize P=4 concurrent teams
    const int P = 4;
    vector<Solution> pool(P);
    for (int i = 0; i < P; ++i)
    {
        greedy_init(pool[i]);
    }

    // Keep track of the absolute highest score seen across any team at any time
    Solution best_overall = pool[0];

    // 3. The Main Alternating CHILS Loop
    while (!is_time_up())
    {

        // PHASE A: Search the full graph for all 4 teams
        for (int i = 0; i < P; ++i)
        {
            baseline_local_search(pool[i], 100, full_graph);
            if (pool[i].total_weight > best_overall.total_weight)
            {
                best_overall = pool[i]; // Update global high score
            }
            if (is_time_up())
                break;
        }

        if (is_time_up())
            break;

        // PHASE B: Compute D-Core (Find the coders the 4 teams disagree on)
        vector<int> d_core;
        for (int v = 0; v < N; ++v)
        {
            int in_count = 0;
            for (int i = 0; i < P; ++i)
            {
                if (pool[i].in_set[v])
                    in_count++;
            }
            // If the coder is on SOME teams (>0) but not ALL teams (<P), it's a disagreement
            if (in_count > 0 && in_count < P)
            {
                d_core.push_back(v);
            }
        }

        // PHASE C: Search strictly inside the D-Core
        if (d_core.size() > 50)
        {
            for (int i = 0; i < P; ++i)
            {
                // Notice we pass 'd_core' here instead of 'full_graph'
                baseline_local_search(pool[i], 50, d_core);
                if (pool[i].total_weight > best_overall.total_weight)
                {
                    best_overall = pool[i];
                }
            }
        }
        else
        {
            // If D-core is too small, it means the teams all found the exact same solution.
            // We force a massive random shakeup on teams 1-3 (leaving team 0 as a safe anchor).
            for (int i = 1; i < P; ++i)
            {
                for (int k = 0; k < 10; k++)
                    perturb(pool[i], full_graph);
            }
        }
    }

    // 4. Output Formatting
    cout << best_overall.total_weight << "\n";
    vector<int> final_team;
    for (int i = 0; i < N; ++i)
    {
        if (best_overall.in_set[i])
        {
            final_team.push_back(i + 1); // Convert back to 1-based indexing for the judge
        }
    }

    // Print the final roster space-separated
    for (size_t i = 0; i < final_team.size(); ++i)
    {
        cout << final_team[i] << " ";
    }
    cout << "\n";

    return 0;
}