#! /usr/bin/env python3.4

nt = ["#"]
cnt = 1

if __name__ == "__main__":
    NUMBER = input();
    for i in range(len(NUMBER)):
        nt.append(NUMBER[i])
        nt.append("#")

    NUMBER = ''.join(nt)
    t = [None]*len(NUMBER)

    i = 0
    while i < (len(NUMBER)):
        while True:
            if i - cnt > 0 and i + cnt < len(NUMBER) and NUMBER[i - cnt] == NUMBER[i + cnt]:
                cnt = cnt + 1
            else:
                t[i] = cnt - 1
                down = i - 1
                up = i + 1
                print(t)
                while(down > 0 and up < len(NUMBER) and t[down] < (up + cnt) - i ):
                    t[up] = t[down]
                    up = up + 1
                    down = down - 1
                    i = up - 1
                cnt = 1
                break
        i = i + 1

    center = t.index(max(t))
    substr = NUMBER[center-max(t):center] + NUMBER[center] + NUMBER[center+1: center + max(t)]
    print((str(substr)).replace("#", ""))
