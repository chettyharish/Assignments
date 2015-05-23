
if __name__ == "__main__":
    numVar = 8
    count0 = 0
    countother = 0
    for i in range(2**numVar):
        str = '{0:08b}'.format(i)
        x = list(str)
        if ((x[0] == '0' and x[1] == '0' and x[2] == '0') or
            (x[4] == '1' and x[5] == '1' and x[7] == '1') or
            (x[0] == '0' and x[2] == '0' and x[7] == '1') or
            (x[1] == '0' and x[2] == '0' and x[3] == '0') ):
            count0 +=1
        else:
            countother +=1
    print(count0)
    print(countother)