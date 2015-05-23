#! /bin/env/python3.4
"""
Unit Tests for imdb_ratings.py
"""
import imdb_ratings as imdb


def test_get_lvdist():
    """
    Testing get_lvdist()
    """
    assert imdb.get_lvdist("Abcd", "bcdr") == 2


def test_votes():
    """
    Testing Movie object votes
    """
    assert imdb.Movie(
        "Secrets of the City", "1994", "1.4", "5").get_votes() == '5'


def test_title():
    """
    Testing Movie object title
    """
    assert imdb.Movie(
        "Secrets of the City", "1994", "1.4", "5").get_title() ==\
        'Secrets of the City'


def test_year():
    """
    Testing Movie object year
    """
    assert imdb.Movie(
        "Secrets of the City", "1994", "1.4", "5").get_year() == '1994'


def test_rating():
    """
    Testing Movie object rating
    """
    assert imdb.Movie(
        "Secrets of the City", "1994", "1.4", "5").get_rating() == '1.4'


def test_creator():
    """
    Testing movie object creator
    """
    assert imdb.GroupMovieByElements(
        "Anne of the Indies\t1951\t87\t6.5\t64\t0").get_title() ==\
        'Anne of the Indies'


def test_condition():
    """
    Testing condition
    """
    line = open("needed_movies.txt", "r").readline()
    record = imdb.Movie(line, 0, 0, 0)
    assert imdb.condition(record)
