"""
Program to search an ISBN book on amazon
and return the details of the first book
link found on the website

>>> get_book_info("http://www.amazon.com/\
Introduction-Algorithms-3rd-Thomas-Cormen/dp/0262033844/")
('Introduction to Algorithms, 3rd Edition', ['Thomas H. Cormen'\
, 'Charles E. Leiserson',\
 'Ronald L. Rivest', 'Clifford Stein'],\
 '79.13', '4.3', '170', '8 x 1.8 x 9 inches')

>>> get_book_link("http://www.amazon.com/s/\
ref=nb_sb_noss?url=search-alias%3Dstripbooks\
&field-keywords=0321751043","0321751043")
'http://www.amazon.com/Computer-Programming-\
Volumes-1-4A-Boxed/dp/0321751043/'

"""
import re
import urllib.request


def test_book_link():
    """
    Unit test to test get_book_link function
    """
    assert get_book_link("http://www.amazon.com/s/" +
                         "ref=nb_sb_noss?url=search-alias%3D" +
                         "stripbooks&field-keywords=0321751043", "0321751043")\
        == 'http://www.amazon.com/Computer-' + \
        'Programming-Volumes-1-4A-Boxed/dp/0321751043/'


def test_book_info():
    """
    Unit test to test get_book_info function
    """
    assert get_book_info("http://www.amazon.com/" +
                         "Introduction-Algorithms-3rd-Thomas-Cormen/dp/0262033844/") \
        == ('Introduction to Algorithms, 3rd Edition',
            ['Thomas H. Cormen', 'Charles E. Leiserson',
             'Ronald L. Rivest', 'Clifford Stein'], '79.13',
            '4.3', '170', '8 x 1.8 x 9 inches')


def get_book_info(book_url):
    """
    Function to fetch the book details
    from the amazon search website
    """
    re_book_name = re.compile(r"(>)(.*)(<)")
    re_auth_name = re.compile(r"(field-author=)(.*?)(&)")
    re_price = re.compile(r"(\"buyingPrice\":)(.*?)(,)")
    re_stars = re.compile(r"(title=\")(.*?)( )")
    re_reviewers = re.compile(r"(>)(.*?)( )")
    re_dimension = re.compile(r"(.* )( )(.* inches)")
    (book_name, auth_name, price, stars,
     reviewers, dimension) = 1, [], 1, 1, 1, 1

    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 6.3; \
    WOW64; rv:33.0) Gecko/20100101 Firefox/33.0'}
    request = urllib.request.Request(
        book_url, data=b'None', headers=headers)
    html_reply = urllib.request.urlopen(request)
    html_reply = [x for x in html_reply]

    for line in html_reply:
        if "productTitle" in str(line):
            if re.search(re_book_name, str(line)):
                book_name = re.search(re_book_name, str(line)).group(2)
                break

    for line in html_reply:
        if "field-author" in str(line):
            if re.search(re_auth_name, str(line)):
                auth_name.append(
                    str(re.search(re_auth_name, str(line))
                        .group(2)).replace("+", " "))

    for line in html_reply:
        if "buyingPrice" in str(line):
            if re.search(re_price, str(line)):
                price = re.search(re_price, str(line)).group(2)
                break

    for line in html_reply:
        if "id=\"acrPopover\"" in str(line):
            if re.search(re_stars, str(line)):
                stars = re.search(re_stars, str(line)).group(2)
                break

    for line in html_reply:
        if "id=\"acrCustomerReviewText\"" in str(line):
            if re.search(re_reviewers, str(line)):
                reviewers = re.search(re_reviewers, str(line)).group(2)
                break

    for line in html_reply:
        if "inches" in str(line):
            if re.search(re_dimension, str(line)):
                dimension = re.search(re_dimension, str(line)).group(3)
                break

    return (book_name, auth_name, price, stars, reviewers, dimension)


def get_book_link(path, isbn):
    """
    Function to fetch the book link
    from the amazon search website
    """
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 6.3; \
    WOW64; rv:33.0) Gecko/20100101 Firefox/33.0'}
    request = urllib.request.Request(path, data=b'None', headers=headers)
    html_reply = urllib.request.urlopen(request)
    re_link = re.compile(r"(href=\")(.*?/dp/.*?/)")
    for line in html_reply:
        if "dp/" + isbn in str(line):
            link = re.search(re_link, str(line)).group(2)
    return link


if __name__ == "__main__":
    ISBNLIST = ["0321751043", "1449355730", "032157351X",
                "012383872X", "0262033844", "032190575X",
                "0132126958", "113318779X", "1593271441"]

    for ISBN in ISBNLIST:
        PATH = r"http://www.amazon.com/s/ref=nb_sb_noss?" +\
            "url=search-alias%3Dstripbooks&field-keywords=" + \
            str(ISBN)

        BOOK_URL = get_book_link(PATH, ISBN)
        details = get_book_info(BOOK_URL)
        print("Details for book with ISBN : " + str(ISBN))
        print("\tBook Name : " + str(details[0]))
        print("\tAll authors : " + " , ".join([str(x) for x in details[1]]))
        print("\tPrice : $" + str(details[2]))
        print("\tRating for the book : " + str(details[3]) + " out of 5")
        print("\tNo. of reviewers : " + str(details[4]))
        print("\tDimensions : " + str(details[5]) + "\n")

