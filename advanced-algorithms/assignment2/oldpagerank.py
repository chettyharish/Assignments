import os
import ast
import sys
import numpy
from numpy import matrix
from operator import itemgetter
import sqlite3


def missing_error(file, module, alias, loc):
    """
    C2 has lot of generics due to file names without ".py"
    C5 has lot of generics fue to orange usin "." for a path from base!
    """
    supress = "A1 A2 A3 A4 A5 B1 B2 B3 B4 B5 B6 B7 B8 B9 C1 C2 C3 C4 C5 C6 C7 C8 C9 C10 C11".split()
    if loc not in supress:
        print("ERROR at location " + str(loc) + " in file " + str(file)
              + " due to module :" + str(module) + " with alias " + str(alias))
    pass


def listjoin(votes, bfs_result):
    """
    Function to append a list into list
    without creating a new list!
    """
    for ele in bfs_result:
        votes.append(ele)


def bfs(temp, base_module, path, file, votes, directory_tree, include_list,
        lookuptable, path_to_index, index_to_path, one_level_down):
    """
    Function to carry out a bfs so that we can account for all votes
    in directory and sub-directories.
    Returns a list of number using lookuptable and stuff
    Takes in the dict() where we start the bfs
    """
    queue = [temp]
    output = []
    path_split = path.split("/")

    while len(queue) > 0:
        for key in queue[0].keys():
            if isinstance(queue[0].get(key), dict):
                queue.append(queue[0].get(key))
            else:
                output.append(key.split(".")[0])
        queue.pop(0)

    vote_list = [path_to_index[lookuptable[base_module][x]] for x in output]
    return vote_list


