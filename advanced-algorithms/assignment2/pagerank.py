import os
import yaml
import sys
import ast
import numpy

index = 0
file_table = dict()
directory_table = dict()
directory_tree = dict()
path_to_index = dict()
index_to_path = dict()
outlinks = dict()
path_to_module_generic = dict()
include_list = ["math", "decimal", "datetime", "time", "re", "glob", "fnmatch",
                "os", "tempfile", "shutil", "json", "os.path", "sqlite",
                "random", "logging", "urllib2"]


def outputData():
    stream1 = open('directory_tree.yaml', 'w')
    stream2 = open('file_table.yaml', 'w')
    stream3 = open('directory_table.yaml', 'w')
    stream4 = open('path_to_index.yaml', 'w')
    stream5 = open('index_to_path.yaml', 'w')
    stream6 = open('outlinks.yaml', 'w')
    yaml.dump(directory_tree, stream1, indent=6)
    yaml.dump(file_table, stream2, indent=6)
    yaml.dump(directory_table, stream3, indent=6)
    yaml.dump(path_to_index, stream4, indent=6, default_flow_style=False)
    yaml.dump(index_to_path, stream5, indent=6, default_flow_style=False)
    yaml.dump(outlinks, stream6, indent=6)


def pagerank(graph, s, t, threshold, num_nodes):
    """
    Function to calculate each step of pagerank
    by splitting = M = sA + sD + tE
    So the formula for each multiplication is
    We use simplied forms of D and E
    Runs for 100 iterations or if the change in P is less than threshold
    """
    P_old = numpy.matrix(numpy.ones((num_nodes, 1))) / num_nodes
    for i in range(100):
        print("Pagerank step " + str(i + 1))
        P_new = numpy.matrix(numpy.zeros((num_nodes, 1)))
        D = sum([P_old[j] for j in graph.dangling_modules.keys()]) / num_nodes
        E = 1 / num_nodes
        for j in range(num_nodes):
            A = sum([P_old[k] / len(graph.out_links[k])
                     for k in graph.in_links[j]])
            P_new[j] = s * A + s * D + t * E
        P_new = P_new / numpy.sum(P_new)

        norm_diff = numpy.sum(numpy.abs(P_old - P_new))
        if(norm_diff < threshold):
            return P_old

        P_old = P_new
    return P_old


def initialize_graph(graph, outlinks):
    """
    Converts our outlinks into required graph!
    """
    for ele in outlinks:
        for values in outlinks[ele]:
            if ele not in graph.in_links[values]:
                graph.in_links[values].append(ele)

        if len(outlinks[ele]) > 0:
            graph.out_links[ele] = list(set(outlinks[ele]))
            if ele in graph.dangling_modules:
                graph.dangling_modules.pop(ele)


class Graph:

    def __init__(self, num_nodes):
        self.in_links = dict()
        self.out_links = dict()
        self.dangling_modules = dict()
        for i in range(num_nodes):
            self.in_links[i] = []
            self.out_links[i] = []
            self.dangling_modules[i] = 1


def listjoin(list1, list2):
    """
    Function to append a list into list
    without creating a new list!
    """
    for ele in list2:
        list1.append(ele)


def bfs(temp, path, votes, name_s, path_s):
    """
    Function to carry out a bfs so that we can account for all votes
    in directory and sub-directories.
    Returns a list of number using lookuptable and stuff
    Takes in the dict() where we start the bfs
    """
    queue = [temp]
    output = []
    while len(queue) > 0:
        for key in queue[0].keys():
            if isinstance(queue[0].get(key), dict):
                queue.append(queue[0].get(key))
            else:
                output.append(key)
        queue.pop(0)

    for file in output:
        fpath_list = file_table[file]
        if (len(fpath_list) == 1):
            """
            Only one file with that name!
            Just use it for indexing
            """
            votes.append(path_to_index[fpath_list[0]])
        else:
            """
            Multiple files with same name
            Select the fpath using the base directory
            """
            if name_s != None:
                for fpath in fpath_list:
                    fpath_s = fpath.split("/")
                    if name_s[0] == fpath_s[0]:
                        votes.append(path_to_index[fpath])

            if path_s != None:
                for fpath in fpath_list:
                    fpath_s = fpath.split("/")
                    if path_s[0] == fpath_s[0]:
                        votes.append(path_to_index[fpath])

        # Some weird import , pushing it to generic type
        votes.append(path_to_index["generic"])


