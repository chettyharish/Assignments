import yaml
import ast
import os
import sys
import traceback
import subprocess

"""
Problem import * from generic stuff are unparsable
e.g. from wxPython.wx import *
"""


def get_keys_level_down(directory_tree):
    base = [directory_tree[x].keys() for x in directory_tree if ".py" not in x]
    one_level_down = []
    for x in base:
        one_level_down += x
    return one_level_down


class FindDef(ast.NodeVisitor):

    """
    Func Defs are added to the import table
    """

    def __init__(self, import_name, path):
        self.import_name = import_name
        self.path = path

    def visit_FunctionDef(self, node):
        self.import_name[node.name] = self.path


def FindFuncDef(complete_file_path, lookuptable, import_name):
    """
    Adds Function definition to import table
    """
    # Extracting the file name from complete path and removing the .py in the
    # end
    file_name = complete_file_path.split("/")[-1].split(".")[0]
    if(os.path.exists(complete_file_path)):
        with open(complete_file_path, "r") as myfile:
            data = myfile.read()
    elif file_name in lookuptable:
        complete_file_path = lookuptable[file_name]
        with open(complete_file_path, "r") as myfile:
            data = myfile.read()

    else:
        #         print(file_name)
        #         print(complete_file_path + "\tdoesnt exist")
        return
    try:
        tree = ast.parse(data)
    except Exception:
        """
        v3.4 file has shown!! big problem
        """
        print(complete_file_path)
        return
    fobj = FindDef(import_name, "/".join(complete_file_path.split("/")[:-1]))
    fobj.visit(tree)


def resolveStars(module, import_name, directory_tree, lookuptable, path, file):
    """
    The function handles the case for "*" in import
    The first condition is for modules with complete paths and end in directory (file source)
    The second condition is for modules with complete paths and end in a file (function source)
    The third condition is for file sourcing from own directory
    """
#     print(path + "/" + file)
    keys_level_down = get_keys_level_down(directory_tree)
    module_splits = module.split(".")
    if module_splits[0] in directory_tree and module_splits[0] not in keys_level_down:
        """
        A module from the root directory path
        """
        temp = directory_tree
        for ele in module_splits[:-1]:
            if ele in temp:
                temp = temp[ele]
            else:
                """
                Some unknown file/module used
                (e.g. matplotlib.pylab)
                """
                print("Unknown file1 : " + module)
                return

        if module_splits[-1] + ".py" in temp:
            """
            It is a file, get all the function 
            definitions from inside the file for voting
            """
#             print("From 1")
            FindFuncDef(
                (module + ".py").replace(".", "/"), lookuptable, import_name)
        else:
            """
            It is a directory, so go into it and get 
            all the python files
            """
            if not os.path.isdir(module.replace(".", "/")):
                """
                Some unknown file/module used
                (e.g. matplotlib.pylab)
                """
                print("Unknown file2 : " + module)
                return

            temp = temp[module_splits[-1]]
            for keys in temp:
                """
                Add all the files from the directory to the table
                """
                if ".py" in keys:
                    import_name[
                        keys] = "/".join([module.replace(".", "/"), keys])

    elif module_splits[0] in directory_tree and module_splits[0] in keys_level_down:
        """
        Same as the above, but has the form
        numpy.X instead of numpy.numpy.X
        So delete the first element
        """
        temp = directory_tree[module_splits[0]]
        for ele in module_splits[:-1]:
            if ele in temp:
                temp = temp[ele]
            else:
                """
                Some unknown file/module used
                (e.g. matplotlib.pylab)
                """
                print("Unknown file3 : " + module)
                return
        if module_splits[-1] + ".py" in temp:
            """
            It is a file, get all the function 
            definitions from inside the file for voting
            """
