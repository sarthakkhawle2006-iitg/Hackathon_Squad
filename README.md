# Hackathon Squad — Maximum Weight Independent Set

Given N coders each with a skill rating and M conflict pairs between them, assemble the highest-scoring squad where no two members conflict with each other.

This is the **Maximum Weight Independent Set (MWIS)** problem on a general graph — NP-Hard on general graphs, meaning no known polynomial-time exact algorithm exists. This repo contains two approaches: a recursive brute-force for small inputs and a high-performance metaheuristic solver (CHILS) built to handle up to 200,000 nodes within a 5-minute time limit.

---
