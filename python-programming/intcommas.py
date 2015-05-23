"""
asdasd
"""

NUMBER = 12345678915

RNUM = str(NUMBER)[::-1]

RNUML = []
for i in range(0, len(RNUM)):
    if i % 3 == 0 and i != 0:
        RNUML.append(',')
        RNUML.append(RNUM[i])
    else:
        RNUML.append(RNUM[i])

NUMBER = ''.join(RNUML)
print(NUMBER[::-1])
