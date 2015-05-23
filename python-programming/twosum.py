#! /usr/bin/env python3.4


def twoSum(x, total):
    i = 0
    j = len(x)-1

    while(True):
        if(x[i]+x[j] == int(total)):
            return (i, j)
        if(x[i]+x[j] > int(total)):
            j = j - 1
        else:
            i = i + 1
    return (-99, -99)

x = [3, 6, 9, 12, 56, 67, 92, 101]
rval = twoSum(x, input("Total"))

if(rval[0] == -99 and rval[1] == -99):
    print("No pairs")
else:
    print(rval[0], rval[1])
