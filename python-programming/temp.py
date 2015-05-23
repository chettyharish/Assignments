import re
re_Year = re.compile(r"((\()(\d{0,4}.*?)(\)))|((\()(\?\?\?\?)(\)))")

year = "(????)"
year = "(1998\III)"
year = "(1998)"
print(re.search(re_Year, str(year)).group(0))
print(re.search(re_Year, str(year)).group(1))
print(re.search(re_Year, str(year)).group(2))
print(re.search(re_Year, str(year)).group(3))

if re.search(re_Year, str(year)).group(3) != "????":
    year = re.search(re_Year, str(year)).group(3)
else:
    year = None
print(year)