
if __name__ == "__main__":
    numVar = 13
    each = 0
    count1 = 0
    count2 = 0
    count0 = 0
    for i in range(2**numVar):
        str = '{0:013b}'.format(i)
        x = list(str)
        if x[0] == '0' and x[1] == '0' and x[3] == '1':
            each +=1
        if x[0] == '0' and x[1] == '0' and x[3] == '1' and x[5] == '0'and x[6]=='1' and x[7]=='1':
            count0 +=1
        if x[0] == '0' and x[1] == '0' and x[3] == '1' and x[5] == '0'and x[6]=='1':
            count1 +=1
        if x[0] == '0' and x[1] == '0' and x[3] == '1' and x[5] == '0':
            count2 +=1
    print(2**numVar)
    print(each)
    print(count0)
    print(count1)
    print(count2)