class ParseImports(ast.NodeVisitor):

    """
    A single import node is parsed and added to main dictionary
    """

    def __init__(self, path, file, votes):
        self.votes = votes
        self.path = path
        self.file = file

    def visit_Import(self, node):
        for alias in node.names:
            name_s = alias.name.split(".")
            path_s = self.path.split("/")
            dname = name_s[-1]  # It may be a directory
            fname = name_s[-1] + ".py"  # It may or may not be a file
            if alias.name in include_list:
                """
                Part of include list and is a file in the library
                So appending votes by getting the index from the path
                and getting the path from the lookuptable
                """
                if alias.name == "os":
                    """
                    Special case of "os"
                    """
                    self.votes.append(
                        path_to_index[file_table["os.path.py"][0]])
                self.votes.append(path_to_index[file_table[fname][0]])
                return

            if fname in file_table:
                """
                File found in table, can be a single path or a list of paths with same name
                """
                fpath_list = file_table[fname]
                if len(fpath_list) == 1:
                    self.votes.append(path_to_index[fpath_list[0]])
                    return
                else:
                    """
                    First look for a path starting with same base module
                    Then look for a path starting with same base path
                    """
                    for fpath in fpath_list:
                        fpath_s = fpath.split("/")
                        if name_s[0] == fpath_s[0]:
                            self.votes.append(path_to_index[fpath])
                            return

                    for fpath in fpath_list:
                        fpath_s = fpath.split("/")
                        if path_s[0] == fpath_s[0]:
                            self.votes.append(path_to_index[fpath])
                            return

                    # Some weird import , pushing it to generic type
                    self.votes.append(path_to_index["generic"])
                    return

            if dname in directory_table:
                """
                Directory found in table, can be a single directory or a list of directories with same name
                """
                dpath_list = directory_table[dname]
                if len(dpath_list) == 1:
                    dpath_s = dpath_list[0].split("/")
                    try:
                        temp = directory_tree
                        for ele in dpath_s:
                            temp = temp[ele]
                        bfs(temp, self.path, self.votes, name_s, path_s)
                    except Exception:
                        #Directory is not present in tree
                        # So the directory has no .py files
                        # So no votes were cast!
                        return
                    return
                else:
                    """
                    First look for a path starting with same base module
                    Then look for a path starting with same base path
                    """
                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if name_s[0] == dpath_s[0]:
                            try:
                                temp = directory_tree
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path,
                                    self.votes, name_s, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return

                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if path_s[0] == dpath_s[0]:
                            try:
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path,
                                    self.votes, name_s, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return

                    # Some weird import , pushing it to generic type
                    self.votes.append(path_to_index["generic"])
                    return
                return

            # Does not end in a file nor in a directory !
            # So no other option than generic
            self.votes.append(path_to_index["generic"])
            return

    def visit_ImportFrom(self, node):
        for alias in node.names:
            level = node.level
            fromloc = node.module
            path_s = self.path.split("/")
            importloc = alias.name
            importloc_s = alias.name.split(".")
            if fromloc != None:
                fromloc_s = fromloc.split(".")

            if fromloc in include_list:
                """
                Part of include list and is a file in the library
                So appending votes by getting the index from the path
                and getting the path from the lookuptable
                """
                if fromloc == "os":
                    """
                    Special case of "os"
                    """
                    self.votes.append(
                        path_to_index[file_table["os.path.py"][0]])
                self.votes.append(
                    path_to_index[file_table[fromloc + ".py"][0]])
                return

            if importloc == "*":
                """
                Importing everything!
                """
                if fromloc == None or level > 0:
                    """
                    Direct addressing of imports
                    Also we have to use level - 1 as 
                    first "." represents own folder
                    """
                    module_path_s = path_s[0:len(path_s) - (level - 1)]
                    temp = directory_tree
                    for ele in module_path_s:
                        temp = temp[ele]

                    if fromloc == None:
                        """
                        import of the form
                        from . import *
                        from .. import *
                        """
                        bfs(temp, self.path, self.votes, None, path_s)
                        return
                    else:
                        """
                        import of the form
                        from .linalg import *
                        from ..pylab import *
                        """

                        fromloc_s = fromloc.split(".")

                        for ele in fromloc_s[:-1]:
                            temp = temp[ele]
                        fname = fromloc_s[-1] + ".py"  # Can be a file
                        if fname not in temp:
                            """
                            Import * from a folder
                            """
                            bfs(temp, self.path, self.votes, None, path_s)
                            return
                        else:
                            """
                            Import * from a file
                            """
                            fpath = "/".join(path_s +
                                             fromloc_s[:-1]) + "/" + fname
                            self.votes.append(path_to_index[fpath])
                            return
                    return
                elif fromloc_s[-1] + ".py" in file_table:
                    """
                    File found in table, can be a single path or a list of paths with same name
                    """
                    fname = fromloc_s[-1] + ".py"
                    fpath_list = file_table[fname]
                    if len(fpath_list) == 1:
                        self.votes.append(path_to_index[fpath_list[0]])
                        return
                    else:
                        """
                        First look for a path starting with same base module
                        Then look for a path starting with same base path
                        """
                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if fromloc_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if path_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        # Some weird import , pushing it to generic type
                        self.votes.append(path_to_index["generic"])
                        return
                    pass
                elif fromloc_s[-1] in directory_table:
                    """
                    First look for a path starting with same base module
                    Then look for a path starting with same base path
                    """
                    dpath_list = directory_table[fromloc_s[-1]]
                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if fromloc_s[0] == dpath_s[0]:
                            try:
                                temp = directory_tree
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path, self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return

                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if path_s[0] == dpath_s[0]:
                            try:
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path,
                                    self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return
                else:
                    #                     Does not end in a file nor in a directory !
                    #                     So no other option than generic
                    self.votes.append(path_to_index["generic"])
            else:
                """
                Importing something !
                """
                if fromloc == None or level > 0:
                    """
                    Direct addressing of imports
                    Also we have to use level - 1 as 
                    first "." represents own folder
                    """
                    fromloc_s = path_s[0:len(path_s) - (level - 1)]
                    temp = directory_tree
                    for ele in fromloc_s:
                        temp = temp[ele]

                    if fromloc == None:
                        """
                        import of the form
                        from . import linalg
                        """
                        for ele in importloc_s[:-1]:
                            temp = temp[ele]
                        ele = importloc_s[-1]
                        fname = importloc_s[-1] + ".py"
                        if fname in temp:
                            """
                            File import
                            """
                            fpath = "/".join(fromloc_s) + \
                                "/".join(importloc_s[:-1]) + "/" + fname
                            self.votes.append(path_to_index[fpath])
                        elif ele in temp:
                            """
                            Folder import
                            """

                            temp = temp[ele]
                            bfs(temp, self.path, self.votes, None, path_s)
                        else:
                            # Does not end in a file nor in a directory !
                            # So no other option than generic
                            self.votes.append(path_to_index["generic"])
                        return
                    else:
                        """
                        import of the form
                        from .linalg import *
                        from ..pylab import *
                        """
                        try:
                            mfromloc_s = fromloc.split(".")
                            for ele in mfromloc_s[:-1]:
                                temp = temp[ele]
                            fname = mfromloc_s[-1] + ".py"  # Can be a file
                            if fname in temp:
                                """
                                Import from a file
                                """
                                fpath = "/".join(fromloc_s +
                                                 mfromloc_s[:-1]) + "/" + fname
                                self.votes.append(path_to_index[fpath])
                                return
                            else:
                                """
                                Import * from a folder
                                """
                                for ele in importloc_s[:-1]:
                                    temp = temp[ele]
                                    ele = importloc_s[-1]
                                fname = importloc_s[-1] + ".py"

                                if fname in temp:
                                    """
                                    File import
                                    """
                                    fpath = "/".join(mfromloc_s) + \
                                        "/".join(importloc_s[:-1]
                                                 ) + "/" + fname
                                    self.votes.append(path_to_index[fpath])
                                elif ele in temp:
                                    """
                                    Folder import
                                    """

                                    temp = temp[ele]
                                    bfs(temp, self.path,
                                        self.votes, None, path_s)
                                else:
                                    # Does not end in a file nor in a directory !
                                    # So no other option than generic
                                    self.votes.append(path_to_index["generic"])
                                return
                        except Exception:
                            # Does not end in a file nor in a directory !
                            # So no other option than generic
                            self.votes.append(path_to_index["generic"])
                    return
                elif fromloc_s[-1] + ".py" in file_table:
                    """
                    File found in table, can be a single path or a list of paths with same name
                    """
                    fname = fromloc_s[-1] + ".py"
                    fpath_list = file_table[fname]
                    if len(fpath_list) == 1:
                        self.votes.append(path_to_index[fpath_list[0]])
                        return
                    else:
                        """
                        First look for a path starting with same base module
                        Then look for a path starting with same base path
                        """
                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if fromloc_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if path_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        # Some weird import , pushing it to generic type
                        self.votes.append(path_to_index["generic"])
                        return
                elif fromloc_s[-1] in directory_table:
                    """
                    First look for a path starting with same base module
                    Then look for a path starting with same base path
                    """
                    dpath_list = directory_table[fromloc_s[-1]]
                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if fromloc_s[0] == dpath_s[0]:
                            try:
                                temp = directory_tree
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path, self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return

                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if path_s[0] == dpath_s[0]:
                            try:
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path,
                                    self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return
                elif importloc_s[-1] + ".py" in file_table:
                    """
                    File found in table, can be a single path or a list of paths with same name
                    """
                    fname = importloc_s[-1] + ".py"
                    fpath_list = file_table[fname]
                    if len(fpath_list) == 1:
                        self.votes.append(path_to_index[fpath_list[0]])
                        return
                    else:
                        """
                        First look for a path starting with same base module
                        Then look for a path starting with same base path
                        """
                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if fromloc_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        for fpath in fpath_list:
                            fpath_s = fpath.split("/")
                            if path_s[0] == fpath_s[0]:
                                self.votes.append(path_to_index[fpath])
                                return

                        # Some weird import , pushing it to generic type
                        self.votes.append(path_to_index["generic"])
                        return
                elif importloc_s[-1] in directory_table:
                    """
                    First look for a path starting with same base module
                    Then look for a path starting with same base path
                    """
                    dpath_list = directory_table[importloc_s[-1]]
                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if fromloc_s[0] == dpath_s[0]:
                            try:
                                temp = directory_tree
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path, self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return

                    for dpath in dpath_list:
                        dpath_s = dpath.split("/")
                        if path_s[0] == dpath_s[0]:
                            try:
                                for ele in dpath_s:
                                    temp = temp[ele]
                                bfs(temp, self.path,
                                    self.votes, None, path_s)
                            except Exception:
                                #Directory is not present in tree
                                # So the directory has no .py files
                                # So no votes were cast!
                                return
                            return
                else:
                    #                     Does not end in a file nor in a directory !
                    #                     So no other option than generic
                    self.votes.append(path_to_index["generic"])