#             print("From 2")
            FindFuncDef(
                (module_splits[0] + "/" + module.replace(".", "/") + ".py"), lookuptable, import_name)
        else:
            """
            It is a directory, so go into it and get 
            all the python files
            """
            if not os.path.isdir(module_splits[0] + "/" + module.replace(".", "/")):
                """
                Some unknown file/module used
                (e.g. matplotlib.pylab)
                """
                print("Unknown file4 : " + module)
                return

            temp = temp[module_splits[-1]]
            for keys in temp:
                """
                Add all the files from the directory to the table
                """
                if ".py" in keys:
                    import_name[
                        keys] = "/".join([module_splits[0], module.replace(".", "/"), keys])

    else:
        """
        It is from the same directory as the file
        It might be the whole directory or just a file
        """
        if module == ".":
            """
            Sourcing entire directory
            """
            temp = directory_tree
            for ele in path.split("/"):
                if ele in temp:
                    temp = temp[ele]
                else:
                    """
                    Some unknown file/module used
                    (e.g. matplotlib.pylab)
                    """
                    print("Unknown file5 : " + module)
                    return

            for keys in temp:
                """
                Add all the files from the directory to the table
                """
                if ".py" in keys:
                    import_name[
                        keys] = "/".join([path.replace(".", "/"), keys])
        else:
            """
            Sourcing a single file
            """
#             print("From 3")
            FindFuncDef(
                (path + "/" + module.replace(".", "/") + ".py"), lookuptable, import_name)
    return


class ParseImports(ast.NodeVisitor):

    """
    A single import node is parsed and added to main dictionary
    """

    def __init__(self, all_class, path, file):
        self.import_name = all_class
        self.path = path
        self.file = file

    def visit_Import(self, node):
        for alias in node.names:
            if alias.asname == None:
                self.import_name[alias.name] = alias.name.replace(".", "/")
            else:
                self.import_name[alias.asname] = alias.name.replace(".", "/")
        ast.NodeVisitor.generic_visit(self, node)

    def visit_ImportFrom(self, node):
        for alias in node.names:
            #             print(str(alias) +"\t"+ node.module +"\t"+str(alias.name) +"\t"+ str(alias.asname) )
            if alias.name == "*":
                """
                Condition where we import all the functions from a module
                (e.g. from numpy import *)
                """
                resolveStars(node.module, self.import_name,
                             directory_tree, lookuptable, self.path, self.file)
            elif alias.asname == None:
                """
                Condition where imported submodule does not have 
                an alias (i.e. "as" condition)
                """
                if node.module == None:
                    """
                    The module name is a "."
                    """
                    self.import_name[alias.name] = "/".join([".", alias.name])
                else:
                    self.import_name[
                        alias.name] = "/".join([node.module.replace(".", "/"), alias.name])
            else:
                """
                Condition where imported submodule does have 
                an alias (i.e. "as" condition)
                """
                if node.module == None:
                    """
                    The module name is a "."
                    """
                    self.import_name[
                        alias.asname] = "/".join([".", alias.name])
                else:
                    """
                    The module name is either a complete path from root
                    or a file from own directory
                    or a random import from the library (resolved using lookuptable)
                    """
                    self.import_name[
                        alias.name] = "/".join([node.module.replace(".", "/"), alias.name])
        ast.NodeVisitor.generic_visit(self, node)


class FindImports(ast.NodeVisitor):

    """
    Import Calls are added to the main dictionary
    """

    def __init__(self, path, file):
        self.all_class = dict()
        self.path = path
        self.file = file

    def visit_Import(self, node):
        parser = ParseImports(self.all_class, self.path, self.file)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)

    def visit_ImportFrom(self, node):
        parser = ParseImports(self.all_class, self.path, self.file)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)

    def get_list(self):
        return self.all_class


class ParseFuncs(ast.NodeVisitor):

    """
    A single function node is parsed here
    """

    def __init__(self):
        self.func_name = []

    def visit_Attribute(self, node):
        ast.NodeVisitor.generic_visit(self, node)
        self.func_name.append(node.attr)

    def visit_Name(self, node):
        self.func_name.append(node.id)


class FindFuncs(ast.NodeVisitor):

    """
    Func Calls are added to the main list
    """

    def __init__(self):
        self.all_func = []

    def visit_Call(self, node):
        parser = ParseFuncs()
        parser.visit(node.func)
        self.all_func.append(parser.func_name)
        ast.NodeVisitor.generic_visit(self, node)

    def get_list(self):
        return self.all_func


def add_entry(path, directory_tree):
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


def path_crawler(base_path, directory_tree, lookuptable):
    """
    Walks through all files in the directory
    listing them
    """
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if file.endswith(".py") and file.find("__init__") == -1 and file[0] != ".":
                temp = add_entry(
                    os.path.relpath(path, base_path), directory_tree)
                temp[file] = dict()
                lookuptable[file.split(".")[0]] = path + "/" + file


def generic_path_crawler(directory_tree, include_list, lookuptable):
    """
    Simply adds the file names for now
    """
    for ele in include_list:
        directory_tree[ele] = dict()
        lookuptable[ele] = ""


