#! /bin/env/python3.4
"""
Function to create CSV and HTML output of movies
of user from a user supplied file

    >>> handler = TableHandler()
    >>> handler.header(["Votes", "Rating", "Year", "Title"])
    >>> handler.row(["5", "6", "1942", "Atlantic Convoy"])
    >>> handler.footer()
    >>> len([x for x in open("imdb_output.txt")])
    2

    >>> handler = HTMLTableHandler()
    >>> handler.header(["Votes", "Rating", "Year", "Title"])
    >>> handler.row(["5", "6", "1942", "Atlantic Convoy"])
    >>> handler.footer()
    >>> len([x for x in open("imdb_output.html")])
    40

    >>> handler = CSVTableHandler()
    >>> handler.header(["Votes", "Rating", "Year", "Title"])
    >>> handler.row(["5", "6", "1942", "Atlantic Convoy"])
    >>> handler.footer()
    >>> len([x for x in open("imdb_output.csv")])
    2

    >>> from imdb_ratings import *
    >>> from imdb_ratings_imp import *
    >>> handler = TableHandler()
    >>> movie_obj=GroupMovieByElements(" 123 10000 10 \\"7th Heaven\\" (1996)")
    >>> mp = MoviePrinter([movie_obj], handler)
    >>> mp.print_movie_table()
    >>> len([x for x in open("imdb_output.txt")])
    2

    >>> from imdb_ratings import *
    >>> from imdb_ratings_imp import *
    >>> file = open("temp.txt", mode='w')
    >>> file.write(" 123 38 6.3 \\"7th Heaven\\" (1996)")
    31
    >>> file.close()
    >>> file = open("temp2.txt", mode='w')
    >>> file.write("7th haven")
    9
    >>> file.close()
    >>> file_printer("temp.txt" , "temp2.txt")
    >>> len([x for x in open("imdb_output.txt")])
    1

"""
from imdb_ratings import condition, GroupMovieByElements


class TableHandler(object):

    """
    Class to print Tab spaced output
    """

    def __init__(self):
        """
        Initializing
        """
        self = self

    def header(self, hdata):
        """
        Function to print header
        """
        self = self
        file = open("imdb_output.txt", "w")
        file.write(str("\t".join(hdata)) + "\n")

    def row(self, rdata):
        """
        Function to print row data
        """
        self = self
        file = open("imdb_output.txt", "a")
        file.write(str("\t".join(rdata)) + "\n")

    def footer(self):
        """
        Function to print footer data
        """
        pass


class HTMLTableHandler(TableHandler):

    """
    Class to print HTML output
    """

    def __init__(self):
        """
        Initializing
        """
        self = self

    def header(self, hdata):
        """
        Function to print header
        """
        self = self
        file = open("imdb_output.html", "w")
        file.write("<html>\n\t<head>\n\t<style>\n" +
                   "\t\t\ttable, th, td {border: 1px solid\n" +
                   "\t\t\tblack;border-collapse: collapse;}" +
                   "\n\t</style>\n" +
                   "\t</head>\n\t<body>\n\t\t<table style=\"width:100%\">\n")
        file.write("\t\t\t<tr>\n")
        for line in hdata:
            file.write(
                "\t\t\t\t\t<th>\n\t\t\t\t\t\t"
                + str(line) + "\n\t\t\t\t\t</th>\n")
        file.write("\t\t\t</tr>\n")

    def row(self, rdata):
        """
        Function to print row data
        """
        self = self
        file = open("imdb_output.html", "a")
        file.write("\t\t\t<tr>\n")
        for line in rdata:
            file.write(
                "\t\t\t\t\t<td>\n\t\t\t\t\t\t"
                + str(line) + "\n\t\t\t\t\t</td>\n")
        file.write("\t\t\t</tr>\n")

    def footer(self):
        """
        Function to print footer data
        """
        file = open("imdb_output.html", "a")
        file.write("\t\t</table>\n\t</body>\n</html>\n")


class CSVTableHandler(TableHandler):

    """
    Class to print CSV output
    """

    def __init__(self):
        """
        Initializing
        """
        self = self

    def header(self, hdata):
        """
        Function to print header
        """
        self = self
        file = open("imdb_output.csv", "w")
        file.write(str(",".join(hdata)) + "\n")

    def row(self, rdata):
        """
        Function to print row data
        """
        self = self
        file = open("imdb_output.csv", "a")
        file.write(str(",".join(rdata)) + "\n")

    def footer(self):
        """
        Function to print footer data
        """
        pass


class MoviePrinter:

    """
    Class to print output
    """

    def __init__(self, movie_list, handler):
        """
        Initializer which assigns handler
        """
        self = self
        self.movie_list = movie_list
        self.handler = handler

    def print_movie_table(self):
        """
        Function to print data using handler
        """
        self = self
        headers = ["Votes", "Rank", "Year", "Title"]
        self.handler.header(headers)

        for movie in self.movie_list:
            self.handler.row([str(movie.get_votes()), str(movie.get_rank()),
                              str(movie.get_year()), str(movie.get_title())])

        self.handler.footer()


def file_printer(all_file, user_file):
    """
    Function to output data
    """
    movie_list = []
    user_provided_titles = [x for x in open(user_file)]
    for line in open(all_file, encoding="ISO-8859-1"):
        record = GroupMovieByElements(line)
        if condition(record, user_provided_titles):
            movie_list.append(record)

    handler = TableHandler()
    printer = MoviePrinter(movie_list, handler)
    printer.print_movie_table()

    handler = HTMLTableHandler()
    printer = MoviePrinter(movie_list, handler)
    printer.print_movie_table()

    handler = CSVTableHandler()
    printer = MoviePrinter(movie_list, handler)
    printer.print_movie_table()

if __name__ == "__main__":
    file_printer("files/ratings.list", "needed_movies.txt")
