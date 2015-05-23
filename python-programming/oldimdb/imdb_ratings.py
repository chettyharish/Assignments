#! /bin/env/python3.4
"""
Holds movie objects and functions to create movie objects
Match user required moves and Levenshtein distance

    >>> get_lvdist("Abcd","bcdr")
    2

    >>> Movie("Secrets of the City","1994","1.4","5").get_votes()
    '5'

    >>> Movie("Secrets of the City","1994","1.4","5").get_title()
    'Secrets of the City'

    >>> Movie("Secrets of the City","1994","1.4","5").get_year()
    '1994'

    >>> Movie("Secrets of the City","1994","1.4","5").get_rating()
    '1.4'

    >>> x = GroupMovieByElements("Atlantic\\t1952\\t70\\t5.4\\t8\\t0\\t0\\n")
    >>> x.get_title()
    'Atlantic'

    >>> line = open("needed_movies.txt", "r").readline()
    >>> record = Movie(line, 0, 0, 0)
    >>> condition(Movie(line, 0, 0, 0))
    True
"""


class Movie:

    """
    Creates movie object
    """

    def __init__(self, title, year, rating, votes):
        """
        Function initialized Movie object
        """
        self.votes = votes
        self.rating = rating
        self.year = year
        self.title = title

    def get_votes(self):
        """
        Function returns movie votes
        """
        return self.votes

    def get_rating(self):
        """
        Function returns movie rating
        """
        return self.rating

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


def condition(record):
    """
    Function to find the min Levenshtein distance
    between all movies and user named movies
    """
    dist = 999
    for line in open("needed_movies.txt"):
        name1 = str(line).strip().lower().replace(" ", "")
        name2 = record.get_title().lower().replace(" ", "")
        curr_dist = get_lvdist(name1, name2)
        if curr_dist < dist:
            dist = curr_dist
        if dist <= 3:
            break
    return dist <= 3


def GroupMovieByElements(line):
    """
    Function creates Movie objects from the
    complete file
    """
    temp = line.split("\t")
    new_movie = Movie(temp[0], temp[1], temp[4], temp[5])
    return new_movie
