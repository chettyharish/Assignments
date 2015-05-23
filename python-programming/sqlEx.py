import sqlite3
from lxml import etree


if __name__ == "__main__":
    #mapdb = sqlite3.connect("mapdb.sqlite")
    doc = etree.parse("/home/chettyharish/workspace/Exercises/files/map.osm")
    root = doc.getroot()
#     for elem in root.findall("node"):
#         print(elem.get("lat"), elem.get("lon"))
    for way in root.findall("way"):
        print(way.get("id"))
        print("Number of nodes in path :" + str(len(way.findall("nd")))) 
        for nd in way.findall("nd"):
            for node in root.findall("node"):
                if(node.get("id") == nd.get("ref")):
                    print(node.get("lat") +"\t"+ node.get("lon"))
        print