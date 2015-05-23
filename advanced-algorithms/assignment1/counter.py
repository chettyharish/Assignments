

if __name__ == "__main__":
    fp = open("temp.txt")
    count = 0
    for line in fp:
        count += int(line.strip())
    print(count)