class FindImports(ast.NodeVisitor):

    """
    Import Calls (Votes) are added to the main set
    """

    def __init__(self, path, file, votes):
        self.votes = votes
        self.path = path
        self.file = file

    def visit_Import(self, node):
        parser = ParseImports(self.path, self.file, self.votes)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)

    def visit_ImportFrom(self, node):
        parser = ParseImports(self.path, self.file, self.votes)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)


def add_entry(path):
    """
    Using temp to create new dictionary
    and then jump to that dictionary
    Equivalent to traversing a tree
    """
    temp = directory_tree
    for curr_dir in path.split("/"):
        temp[curr_dir] = temp.get(curr_dir, dict())
        temp = temp[curr_dir]
    return temp


def path_crawler(base_path):
    """
    Walks through all files in the directory
    listing them
    """
    global index
    for path, dirs, files in os.walk(base_path):
        if base_path in path:
            path = os.path.relpath(path, base_path)
        for directory in dirs:
            if directory in directory_table:
                directory_table[directory].append(path)
            else:
                directory_table[directory] = [path]

        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1 and file[0] != "." and path != "."):
                fpath = "/".join([path, file])
                temp = add_entry(path)
                temp[file] = []

                if file in file_table:
                    file_table[file].append(fpath)
                else:
                    file_table[file] = [fpath]

                path_to_index[fpath] = index
                index_to_path[index] = fpath
                index += 1


