import os

def pow2andverf(num1,num2):
    if num1 != num2:
        return False
    elif num1 != 0 and ((num1 & (num1 - 1)) == 0):
        if num1 >= 2**3:
            return True
        else:
            return False
    else:
        return False
    
if __name__ == "__main__":
    
    counted = dict()
    for path, dirs, files in os.walk("/home/chettyharish/workspace/temporary/Temp"):
        for file in files:
            f = open(os.path.join(path,file) , 'r')
            for line in f:
                line = line.split()
                if line[0] == 'c' or line[0] == 'p':
                    continue
                if int(line[0]) not in counted and -1*int(line[0]) not in counted :
                    if int(line[0]) >= 0:
                        counted[int(line[0])] = [1,0]
                    else:
                        counted[-1*int(line[0])] = [0,1]
                else:
                    if int(line[0]) >= 0:
                        counted[int(line[0])][0] += 1
                    else:
                        counted[-1*int(line[0])][1] += 1
                        
                        
                if int(line[1]) not in counted and -1*int(line[1]) not in counted :
                    if int(line[1]) >= 0:
                        counted[int(line[1])] = [1,0]
                    else:
                        counted[-1*int(line[1])] = [0,1]
                else:
                    if int(line[1]) >= 0:
                        counted[int(line[1])][0] += 1
                    else:
                        counted[-1*int(line[1])][1] += 1
                        
                        
                if int(line[2]) not in counted and -1*int(line[2]) not in counted :
                    if int(line[2]) >= 0:
                        counted[int(line[2])] = [1,0]
                    else:
                        counted[-1*int(line[2])] = [0,1]
                else:
                    if int(line[2]) >= 0:
                        counted[int(line[2])][0] += 1
                    else:
                        counted[-1*int(line[2])][1] += 1
            
            flag = True
            for ele in counted:
                print(str(counted[ele][0])+"\t\t"+str(counted[ele][1]))
                if pow2andverf(counted[ele][0], counted[ele][1]):
                    flag = False
            if flag == True:
                print(file + "\tis   SAT" )
            else:
                print(file + "\tis   NOT SAT" )
            f.close()
            