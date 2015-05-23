import re


class Movie:
    def __init__(self, index, dist, votes, rank, title, year):
        self.dist = dist
        self.votes = int(votes)
        self.rank = float(rank)
        self.title = title
        self.year = int(year)
        self.index = int(index)

    def getIndex(self):
        return self.index
    
    def getDist(self):
        return self.dist

    def getYear(self):
        return self.year

    def getRank(self):
        return self.rank

    def getTitle(self):
        return self.title

    def getVotes(self):
        return self.votes


def createMovieList(fileData):
    all_movie_list = []
    re_Year = re.compile(r"(\()(\d{4})(\))")
    for i, line in enumerate(f):
        split_line = line.split()
        for j, ele in enumerate(split_line):
            if re.search(re_Year, split_line[j]):
                new_movie = Movie(i,split_line[0], split_line[1], split_line[2],
                                  " ".join(
                                      split_line[3: j]).replace("\"", ""),
                                  re.search(re_Year, split_line[j]).group(2))
                all_movie_list.append(new_movie)

    return all_movie_list


def filterMovieList(all_movie_list):
    filter_movie_list = dict()
    for movie in all_movie_list:
        if movie.getYear() == 2010 and movie.getVotes() > 1000 and movie.getRank() > 8.0:
            if movie.getTitle() not in filter_movie_list:
                filter_movie_list[movie.getTitle()] = dict()
                filter_movie_list[movie.getTitle()]["index"] = movie.getIndex()
                filter_movie_list[movie.getTitle()]["rank"] = movie.getRank()
                filter_movie_list[movie.getTitle()]["votes"] = movie.getVotes()
                filter_movie_list[movie.getTitle()]["year"] = movie.getYear()
                filter_movie_list[movie.getTitle()]["dist"] = movie.getDist()
    return filter_movie_list

def getDistribution(num):
    return  {
             "." : "no votes cast",
             "0" : "01-09%",
             "1" : "10-19%",
             "2" : "20-29% ",
             "3" : "30-39%",
             "4" : "40-49%",
             "5" : "50-59%",
             "6" : "60-69%",
             "7" : "70-79%",
             "8" : "80-89%",
             "9" : "90-99%"
             }.get(num)
    
    
def parseDistribution(distribution):
    terms = list(distribution)
    output = "\tRating\tDistribution(%users)\n"
    for rating in range(10):
        output = output + "\t"+ str(rating) + "\t\t"+  getDistribution(terms[rating]) + "\n"
    return output

if __name__ == "__main__":
    f = open("files/ratings.list", encoding="ISO-8859-1")
    all_movie_list = createMovieList(f)
    filter_movie_list = filterMovieList(all_movie_list)
    for movie in filter_movie_list:
        print(str(movie))
        print("\tindex :" + str(filter_movie_list[movie]["index"]))
        print("\trank :" + str(filter_movie_list[movie]["rank"]))
        print("\tvotes :" + str(filter_movie_list[movie]["votes"]))
        print("\tyear :" + str(filter_movie_list[movie]["year"]))
        print(parseDistribution(filter_movie_list[movie]["dist"]))
