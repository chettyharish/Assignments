from itertools import permutations
n = 8
cols = range(n)
for vec in permutations(cols):
    print("VEC = ",vec)    
    S1 = set([vec[i] + i for i in cols])
    S2 = set([vec[i] - i for i in cols])
    print("S1 = ", S1)
    print("S2 = ", S2)
    if n == len(S1) == len(S2):
        print("VECA = ",vec , "\n\n\n")
    else:
        print("\n\n\n")