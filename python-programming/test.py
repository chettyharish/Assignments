import amazon

def test_book_link():
    """
    Unit test to test get_book_link function
    """
    assert amazon.get_book_link("http://www.amazon.com/s/" +
                         "ref=nb_sb_noss?url=search-alias%3D" +
                         "stripbooks&field-keywords=0321751043", "0321751043")\
        == 'http://www.amazon.com/Computer-' + \
        'Programming-Volumes-1-4A-Boxed/dp/0321751043/'


def test_book_info():
    """
    Unit test to test get_book_info function
    """
    assert amazon.get_book_info("http://www.amazon.com/" +
                         "Introduction-Algorithms-3rd-Thomas-Cormen/dp/0262033844/") \
        == ('Introduction to Algorithms, 3rd Edition',
            ['Thomas H. Cormen', 'Charles E. Leiserson',
             'Ronald L. Rivest', 'Clifford Stein'], '79.13',
            '4.3', '170', '8 x 1.8 x 9 inches')