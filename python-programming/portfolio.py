import requests
import re


def evalPortfolio(fname):
    """
    Evaluate the portfolio
    """
    total = 0
    portfolio = open("portfolio.csv", "r")
    for line in portfolio:
        mylist = line.split(",")
        total = total + float(float(tickerPrice(mylist[0]))*float(mylist[1]))
    return total


def tickerPrice(ticker):
    """
    Return the ticker price
    """
    urltext = requests.get("http://www.google.com/finance?q="+ticker).text
    m = re.search('id="ref_(.*?)">(.*?)<', urltext).group(2)
    return float(m)

if __name__ == '__main__':
    print(evalPortfolio("portfolio.csv"))
