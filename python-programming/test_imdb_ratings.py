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
        "25", "6.4", "7th Heaven", "(1996)").get_votes() == 25


def test_title():
    """
    Testing Movie object title
    """
    assert imdb.Movie(
        "25", "6.4", "7th Heaven", "(1996)").get_title() ==\
        '7th Heaven'


def test_year():
    """
    Testing Movie object year
    """
    assert imdb.Movie(
        "25", "6.4", "7th Heaven", "(1996)").get_year() == 1996


def test_year2():
    """
    Testing Movie object year
    """
    assert imdb.Movie(
        "25", "6.4", "7th Heaven", "(????)").get_year() is None


def test_rank():
    """
    Testing Movie object rating
    """
    assert imdb.Movie(
        "25", "6.4", "7th Heaven", "(1996)").get_rank() == 6.4


def test_creator():
    """
    Testing movie object creator
    """
    movie = imdb.GroupMovieByElements(" 123 38 6.3 \"7th Heaven\" (1996)")
    assert movie.get_title() == '7th Heaven'


def test_condition():
    """
    Testing condition
    """
    movie = imdb.GroupMovieByElements(" 123 38 6.3 \"7th Heaven\" (1996)")
    assert imdb.condition(movie, ["7th haven"]) is False