def generic_path_crawler():
    """
    Simply adds the file names for now
    and looks in sys.path for the source libraries (if available)
    """
    global index
    for ele in include_list:
        fpath = "generic/" + ele + ".py"
        file_table[ele + ".py"] = [fpath]
        path_to_index[fpath] = index
        index_to_path[index] = fpath
        path_to_module_generic[index] = ele
        index += 1


def file_crawler(base_path):
    """
    Walks through all files and parses them for votes!
    """
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1 and file[0] != "." and path != "."):
                if base_path in path:
                    path = os.path.relpath(path, base_path)
                path_s = path.split("/")
                temp = directory_tree
                for curr_dir in path_s:
                    temp = temp[curr_dir]
                parseFile(path, file, temp[file])


def parseFile(path, file, votes):
    """
    Scans through the file and lists all the votes
    parenthesis_flag -> flag to set if a "(" is observed
                        for multi-line import
    slash_flag -> flag to set if a "\" is observed 
                    for multi-line import
    """

    data = []
    parenthesis_flag = False
    slash_flag = False
    file_name = path + "/" + file

#     print(file_name)
    for line in open(file_name, "r", encoding="iso-8859-1"):
        splits = line.lstrip(" ").split(" ")
        if (splits[0] == "import" or splits[0] == "from"
                or parenthesis_flag == True or slash_flag == True):
            if(len(splits) > 2 and splits[0] == "import"):
                if splits[2] != "as":
                    continue

            if(splits[0] == "from"):
                if(len(splits) > 2):
                    if splits[2] != "import":
                        continue
                else:
                    continue

            data.append(line.lstrip(' '))
            if "(" in line:
                parenthesis_flag = True
            if ")" in line:
                parenthesis_flag = False

            if "\\" in line:
                slash_flag = True
            else:
                slash_flag = False

    data = "".join(data)
    tree = ast.parse(data)

    imp_obj = FindImports(path, file, votes)
    imp_obj.visit(tree)


