#! /bin/env/python3.4
"""
Holds movie objects and functions to create movie objects
Match user required moves and Levenshtein distance

    >>> get_lvdist("Abcd","bcdr")
    2

    >>> Movie("25" , "6.4" , "7th Heaven" , "(1996)").get_year()
    1996

    >>> Movie("25" , "6.4" , "7th Heaven" , "(1996)").get_title()
    '7th Heaven'

    >>> Movie("25" , "6.4" , "7th Heaven" , "(1996)").get_rank()
    6.4

    >>> Movie("25" , "6.4" , "7th Heaven" , "(1996)").get_votes()
    25

    >>> GroupMovieByElements(" 123 38 6.3 \\"7th Heaven\\" (1996)").get_title()
    '7th Heaven'

    >>> x = GroupMovieByElements(" 123 38 6.3 \\"7th Heaven\\" (1996)")
    >>> condition(x,["7th haven"])
    False

"""
import re


class Movie:

    """
    Creates movie object
    """

    def __init__(self, votes, rank, title, year):
        """
        Function initialized Movie object
        """
        re_year = re.compile(r"(\()(\d{0,4})(.*)(\))")
        self.votes = int(votes)
        self.rank = float(rank)

        if re.search(re_year, str(year)).group(2) != "":
            self.year = int(re.search(re_year, str(year)).group(2))
        else:
            self.year = None
        self.title = str(title)

    def get_votes(self):
        """
        Function returns movie votes
        """
        return self.votes

    def get_rank(self):
        """
        Function returns movie rating
        """
        return self.rank

    def get_year(self):
        """
        Function returns movie year
        """
        return self.year

    def get_title(self):
        """
        Function returns movie title
        """
        return self.title


def get_lvdist(name1, name2):
    """
    Function calculates Levenshtein distance between two strings
    """
    if len(name1) < len(name2):
        name1, name2 = name2, name1

    lv_mat = [[0 for i in range(len(name2) + 1)]
              for j in range(len(name1) + 1)]

    for i in range(len(name2) + 1):
        lv_mat[0][i] = i

    for j in range(len(name1) + 1):
        lv_mat[j][0] = j

    for in1, char1 in enumerate(list(name1)):
        for in2, char2 in enumerate(list(name2)):
            if char1 == char2:
                lv_mat[in1 + 1][in2 + 1] = lv_mat[in1][in2]
            else:
                lv_mat[in1 + 1][in2 + 1] = min(lv_mat[in1][in2],
                                               lv_mat[in1][in2 + 1],
                                               lv_mat[in1 + 1][in2]) + 1
    return lv_mat[-1][-1]


def condition(movie_object, user_provided_titles):
    """
    Function to find the min Levenshtein distance
    between all movies and user named movies
    """
    dist = 999
    for line in user_provided_titles:
        name1 = str(line).strip().lower().replace(" ", "")
        name2 = movie_object.get_title().lower().replace(" ", "")
        curr_dist = get_lvdist(name1, name2)
        if curr_dist < dist:
            dist = curr_dist
        if dist <= 3:
            break
    if (movie_object.get_rank() >= 8.0
            and movie_object.get_votes() >= 1000
            and dist <= 3):
        return True
    else:
        return False


def GroupMovieByElements(line):
    """
    Function creates Movie objects from the
    complete file
    """
    re_yearline = re.compile(
        r"((\()(\d{0,4})(\)))|((\()(\d{0,4})(\/.*?)(\)))|(\()(\?\?\?\?)(\))")
    split_line = line.split()
    for j, ele in enumerate(split_line):
        if re.search(re_yearline, split_line[j]):
            votes = split_line[1]
            rank = split_line[2]
            title = " ".join(split_line[3: j]).replace("\"", "")
            year = re.search(re_yearline, split_line[j]).group(0)
            new_movie = Movie(votes, rank, title, year)
            break
    return new_movie
