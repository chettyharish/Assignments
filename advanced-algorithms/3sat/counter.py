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
            
            for ele in itertools.permutations(all, 2):
                newset = ele[0] | ele[1]
                ct2dict[len(newset)] = ct2dict.get(len(newset), 0) + 1
            
            total = sum(ct2dict.values())
            print(file +"\t"+ str(sorted([(x , ct2dict[x]*100.0/total) for x in ct2dict])))
            
            
            for ele in itertools.permutations(all, 3):
                newset = ele[0] | ele[1] | ele[2]
                ct3dict[len(newset)] = ct3dict.get(len(newset), 0) + 1
            
            total = sum(ct3dict.values())
            print(file +"\t"+ str(sorted([(x , ct3dict[x]*100.0/total) for x in ct3dict])))
            
            for ele in itertools.permutations(all, 4):
                newset = ele[0] | ele[1] | ele[2] | ele[3]
                ct4dict[len(newset)] = ct4dict.get(len(newset), 0) + 1
             
            total = sum(ct4dict.values())
            print(file +"\t"+ str(sorted([(x , ct4dict[x]*100.0/total) for x in ct4dict])))
            
            
            for ele in itertools.permutations(all, 5):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]
                ct5dict[len(newset)] = ct5dict.get(len(newset), 0) + 1
             
            total = sum(ct5dict.values())
            print(file +"\t"+ str(sorted([(x , ct5dict[x]*100.0/total) for x in ct5dict])))

            for ele in itertools.permutations(all, 6):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]| ele[5]
                ct6dict[len(newset)] = ct6dict.get(len(newset), 0) + 1
             
            total = sum(ct6dict.values())
            print(file +"\t"+ str(sorted([(x , ct6dict[x]*100.0/total) for x in ct6dict])))




            for ele in itertools.permutations(all, 7):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]| ele[5]| ele[6]
                ct7dict[len(newset)] = ct7dict.get(len(newset), 0) + 1
             
            total = sum(ct7dict.values())
            print(file +"\t"+ str(sorted([(x , ct7dict[x]*100.0/total) for x in ct7dict])))


            for ele in itertools.permutations(all, 8):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]| ele[5]| ele[6]| ele[7]
                ct8dict[len(newset)] = ct8dict.get(len(newset), 0) + 1
             
            total = sum(ct8dict.values())
            print(file +"\t"+ str(sorted([(x , ct8dict[x]*100.0/total) for x in ct8dict])))

            for ele in itertools.permutations(all, 9):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]| ele[5]| ele[6]| ele[7]| ele[8]
                ct9dict[len(newset)] = ct9dict.get(len(newset), 0) + 1
             
            total = sum(ct9dict.values())
            print(file +"\t"+ str(sorted([(x , ct9dict[x]*100.0/total) for x in ct9dict])))

            for ele in itertools.permutations(all, 10):
                newset = ele[0] | ele[1] | ele[2] | ele[3]| ele[4]| ele[5]| ele[6]| ele[7]| ele[8]| ele[9]
                ct10dict[len(newset)] = ct10dict.get(len(newset), 0) + 1
             
            total = sum(ct10dict.values())
            print(file +"\t"+ str(sorted([(x , ct10dict[x]*100.0/total) for x in ct10dict])))


















            print()