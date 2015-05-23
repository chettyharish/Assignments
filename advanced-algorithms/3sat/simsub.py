import os
import itertools

if __name__ == "__main__":
    
    all = []
    counter = 0
    ct2dict = dict()
    ct3dict = dict()
    ct4dict = dict()
    ct5dict = dict()
    ct6dict = dict()
    ct7dict = dict()
    ct8dict = dict()
    ct9dict = dict()
    ct10dict = dict()
    
    for path, dirs, files in os.walk(os.path.join(os.getcwd(), "Temp")):#/home/chettyharish/workspace/temporary/Temp"):
        for file in files:
            f = open(os.path.join(path,file) , 'r')
            for line in f:
                line = line.split()
                if line[0] == 'c' or line[0] == 'p':
                    continue
                all.append(set([line[0],line[1],line[2]]))
#             print(len(all))

            last = all[-1]
            lenall = len(all)
            N = 50
            pow = 0
            
            for i in range(lenall):
                last = all[i]
                for j in range(i - 1):
                    if len(all[j] | last) == 4:
                        pow += 2**(N - 4)
                    elif len(all[j] | last) == 5:
                        pow += 2**(N - 5)
                    elif len(all[j] | last) == 6:
                        pow += 2**(N - 6)
            print(file)
            print(lenall*2**(N-3))
            print(pow)





            print()