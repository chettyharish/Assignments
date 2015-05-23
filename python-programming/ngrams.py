import re


def removeChars(D, O):
    for line in D:
        if(line != "\n"):
            line = re.sub('[^A-Za-z]+', '', line)
            line = line.strip("\n")
            line = line.lower()
            O.write(line)


def ngramsD(D, M):
    for line in D:
        for i in range(3, len(line), 3):
            x = line[i-3:i]
            M[x] = M.get(x, 0) + 1


D = open("ngrams.txt", "r")
O = open("temp1.txt", "w")
D = removeChars(D, O)
D = open("temp1.txt", "r")
M = dict()
ngramsD(D, M)

for i in range(0, 10):
    x = sorted(M, key=M.get, reverse=True)[i]
    print("{0} => {1}".format(x, M.get(x)))