def outlinks_walker(base_path):
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1 and file[0] != "." and path != "."):
                if base_path in path:
                    path = os.path.relpath(path, base_path)

                file_path = path + "/" + file
                split = file_path.split("/")
                temp = directory_tree
                for ele in split[:-1]:
                    temp = temp[ele]
                outlinks[path_to_index[file_path]] = temp[file]

    for ele in include_list:
        ele = ele + ".py"
        path = "generic/" + ele
        outlinks[path_to_index[path]] = []


if __name__ == "__main__":
    if(len(sys.argv) != 3):
        print("Usage : python3.4 package_path pagerank_threshold")
        print("Example : python3.4 . 1e-5")
        exit(1)
    base_path = sys.argv[1]
    threshold = float(sys.argv[2])
    s = 0.85
    t = 1 - s

#     The default generic.py entry , it will be a dangling pointer:
    path_to_index["generic"] = 0
    index_to_path[0] = "generic"
    index += 1

    print("Starting Indexer")
    path_crawler(base_path)
    generic_path_crawler()
    
    print("Starting File Parser")
    file_crawler(base_path)
    outlinks_walker(base_path)
    outlinks[0] = []

    print("Initializing Graph")
    num_nodes = len(outlinks)
    graph = Graph(num_nodes)
    initialize_graph(graph, outlinks)

    print("Calculating pagerank")
    prlist = pagerank(graph, s, t, threshold, num_nodes)
    prlist = numpy.array(prlist).flatten().tolist()

#     print("Writing Data")
#     outputData()

    f2 = open("pagerank_modules.csv", "w")
    f3 = open("pagerank_packages.csv", "w")
    print("Writing module pageranks written in pagerank_modules.csv")

    for i, ele in enumerate(prlist):
        if i in path_to_module_generic:
            name = path_to_module_generic[i].replace(".py", "")
            name = name.replace("generic/", "")
            f2.write(name + "," + str(ele) + "\n")
        else:
            name = str(index_to_path[i]).replace("/", ".")
            name = name.replace(".py", "")
            f2.write(name + "," + str(ele) + "\n")

    print("Writing module pageranks written in pagerank_packages.csv")
    package_dict = dict()
    for i, ele in enumerate(prlist):
        if i in path_to_module_generic:
            name = path_to_module_generic[i].replace(".py", "")
            name = name.replace("generic/", "")
            name_s = name.split(".")
            package_dict[name_s[0]] = package_dict.get(name_s[0], 0) + ele
        else:
            name = str(index_to_path[i]).replace("/", ".")
            name_s = name.split(".")
            package_dict[name_s[0]] = package_dict.get(name_s[0], 0) + ele

    for package in package_dict:
        f3.write(package + "," + str(package_dict[package]) + "\n")