def file_crawler(base_path, directory_tree, lookuptable):
    """
    Walks through all files and parses them for votes!
    """
    f = open("other_version", "w")
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if file.endswith(".py") and file.find("__init__") == -1 and file[0] != ".":
                temp = directory_tree
                for curr_dir in os.path.relpath(path, base_path).split("/"):
                    temp = temp[curr_dir]
                flag = parseFile(
                    path, file, temp[file], directory_tree, lookuptable)
                if flag == False:
                    """
                    Python v3.4 file stored for next iteration
                    """
                    f.write(path + "/" + file + "\n")


def parseFile(path, file, file_dict, directory_tree, lookuptable):
    """
    Scans through the file and lists all the votes
    """
    if len(path) == 1:
        return True

    file_name = path + "/" + file
    with open(file_name, "r") as myfile:
        data = myfile.read()

    try:
        tree = ast.parse(data)
    except Exception as e:
        print(file_name)
        print(traceback.format_exc()) 
        return False
    
    imp_obj = FindImports(path, file)
    imp_obj.visit(tree)
    all_Imports = imp_obj.get_list()

#     all_Imports = resolveStars(all_Imports , directory_tree , lookuptable)

    func_obj = FindFuncs()
    func_obj.visit(tree)
    all_funcs = func_obj.get_list()
#
#     if file == "fakefile.py":
#         print(file_name)
#         print(yaml.dump(all_Imports , indent = 6, default_flow_style=False))
#         for ele in all_funcs:
#             print(ele)
#         exit(1)

    for function in all_funcs:
        if function[0] in all_Imports:
            function = all_Imports[function[0]].split("/") + function[1:]
            function = function[:-1]

            if len(function) == 0:
                """
                It is a local function or a function object
                but not a call
                e.g. copy() in ./PyML/utils/salstat_stats.py
                """
                continue

            if function[0] in directory_tree:
                """
                A root directory path
                """
                file_dict[
                    "/".join(function)] = file_dict.get("/".join(function), 0) + 1
            elif os.path.exists(path + "/" + function[0] + ".py") == True:
                """
                Import from same directory
                """
                file_dict[path + "/" + function[0] +
                          ".py"] = file_dict.get(path + "/" + function[0] + ".py", 0) + 1
            elif function[-1] in lookuptable:
                """
                Library/Installation referece without qualifier 
                or not in same directory
                e.g. import six in ./matplotlib/lib/matplotlib/tests/test_transforms.py
                """
                file_dict[lookuptable[function[-1]]
                          ] = file_dict.get(lookuptable[function[-1]], 0) + 1
            else:
                """
                Found nowhere, so add it to the generic list
                """
                file_dict["generic"] = file_dict.get("generic", 0) + 1
    return True


if __name__ == "__main__":
    include_list = ["math", "decimal", "datetime", "time", "re", "glob", "fnmatch",
                    "os", "tempfile", "shutil", "sqlite", "json",
                    "random", "logging", "urllib2"]
    directory_tree = dict()
    lookuptable = dict()
    path_crawler(".", directory_tree, lookuptable)
    generic_path_crawler(directory_tree, include_list, lookuptable)
    file_crawler(".", directory_tree, lookuptable)

    stream = open('document.yaml', 'w')
    stream2 = open('lookuptable.yaml', 'w')
    yaml.dump(directory_tree, stream, indent=6, default_flow_style=False)
    yaml.dump(lookuptable, stream2, indent=6, default_flow_style=False)
#     print(yaml.dump(directory_tree , indent = 6, default_flow_style=False))
#     base_library_path = os.path.dirname(os.__file__)
#     print(base_library_path)
#
#
#     f1 = "./scipy/tools/win32/detect_cpu_extensions_wine.py"
#     f2 = "./scipy/tools/win32/build_scripts/pavement.py"
#     with open(f1, "r" ) as myfile:
#         data = myfile.read()
#     tree = ast.parse(data)
#
#     fobj = FindImports()
#     fobj.visit(tree)
#     all_Imports = fobj.get_list()
#     print(yaml.dump(all_Imports, indent=6, default_flow_style=False))
#
#     fobj = FindFuncs()
#     fobj.visit(tree)
#     all_funcs = fobj.get_list()
#
# for i,line in enumerate(all_funcs):
# print(str(i) + "\t" + str(line))
