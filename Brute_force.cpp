#include <bits/stdc++.h>

using namespace std;

class Graph
{
public:
    int n; // number of coders
    int m; // number of conflicts
    vector<long long> skill_ratings;
    vector<vector<int>> adj_list;

    long long max_skill_sum;
    vector<int> best_squad;

    // recursively explore all possibilities and return the best one
    void find_best_squad(int current_coder, long long current_sum, vector<int> &current_squad, vector<bool> &is_selected)
    {
        // Base case : when we checked all coders then check if this particular case could be better than the max so far
        if (current_coder > n)
        {
            // Update the maximum score and best team if we found a better one
            if (current_sum > max_skill_sum)
            {
                max_skill_sum = current_sum;
                best_squad = current_squad;
            }
            return;
        }

        // case 1 : not take
        find_best_squad(current_coder + 1, current_sum, current_squad, is_selected);

        // case 2 : take
        bool can_include = true;
        for (int rival : adj_list[current_coder])
        {
            if (is_selected[rival])
            {
                can_include = false; // A conflict is already on the team
                break;
            }
        }

        // no conflicts then we will try out with possibilities including this coder
        if (can_include)
        {
            is_selected[current_coder] = true;
            current_squad.push_back(current_coder);

            // Recurse to the next coder with the updated sum
            find_best_squad(current_coder + 1, current_sum + skill_ratings[current_coder], current_squad, is_selected);

            // Backtracking :
            is_selected[current_coder] = false;
            current_squad.pop_back();
        }
    }

    Graph(int node, int edge)
    {
        n = node;
        m = edge;
        // using the 1-based indexing
        skill_ratings.resize(n + 1, 0);
        adj_list.resize(n + 1);
        max_skill_sum = -1;
    }

    void input()
    {
        for (int i = 1; i <= n; i++) // because of 1 based indexing
        {
            cin >> skill_ratings[i];
        }

        for (int i = 0; i < m; i++)
        {
            int u, v;
            cin >> u >> v;
            // for undirected graph :
            adj_list[u].push_back(v);
            adj_list[v].push_back(u);
        }
    }

    void solve()
    {
        vector<int> current_squad;
        vector<bool> is_selected(n + 1, false);

        find_best_squad(1, 0, current_squad, is_selected);
    }

    void print_result()
    {
        cout << max_skill_sum << "\n";
        for (int i = 0; i < best_squad.size(); ++i)
        {
            cout << best_squad[i] << " ";
        }
        cout << "\n";
    }
};

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n, m;
    cin >> n >> m;
    Graph g(n, m);
    g.input();
    g.solve();
    g.print_result();

    return 0;
}
