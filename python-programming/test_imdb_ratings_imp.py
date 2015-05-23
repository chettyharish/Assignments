#! /bin/env/python3.4
"""
Unit Tests for imdb_ratings_imp.py
"""
import imdb_ratings as imdb
import imdb_ratings_imp as imp


def test_txt():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.TableHandler()
    handler.header(["Votes", "Rating", "Year", "Title"])
    handler.row(["5", "6", "1942", "Atlantic Convoy"])
    handler.footer()
    assert len([x for x in open("imdb_output.txt")]) == 2


def test_html():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.HTMLTableHandler()
    handler.header(["Votes", "Rating", "Year", "Title"])
    handler.row(["5", "6", "1942", "Atlantic Convoy"])
    handler.footer()
    assert len([x for x in open("imdb_output.html")]) == 40


def test_csv():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.CSVTableHandler()
    handler.header(["Votes", "Rating", "Year", "Title"])
    handler.row(["5", "6", "1942", "Atlantic Convoy"])
    handler.footer()
    assert len([x for x in open("imdb_output.csv")]) == 2


def test_movie_printer():
    """
    Tests whether the output has the exact number of lines expected
    """
    handler = imp.TableHandler()
    movie_obj = imdb.GroupMovieByElements(
        " 123 10000 10 \"7th Heaven\" (1996)")
    movie_printer = imp.MoviePrinter([movie_obj], handler)
    movie_printer.print_movie_table()
    assert len([x for x in open("imdb_output.txt")]) == 2


def test_file_printer():
    """
    Tests whether the output has the exact number of lines expected
    """
    file = open("temp.txt", mode='w')
    file.write(" 123 10000 10 \"7th Heaven\" (1996)")
    file.close()
    file = open("temp2.txt", mode='w')
    file.write("7th haven")
    file.close()
    imp.file_printer("temp.txt", "temp2.txt")
    assert len([x for x in open("imdb_output.txt")]) == 2
