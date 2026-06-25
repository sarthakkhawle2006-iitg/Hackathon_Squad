# Hackathon Squad — Maximum Weight Independent Set

Given N coders each with a skill rating and M conflict pairs between them, assemble the highest-scoring squad where no two members conflict with each other.

This is the **Maximum Weight Independent Set (MWIS)** problem on a general graph NP-Hard on general graphs, meaning no known polynomial-time exact algorithm exists. This repo contains two approaches: a recursive brute-force for small inputs and a high-performance metaheuristic solver (CHILS) built to handle up to 200,000 nodes within a 5-minute time limit.

---

## Problem Format

**Input**
```
N M
w1 w2 ... wN
u1 v1
u2 v2
...
```

**Output**
```
<maximum total skill rating>
<space-separated list of selected coder indices>
```

**Example**
```
5 3
10 20 30 40 50
1 2
2 3
4 5
```
Output:
```
80
1 3 4
```
Coders 1, 3, and 4 share no conflicts. Their combined skill (10 + 30 + 40 = 80) is the maximum achievable.

---

## Repository Structure

```
.
├── BruteForce.cpp     # Recursive backtracking — exact, exponential time
├── CHILS2_0.cpp       # CHILS metaheuristic — near-optimal, runs within 5 minutes
└── README.md
```

---

## Approach 1 — Brute Force (Recursive Backtracking)

**File:** `BruteForce.cpp`

### Idea

The brute-force explores every possible subset of coders via recursion with backtracking. At each coder, exactly two choices are made:

1. **Skip** — move to the next coder without adding them.
2. **Include** — only if none of their conflict partners are already selected. Add them, recurse deeper, then backtrack by removing them.

When all coders have been processed (base case), the current squad's total is compared against the running best.

```
find_best_squad(coder, current_sum, squad, is_selected):
    if coder > N:
        update best if current_sum is higher
        return

    // Choice 1: Skip
    find_best_squad(coder + 1, current_sum, squad, is_selected)

    // Choice 2: Include (only if no active neighbour conflicts)
    if no selected neighbour conflicts:
        mark coder as selected
        find_best_squad(coder + 1, current_sum + skill[coder], squad, is_selected)
        unmark coder    // backtrack
```

### Complexity

- Time: O(2^N) — exponential
- Space: O(N) recursion stack

Exact and correct, but only feasible for N up to roughly 25. Purely for understanding the problem structure.

---

## Approach 2 — CHILS Metaheuristic

**File:** `CHILS2_0.cpp`

CHILS (Cooperative Heuristic Iterated Local Search) runs 4 independent solutions in parallel and continuously improves them over the entire time budget (~4 minutes 50 seconds).

### Execution Flow

```
main()
 ├── Read graph (N coders, M conflicts)
 ├── Initialize 4 solutions via greedy_init()
 └── while (time < 4m 50s):
      ├── PHASE A — Full Graph Search
      │    └── baseline_local_search() on all 4 teams
      │         ├── perturb()            // forced changes to escape local traps
      │         ├── neighborhood_swap()  // O(1) greedy improvement
      │         └── backtrack()          // revert if score dropped
      │
      ├── PHASE B — Compute D-Core
      │    └── Isolate coders the 4 teams disagree on
      │
      └── PHASE C — D-Core Targeted Search
           ├── if d_core is large: restrict local search to d_core only
           └── if d_core is tiny:  force heavy perturbation to diversify
```

### Per-Solution Data Structures

Each `Solution` tracks three arrays, updated in O(degree) time on every add/remove:

| Array | Meaning |
|---|---|
| `in_set[u]` | 1 if coder u is currently on the team |
| `neighbor_weight_in_set[u]` | Total skill of u's conflict partners currently on the team |
| `tight_count[u]` | Count of u's conflict partners currently on the team |

### Key Functions

**`greedy_init()`**
Shuffles all coders randomly, then greedily adds each one if they have zero conflict partners already on the team. Produces a valid conflict-free starting point with randomness to differentiate the 4 parallel solutions.

**`neighborhood_swap()`**
For every coder `u` not on the team: if `W[u] > neighbor_weight_in_set[u]`, it is profitable to evict all of u's current enemies and add u in their place. This check is O(1) because the neighbor weight sum is maintained incrementally. The function loops until no such profitable swap exists.

**`perturb()`**
Picks a random coder from the active pool and forcibly flips their membership. If they are on the team, remove them. If they are off, force them on and evict whoever conflicts. This breaks out of local maxima.

**`baseline_local_search()`**
The main loop: perturb → neighborhood_swap → backtrack. If the perturbation leads to a worse score than before, the saved state is restored. Runs for a fixed number of iterations or until time is up.

**D-Core**
After each full-graph phase, the 4 solutions are compared. A coder is in the D-Core if they appear in *some but not all* solutions — the algorithm's uncertainty region. Restricting the next search pass to only these coders avoids re-scanning all N vertices and focuses compute exactly where the solutions diverge.

### Complexity

| | |
|---|---|
| Per swap check | O(1) amortized |
| Per add/remove | O(degree of vertex) |
| Total runtime | Bounded by 4m 50s wall clock |
| Space | O(N + M) |

### Build Flags

```cpp
#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")
```

AVX2 SIMD and loop unrolling are enabled to maximize throughput on the judge's hardware.

---

## Building and Running

```bash
# Brute Force
g++ -O2 -o brute BruteForce.cpp
echo "5 3
10 20 30 40 50
1 2
2 3
4 5" | ./brute

# CHILS
g++ -O3 -march=native -o chils CHILS2_0.cpp
./chils < input.txt
```

---

## Comparison

| | Brute Force | CHILS |
|---|---|---|
| Correctness | Always exact | Near-optimal (heuristic) |
| Time complexity | O(2^N) | Bounded by wall clock |
| Suitable N | up to ~25 | up to 200,000 |
| Strategy | Exhaustive recursion + backtrack | Iterated local search + D-Core |

---

## Why not an exact solver for large inputs?

MWIS is NP-Hard on general graphs. For large N, exact branch and bound or ILP solvers become intractable. CHILS sidesteps this by using the time budget as the resource. The longer it runs, the better the solution gets, trading provable optimality for practical performance within the contest's time limit.
