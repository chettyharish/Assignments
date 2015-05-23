#! /bin/env/python3.4
"""
Unit Tests for imdb_ratings_imp.py
"""
import imdb_ratings as imdb
import imdb_ratings_imp as imp


def test_html():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.HTMLPrinter()
    handler.header(["Votes", "Rating", "Year", "Title"])
    handler.row(["5", "6", "1942", "Atlantic Convoy"])

    assert len([x for x in open("output.html")]) == 40


def test_csv():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.CSVPrinter()
    handler.header(["Votes", "Rating", "Year", "Title"])
    handler.row(["5", "6", "1942", "Atlantic Convoy"])
    assert len([x for x in open("output.csv")]) == 2


def test_movie_printer():
    """
    Tests whether the output has the exact number of lines expected
    """
    printer = imp.MoviePrinter(imp.CSVPrinter())
    printer.print_movie_table(
        [imdb.Movie("Secrets of the City", "1994", "1.4", "5")])
    assert len([x for x in open("output.csv")]) == 2


def test_file_printer():
    file = open("temp.txt", mode='w')
    file.write("Atlantic Convoy\t1952\t70\t5.4\t8\t0\t0\n")
    file.close()
    imp.file_printer("temp.txt")
    assert len([x for x in open("output.csv")]) == 2