class ParseImports(ast.NodeVisitor):

    """
    A single import node is parsed and added to main dictionary
    """

    def __init__(self, path, file, votes, directory_tree, include_list,
                 lookuptable, path_to_index, index_to_path, one_level_down):
        self.votes = votes
        self.path = path
        self.file = file
        self.directory_tree = directory_tree
        self.include_list = include_list
        self.lookuptable = lookuptable
        self.path_to_index = path_to_index
        self.index_to_path = index_to_path
        self.one_level_down = one_level_down

    def visit_Import(self, node):
        for alias in node.names:
            split = alias.name.split(".")
            path_split = self.path.split("/")
            if alias.name in include_list:
                if isinstance(lookuptable[alias.name], str):
                    """
                    Part of include list and is a file in the library
                    So appending votes by getting the index from the path
                    and getting the path from the lookuptable
                    """
                    if alias.name == "os":
                        self.votes.append(
                            path_to_index[lookuptable["os.path"]])
                    self.votes.append(path_to_index[lookuptable[alias.name]])
                else:
                    """
                    Part of include list and is a directory in the library,
                    so adding the submodules in the dict()
                    e.g. logging , urllib, json and I guess sqlite3
                    """
                    for ele in lookuptable[alias.name]:
                        self.votes.append(
                            path_to_index[lookuptable[alias.name][ele]])
            elif split[0] in directory_tree:
                """
                The Import is not of builtin modules, need to look into 
                all the downloaded modules
                """
                if split[0] in directory_tree[split[0]]:
                    """
                    Case when we have the base directory and an inside directory 
                    with the same name . So we have to start from the inside dict()
                    e.g. numpy -> numpy
                    """
                    base_module = split[0]
                    temp = directory_tree[split[0]]
                    for ele in split[:-1]:
                        if ele in temp:
                            temp = temp[ele]
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "A1")
                            return
                    ele = split[-1]
                    if ele in temp:
                        bfs_result = bfs(directory_tree[split[0]][split[0]], split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                         self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                        listjoin(self.votes, bfs_result)
                    elif ele + ".py" in temp:
                        self.votes.append(
                            path_to_index[lookuptable[base_module][ele]])
                    elif split[-1] in lookuptable[base_module]:
                        """
                        Maybe it is somewhere inside the modules
                        Using the lookuptable to check only inside the module!
                        """
                        self.votes.append(
                            path_to_index[lookuptable[path_split[1]][split[-1]]])
                    else:
                        self.votes.append(
                            path_to_index[lookuptable["generic"]])
#                         print("SOME IMPORT WITHOUT FILE FOUND!!!")
                        return
                else:
                    """
                    We have to start from the outside dict()
                    """
                    if len(split) == 1:
                        bfs_result = bfs(directory_tree[split[0]], split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                         self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                        listjoin(self.votes, bfs_result)
                    else:
                        base_module = split[0]
                        temp = directory_tree[split[0]]
                        for ele in split[1:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                if split[-1] in lookuptable[split[0]]:
                                    self.votes.append(
                                        path_to_index[lookuptable[split[0]][split[-1]]])
                                    return
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "A2")
                                return
                        ele = split[-1]
                        if ele in temp:
                            bfs_result = bfs(directory_tree[split[0]], split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[base_module][ele]])
                        elif split[-1] in lookuptable[base_module]:
                            """
                            Files when accessed in the form:
                            matplotlib.pylab
                            """
                            self.votes.append(
                                path_to_index[lookuptable[base_module][split[-1]]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "A3")
                            return

            elif split[0] in one_level_down:
                """
                Maybe the base directory has a different name!
                So lets look one level down to see if we can find it
                """
                if len(one_level_down[split[0]]) == 1:
                    base_module = one_level_down[split[0]][0]
                    temp = directory_tree[base_module]
                    for ele in split[:-1]:
                        if ele in temp:
                            temp = temp[ele]
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "A4")
                            return
                    ele = split[-1]
                    if ele in temp:
                        bfs_result = bfs(temp[ele], one_level_down[split[0]][0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                         self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                        listjoin(self.votes, bfs_result)
                    elif ele + ".py" in temp:
                        self.votes.append(
                            path_to_index[lookuptable[base_module][ele]])
                    else:
                        self.votes.append(
                            path_to_index[lookuptable["generic"]])
                        missing_error(
                            self.path + "/" + self.file, alias.name, alias.name, "A5")
                        return
            elif split[-1] in lookuptable[path_split[1]]:
                """
                Maybe it is somewhere inside the modules
                Using the lookuptable to check only inside the module!
                """
                self.votes.append(
                    path_to_index[lookuptable[path_split[1]][split[-1]]])
            else:
                """
                Neither is the builtins nor in the downloaded modules
                Pushing it to generic
                """
                self.votes.append(path_to_index[lookuptable["generic"]])
        ast.NodeVisitor.generic_visit(self, node)

    def visit_ImportFrom(self, node):
        for alias in node.names:
            level = node.level
            path_split = self.path.split("/")
            alias_split = alias.name.split(".")

            if level - 1 > 0:
                """
                Since the first dot in the module signifies same directory
                """
                module_path = path_split[1:- (level - 1)]
            else:
                module_path = path_split[1:]
            if node.module != None:
                module_split = node.module.split(".")

            if node.module == "os.path":
                self.votes.append(path_to_index[lookuptable["os.path"]])
                return
            if node.module == "os":
                self.votes.append(path_to_index[lookuptable["os"]])
                self.votes.append(path_to_index[lookuptable["os.path"]])
                return
            if alias.name == "*":
                """
                Condition where we import all the functions from a module
                (e.g. from numpy import *)
                """
                if level > 0:
                    """
                    Direct address provided , get the path from the levels!
                    """
                    if node.module == None:
                        """
                        Case where module is . or ..
                        Case never happens with a *
                        e.g. from . import *
                        e.g. from .. import *
                        Must be a directory as there is no file name anywhere!
                        """
                        temp = directory_tree
                        for ele in module_path[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "B1")
                                return
                            bfs_result = bfs(temp, module_path[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        return
                    else:
                        """
                        Import of the form
                        from .linalg import *
                        from ..linalg import *
                        """
                        listjoin(module_path, module_split)
                        temp = directory_tree
                        for ele in module_path[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "B2")
                                return
                        ele = module_path[-1]
                        if ele in temp:
                            bfs_result = bfs(temp, module_path[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            pass
                            self.votes.append(
                                path_to_index[lookuptable[module_path[0]][ele]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "B3")
                        return
                elif module_split[0] in self.directory_tree:
                    """
                    Module in directory_tree
                    """
                    if module_split[0] in directory_tree[module_split[0]]:
                        """
                        Case when we have the base directory and an inside directory 
                        with the same name . So we have to start from the inside dict()
                        e.g. numpy -> numpy
                        """
                        base_module = module_split[0]
                        temp = directory_tree[module_split[0]]
                        for ele in module_split[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, node.module, alias.name, "B4")
                                return
                        ele = module_split[-1]
                        if ele in temp:
                            bfs_result = bfs(directory_tree[module_split[0]][module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[base_module][ele]])
                        elif module_split[-1] in lookuptable[base_module]:
                            """
                            Maybe it is somewhere inside the modules
                            Using the lookuptable to check only inside the module!
                            """
                            self.votes.append(
                                path_to_index[lookuptable[path_split[1]][module_split[-1]]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, node.module, alias.name, "B5")
                            return
                    else:
                        """
                        We have to start from the outside dict()
                        """
                        if len(module_split) == 1:
                            bfs_result = bfs(directory_tree[module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        else:
                            base_module = module_split[0]
                            temp = directory_tree[module_split[0]]
                            for ele in module_split[1:-1]:
                                if ele in temp:
                                    temp = temp[ele]
                                else:
                                    self.votes.append(
                                        path_to_index[lookuptable["generic"]])
                                    missing_error(
                                        self.path + "/" + self.file, node.module, alias.name, "B6")
                                    return
                            ele = module_split[-1]
                            if ele in temp:
                                bfs_result = bfs(directory_tree[module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                                 self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                                listjoin(self.votes, bfs_result)
                            elif ele + ".py" in temp:
                                self.votes.append(
                                    path_to_index[lookuptable[base_module][ele]])
                            elif module_split[-1] in lookuptable[base_module]:
                                """
                                Files when accessed in the form:
                                matplotlib.pylab
                                """
                                self.votes.append(
                                    path_to_index[lookuptable[base_module][module_split[-1]]])
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, node.module, alias.name, "B7")
                                return
                elif module_split[0] in one_level_down:
                    """
                    Maybe the base directory has a different name!
                    So lets look one level down to see if we can find it
                    """
                    if len(one_level_down[module_split[0]]) == 1:
                        base_module = one_level_down[module_split[0]][0]
                        temp = directory_tree[base_module]
                        for ele in module_split[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, node.module, alias.name, "B8")
                                return
                        ele = module_split[-1]
                        if ele in temp:
                            bfs_result = bfs(temp[ele], one_level_down[module_split[0]][0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[base_module][ele]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, node.module, alias.name, "B9")
                            return
                elif module_split[-1] in lookuptable[path_split[1]]:
                    """
                    Maybe it is somewhere inside the modules
                    Using the lookuptable to check only inside the module!
                    """
                    self.votes.append(
                        path_to_index[lookuptable[path_split[1]][module_split[-1]]])
                else:
                    """
                    Neither is the builtins nor in the downloaded modules
                    Pushing it to generic
                    """
                    self.votes.append(path_to_index[lookuptable["generic"]])
            else:
                """
                The Second part begins!!! () :(
                Conditions where there is no * ,
                so it is always to import a file
                """
                if level > 0:
                    """
                    Direct address provided , get the path from the levels!
                    """
                    if node.module == None:
                        """
                        Case where module is . or ..
                        Case never happens with a *
                        e.g. from . import linalg
                        e.g. from .. import numpy
                        Must be a directory as there is no file name anywhere!
                        """
                        listjoin(module_path, alias_split)
                        temp = directory_tree
                        for ele in module_path[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "C1")
                                return
                        ele = module_path[-1]
                        if ele in temp:
                            bfs_result = bfs(temp, module_path[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                            return
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[module_path[0]][ele]])
                            return
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "C2")
                            return
                        return
                    else:
                        """
                        Import of the form
                        from .linalg import linalg
                        from ..linalg import numpy
                        from ..linalg import SchemeArrowAnnotation, SchemeTextAnnotation
                        """
                        listjoin(module_path, module_split)

                        temp = directory_tree
                        for ele in module_path[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "C3")
                                return
                        ele = module_path[-1]
                        if ele in temp:
                            bfs_result = bfs(temp, module_path[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[module_path[0]][ele]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "C5")
                        return
                elif module_split[0] in self.directory_tree:
                    """
                    Module in directory_tree
                    """
                    if module_split[0] in directory_tree[module_split[0]]:
                        """
                        Case when we have the base directory and an inside directory 
                        with the same name . So we have to start from the inside dict()
                        e.g. numpy -> numpy
                        """
                        base_module = module_split[0]
                        temp = directory_tree[module_split[0]]
                        for ele in module_split[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "C6")
                                return
                        ele = module_split[-1]
                        if ele in temp:
                            bfs_result = bfs(directory_tree[module_split[0]][module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[base_module][ele]])
                        elif module_split[-1] in lookuptable[base_module]:
                            """
                            Maybe it is somewhere inside the modules
                            Using the lookuptable to check only inside the module!
                            """
                            self.votes.append(
                                path_to_index[lookuptable[base_module][module_split[-1]]])
                        else:
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "C7")
                            return
                    else:
                        """
                        We have to start from the outside dict()
                        """
                        if len(module_split) == 1:
                            bfs_result = bfs(directory_tree[module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        else:
                            base_module = module_split[0]
                            temp = directory_tree[module_split[0]]
                            for ele in module_split[1:-1]:
                                if ele in temp:
                                    temp = temp[ele]
                                else:
                                    if module_split[-1] in lookuptable[module_split[0]]:
                                        self.votes.append(
                                            path_to_index[lookuptable[module_split[0]][module_split[-1]]])
                                    return
                                    self.votes.append(
                                        path_to_index[lookuptable["generic"]])
                                    missing_error(
                                        self.path + "/" + self.file, alias.name, alias.name, "C8")
                                    return
                            ele = module_split[-1]
                            if ele in temp:
                                bfs_result = bfs(directory_tree[module_split[0]], module_split[0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                                 self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                                listjoin(self.votes, bfs_result)
                            elif ele + ".py" in temp:
                                self.votes.append(
                                    path_to_index[lookuptable[base_module][ele]])
                            elif module_split[-1] in lookuptable[base_module]:
                                """
                                Files when accessed in the form:
                                matplotlib.pylab
                                """
                                self.votes.append(
                                    path_to_index[lookuptable[base_module][module_split[-1]]])
                            else:
                                if module_split[-1] in lookuptable[module_split[0]]:
                                    self.votes.append(
                                        path_to_index[lookuptable[module_split[0]][module_split[-1]]])
                                return
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "C9")
                                return
                elif module_split[0] in one_level_down:
                    """
                    Maybe the base directory has a different name!
                    So lets look one level down to see if we can find it
                    """
                    if len(one_level_down[module_split[0]]) == 1:
                        base_module = one_level_down[module_split[0]][0]
                        temp = directory_tree[base_module]
                        for ele in module_split[:-1]:
                            if ele in temp:
                                temp = temp[ele]
                            else:
                                if module_split[-1] in lookuptable[base_module]:
                                    self.votes.append(
                                        path_to_index[lookuptable[base_module][module_split[-1]]])
                                    return
                                self.votes.append(
                                    path_to_index[lookuptable["generic"]])
                                missing_error(
                                    self.path + "/" + self.file, alias.name, alias.name, "C10")
                                return
                        ele = module_split[-1]
                        if ele in temp:
                            bfs_result = bfs(temp[ele], one_level_down[module_split[0]][0], self.path, self.file, self.votes, self.directory_tree, self.include_list,
                                             self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
                            listjoin(self.votes, bfs_result)
                        elif ele + ".py" in temp:
                            self.votes.append(
                                path_to_index[lookuptable[base_module][ele]])
                        else:
                            if module_split[-1] in lookuptable[base_module]:
                                self.votes.append(
                                    path_to_index[lookuptable[base_module][module_split[-1]]])
                                return
                            self.votes.append(
                                path_to_index[lookuptable["generic"]])
                            missing_error(
                                self.path + "/" + self.file, alias.name, alias.name, "C11")
                            return
                elif module_split[-1] in lookuptable[path_split[1]]:
                    """
                    Maybe it is somewhere inside the modules
                    Using the lookuptable to check only inside the module!
                    """
                    self.votes.append(
                        path_to_index[lookuptable[path_split[1]][module_split[-1]]])
                else:
                    """
                    Neither is the builtins nor in the downloaded modules
                    Pushing it to generic
                    """
                    self.votes.append(path_to_index[lookuptable["generic"]])
        ast.NodeVisitor.generic_visit(self, node)


class FindImports(ast.NodeVisitor):

    """
    Import Calls (Votes) are added to the main set
    """

    def __init__(self, path, file, votes, directory_tree, include_list,
                 lookuptable, path_to_index, index_to_path, one_level_down):
        self.votes = votes
        self.path = path
        self.file = file
        self.directory_tree = directory_tree
        self.include_list = include_list
        self.lookuptable = lookuptable
        self.path_to_index = path_to_index
        self.index_to_path = index_to_path
        self.one_level_down = one_level_down

    def visit_Import(self, node):
        parser = ParseImports(self.path, self.file, self.votes, self.directory_tree, self.include_list,
                              self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)

    def visit_ImportFrom(self, node):
        parser = ParseImports(self.path, self.file, self.votes, self.directory_tree, self.include_list,
                              self.lookuptable, self.path_to_index, self.index_to_path, self.one_level_down)
        parser.visit(node)
        ast.NodeVisitor.generic_visit(self, node)


def get_keys_level_down(directory_tree):
    """
    Gives us  a dict() of keys which are one level down in directory_tree
    Necessary to resolve cases when 
    numpy.numpy.linalg is imported as numpy.linalg
    """
    base = dict()
    for key in directory_tree.keys():
        temp = directory_tree.get(key)
        for inkey in temp:
            if isinstance(temp.get(inkey), dict):
                if inkey in base:
                    base[inkey].append(key)
                else:
                    base[inkey] = [key]
    return base


def get_all_module_files(lookuptable, module):
    """
    Function creates a list of all files under a module
    which is used for resolving random imports
    """
    module_files = list(lookuptable[module].keys())
    return module_files


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


def path_crawler(base_path, directory_tree, lookuptable, path_to_index, index_to_path):
    """
    Walks through all files in the directory
    listing them
    """
    index = len([x for x in index_to_path.keys()])
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1
                    and file[0] != "." and len(path.split("/")) > 1):
                temp = add_entry(
                    os.path.relpath(path, base_path), directory_tree)
                temp[file] = []
                lookuptable[
                    path.split("/")[1]] = lookuptable.get(path.split("/")[1], dict())
                lookuptable[
                    path.split("/")[1]][file.split(".")[0]] = path + "/" + file
                path_to_index[path + "/" + file] = index
                index_to_path[index] = path + "/" + file
                index += 1


def generic_path_crawler(directory_tree, include_list, lookuptable,
                         path_to_index, index_to_path, path_to_module_generic):
    """
    Simply adds the file names for now
    and looks in sys.path for the source libraries (if available)
    """
    index = len([x for x in index_to_path.keys()])
    for ele in include_list:
        __import__(ele)
        if __file__ not in dir(sys.modules.get(ele)):
            path = "builtin/" + ele + ".py"
            directory_tree[ele + ".py"] = []
            lookuptable[ele] = path
            path_to_index[path] = index
            index_to_path[index] = path
            path_to_module_generic[index] = ele
            index += 1
            continue
        path = sys.modules.get(ele).__file__
        base_path = "/".join(path.split("/")[:-1])
        folder_path = "/".join(path.split("/")[:-2])

        if path.split("/")[-1] == "__init__.py":
            for path, dirs, files in os.walk(base_path):
                for file in files:
                    if (file.endswith(".py") and file.find("__init__") == -1
                            and file[0] != "." and len(path.split("/")) > 1):
                        temp = add_entry(
                            os.path.relpath(path, folder_path), directory_tree)
#                         temp[file] = dict()
                        temp[file] = []
                        lookuptable[ele] = lookuptable.get(ele, dict())
                        lookuptable[ele][
                            file.split(".")[0]] = path + "/" + file
                        path_to_index[path + "/" + file] = index
                        index_to_path[index] = path + "/" + file
                        path_to_module_generic[
                            index] = ele + "." + str(file.split(".")[0])
                        index += 1
        else:
            directory_tree[ele + ".py"] = []
            lookuptable[ele] = path
            path_to_index[path] = index
            index_to_path[index] = path
            path_to_module_generic[index] = ele
            index += 1


def file_crawler(base_path, directory_tree, include_list,
                 lookuptable, path_to_index, index_to_path, one_level_down):
    """
    Walks through all files and parses them for votes!
    """
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1
                    and file[0] != "." and len(path.split("/")) > 1):
                temp = directory_tree
                for curr_dir in os.path.relpath(path, base_path).split("/"):
                    temp = temp[curr_dir]
                flag = parseFile(
                    path, file, temp[file], directory_tree, include_list,
                    lookuptable, path_to_index, index_to_path, one_level_down)


def parseFile(path, file, votes, directory_tree, include_list,
              lookuptable, path_to_index, index_to_path, one_level_down):
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

    imp_obj = FindImports(path, file, votes, directory_tree, include_list,
                          lookuptable, path_to_index, index_to_path, one_level_down)
    imp_obj.visit(tree)


def outlinks_walker(base_path, outlinks, directory_tree, include_list,
                    lookuptable, path_to_index, index_to_path, one_level_down):
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if (file.endswith(".py") and file.find("__init__") == -1
                    and file[0] != "." and len(path.split("/")) > 1):
                file_path = path + "/" + file
                split = file_path.split("/")[1:]
                temp = directory_tree
                for ele in split[:-1]:
                    temp = temp[ele]
#                 print(temp.keys())
                outlinks[path_to_index[file_path]] = temp[file]

    for ele in include_list:
        if isinstance(lookuptable[ele], dict):
            for inele in lookuptable[ele]:
                outlinks[path_to_index[lookuptable[ele][inele]]] = []
        else:
            outlinks[path_to_index[lookuptable[ele]]] = []


class Graph:

    def __init__(self, num_nodes):
        self.in_links = dict()
        self.out_links = dict()
        self.dangling_modules = dict()
        for i in range(num_nodes):
            self.in_links[i] = []
            self.out_links[i] = []
            self.dangling_modules[i] = 1


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

if __name__ == "__main__":
    
    if not (os.path.exists("__init__.py")):
        f= open("__init__py" , "w")

    threshold = 1e-7
    include_list = ["math", "decimal", "datetime", "time", "re", "glob", "fnmatch",
                    "os", "tempfile", "shutil", "json", "os.path", "sqlite3"
                    "random", "logging", "urllib"]
    lookuptable = dict()
    directory_tree = dict()
    path_to_index = dict()
    index_to_path = dict()
    outlinks = dict()
    path_to_module_generic = dict()
#     The default generic.py entry , it will be a dangling pointer:
    directory_tree["generic.py"] = []
    lookuptable["generic"] = "generic"
    path_to_index["generic"] = 0
    index_to_path[0] = "generic"

    print("Starting crawler")
    base_path = "."
    path_crawler(
        base_path, directory_tree, lookuptable, path_to_index, index_to_path)
    generic_path_crawler(
        directory_tree, include_list, lookuptable, path_to_index, index_to_path, path_to_module_generic)

    one_level_down = get_keys_level_down(directory_tree)
    file_crawler(base_path, directory_tree, include_list,
                 lookuptable, path_to_index, index_to_path, one_level_down)

    outlinks_walker(base_path, outlinks, directory_tree, include_list,
                    lookuptable, path_to_index, index_to_path, one_level_down)
    outlinks[0] = []

    print("Initializing Graph")
    num_nodes = len(outlinks)
    s = 0.85
    t = 1 - s

    graph = Graph(num_nodes)
    initialize_graph(graph, outlinks)

    print("Calculating pagerank")
    prlist = pagerank(graph, s, t, threshold, num_nodes)
    prlist = numpy.array(prlist).flatten().tolist()
    f2 = open("pagerank_modules.csv", "w")
    f3 = open("pagerank_packages.csv", "w")

    print("Writing module pageranks written in pagerank_modules.csv")
    for i, ele in enumerate(prlist):
        if i in path_to_module_generic:
            name = path_to_module_generic[i]
            f2.write(name + "," + str(ele) + "\n")
        else:
            name = str(index_to_path[i]).replace("/", ".").lstrip(".")
            f2.write(name + "," + str(ele) + "\n")

    print("Writing package pageranks written in pagerank_packages.csv")
    for package in lookuptable:
        package_pr = 0.0
        if package == "generic":
            package_pr += prlist[0]
            f3.write(package + "," + str(package_pr) + "\n")
        elif package in include_list:
            if isinstance(lookuptable[package], dict):
                for modules in lookuptable[package]:
                    path = lookuptable[package][modules]
                    package_pr += prlist[path_to_index[path]]
                f3.write(package + "," + str(package_pr) + "\n")
            else:
                path = lookuptable[package]
                if package == "os":
                    package_pr += prlist[path_to_index[lookuptable["os"]]]
                    package_pr += prlist[path_to_index[lookuptable["os.path"]]]
                    f3.write(package + "," + str(package_pr) + "\n")
                elif package == "os.path":
                    """
                    os.path is absorbed inside os!
                    """
                    pass
                else:
                    package_pr += prlist[path_to_index[path]]
                    f3.write(package + "," + str(package_pr) + "\n")
        else:
            """
            The downloaded modules
            """
            for modules in lookuptable[package]:
                path = lookuptable[package][modules]
                package_pr += prlist[path_to_index[path]]
            f3.write(package + "," + str(package_pr) + "\n")
    print("Done")
