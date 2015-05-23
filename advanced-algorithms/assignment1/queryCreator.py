import random
L = list(str(x) for x in range(32))
fp = open("query.txt", "w")
for i in range(10):
    random.shuffle(L)
    fp.write(",".join(L)+"\n")
