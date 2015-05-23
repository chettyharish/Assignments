#! /bin/env/python3.4
"""
Function to create CSV and HTML output of movies
of user from a user supplied file

    >>> handler = HTMLPrinter()
    >>> handler.header(["Votes", "Rating", "Year", "Title"])
    >>> handler.row(["5", "6", "1942", "Atlantic Convoy"])
    >>> len([x for x in open("output.html")])
    40

    >>> handler = CSVPrinter()
    >>> handler.header(["Votes", "Rating", "Year", "Title"])
    >>> handler.row(["5", "6", "1942", "Atlantic Convoy"])
    >>> len([x for x in open("output.csv")])
    2

    >>> import imdb_ratings as imdb
    >>> printer = MoviePrinter(CSVPrinter())
    >>> movie_list = [imdb.Movie("Secrets of the City", "1994", "1.4", "5")]
    >>> printer.print_movie_table(movie_list)
    >>> len([x for x in open("output.csv")])
    2

    >>> file = open("temp.txt", mode='w')
    >>> file.write("Atlantic Convoy\\t1952\\t70\\t5.4\\t8\\t0\\t0\\n")
    34
    >>> file.close()
    >>> file_printer("temp.txt")
    >>> len([x for x in open("output.csv")])
    2

"""
from imdb_ratings import condition, GroupMovieByElements


class HTMLPrinter:

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
        file = open("output.html", "w")
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
        file.write("\t\t</table>\n\t</body>\n</html>\n")

    def row(self, rdata):
        """
        Function to print row data
        """
        self = self
        filedata = [line for line in open("output.html", "r")]
        file = open("output.html", "w")
        for i in range(0, len(filedata) - 3):
            file.write(filedata[i])
        file.write("\t\t\t<tr>\n")
        for line in rdata:
            file.write(
                "\t\t\t\t\t<td>\n\t\t\t\t\t\t"
                + str(line) + "\n\t\t\t\t\t</td>\n")
        file.write("\t\t\t</tr>\n")
        for i in range(len(filedata) - 3, len(filedata)):
            file.write(filedata[i])


class CSVPrinter:

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
        file = open("output.csv", "w")
        file.write(str(",".join(hdata)) + "\n")

    def row(self, rdata):
        """
        Function to print row data
        """
        self = self
        file = open("output.csv", "a")
        file.write(str(",".join(rdata)) + "\n")


class MoviePrinter:

    """
    Class to print output
    """

    def __init__(self, handler):
        """
        Initializer which assigns handler
        """
        self = self
        self.handler = handler

    def print_movie_table(self, mlist):
        """
        Function to print data using handler
        """
        self = self
        headers = ["Votes", "Rating", "Year", "Title"]
        self.handler.header(headers)

        for movie in mlist:
            self.handler.row([movie.get_votes(), movie.get_rating(),
                              movie.get_year(), movie.get_title()])


def file_printer(file):
    """
    Function to output data
    """
    mlist = []
    for line in open(file):
        record = GroupMovieByElements(line)
        if condition(record):
            mlist.append(record)
    handler = CSVPrinter()
    handler2 = HTMLPrinter()
    printer = MoviePrinter(handler)
    printer.print_movie_table(mlist)

    printer = MoviePrinter(handler2)
    printer.print_movie_table(mlist)

if __name__ == "__main__":
    FILE = "movies.tab"
    file_printer(FILE)
