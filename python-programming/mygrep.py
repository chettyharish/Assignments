import sys
import glob
import os
import time


def mygrepfunc(pattern, filetype):
    iflist = filewalker(filetype)
    sum = 0
    for tom, f in iflist:
        for i, line in enumerate(open(f)):
            if pattern in line:
                sum += 1
                print(
                    time.ctime(tom) + " " + f[-20:] + 
                    " " + str(i) + "  " + line[:40].strip())
    return sum


def filewalker(filetype):
    dirName = "/home/chettyharish/workspace/Exercises/files"
    listDir = [x[0] for x in os.walk(dirName)]
    filesList = [(os.path.getctime(y + "/" + x), y + "/" + x)
                 for y in listDir for x in glob.glob1(y, "*." + str(filetype))]
    return filesList


if __name__ == "__main__":
    arg1 = sys.argv[1]
    arg2 = sys.argv[2]
    print(mygrepfunc(sys.argv[1], str(sys.argv[2])))
