import os
import yaml
import ast
import crawlerModern

def add_entry(path, directory_tree):
    """
    Using temp to create new dictionary
    and then jump to that dictionary
    Equivalent to traversing a graph
    """
    temp = directory_tree
    for curr_dir in path.split("/"):
        temp[curr_dir] = temp.get(curr_dir, dict())
        temp = temp[curr_dir]
    return temp


def path_crawler(base_path, directory_tree):
    """
    Walks through all files in the directory
    """
    for path, dirs, files in os.walk(base_path):
        for file in files:
            if(file.endswith(".py")) and file.find("__init__") == -1:
                temp = add_entry(path, directory_tree)
                temp[file] = 0

def location(split_line, substr):
    """
    Function to determine locations of "as" in a list
    """
    locs = []
    for i in range(len(split_line)):
        if(split_line[i] == substr):
            locs.append(i)
    return locs

def sanitize(substr):
    """
    Function to remove leading "." in paths
    """
    if substr[0] == ".":
        substr = substr[1:]
    return substr


class ParseCall(ast.NodeVisitor):
    """
    Class and function to Parse AST
    and create a list for every func call
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
    Func Calls are controlled from here
    """
    
    def __init__(self):
        self.all_func = []
    
    def visit_Call(self, node):
        parser = ParseCall()
        parser.visit(node.func)
        self.all_func.append(parser.func_name)
        ast.NodeVisitor.generic_visit(self, node)
    
    def get_list(self):
        return self.all_func



# class ParseImports(ast.NodeVisitor):
#     """
#     Class and function to Parse AST
#     and create a list for every func call
#     """
#     def __init__(self):
#         self.import_name = []
#     
#     def visit_Import(self , node):
#         for alias in node.names:
#             if alias.asname==None:
#                 self.import_name.append({alias.name : alias.name})
#             else:
#                 self.import_name.append({alias.asname : alias.name})
#         ast.NodeVisitor.generic_visit(self, node)
#         
#     def visit_ImportFrom(self, node):
#         self.import_name.append(node.module)
#         for alias in node.names:
#             if alias.asname==None:
#                 self.import_name.append({alias.name : alias.name})
#             else:
#                 self.import_name.append({alias.asname : alias.  name})
#         ast.NodeVisitor.generic_visit(self, node)
# 
# class FindImports(ast.NodeVisitor):
#     def __init__(self):
#         self.all_class = []
#     
#     def visit_Import(self, node):
#         parser = ParseImports()
#         parser.visit(node)
#         self.all_class.append(parser.import_name)
#         ast.NodeVisitor.generic_visit(self, node)
#     
#     def visit_ImportFrom(self , node):
#         parser = ParseImports()
#         parser.visit(node)
#         self.all_class.append(parser.import_name)
#         ast.NodeVisitor.generic_visit(self, node)
# 
#     def get_list(self):
#         return self.all_class


class ParseImports(ast.NodeVisitor):
    """
    Class and function to Parse AST
    and create a list for every func call
    """
    def __init__(self , all_class):
        self.import_name = all_class
     
    def visit_Import(self , node):
        for alias in node.names:
            if alias.asname==None:
                self.import_name[alias.name] = alias.name.replace("." , "/")
            else:
                self.import_name[alias.asname] = alias.name.replace("." , "/")
        ast.NodeVisitor.generic_visit(self, node)
        
        
    def visit_ImportFrom(self, node):
        for alias in node.names:
            if alias.asname==None:
                if node.module == None:
                    self.import_name[alias.name] = "/".join(["." , alias.name])
                else :
                    self.import_name[alias.name] = "/".join([node.module.replace("." , "/") , alias.name])
            else:
                if node.module == None:
                    self.import_name[alias.asname] = "/".join(["." , alias.name])
                else :
                    self.import_name[alias.name] = "/".join([node.module.replace("." , "/") , alias.name])
        ast.NodeVisitor.generic_visit(self, node)


class FindImports(ast.NodeVisitor):
    def __init__(self):
        self.all_class = dict()
    
    def visit_Import(self, node):
        parser = ParseImports(self.all_class)
        parser.visit(node)
#         self.all_class.append(parser.import_name)
        ast.NodeVisitor.generic_visit(self, node)
    
    def visit_ImportFrom(self , node):
        parser = ParseImports(self.all_class)
        parser.visit(node)
#         self.all_class.append(parser.import_name)
        ast.NodeVisitor.generic_visit(self, node)

    def get_list(self):
        return self.all_class

if __name__ == "__main__":
#     directory_tree = dict()
#     path_crawler(".", directory_tree)
#     stream = open('document2.yaml', 'w')
#     print(
#         yaml.dump(directory_tree, stream, indent=6, default_flow_style=False))
#     f = open("/home/chettyharish/workspace/aalgo-4/numpy/numpy/polynomial/hermite.py")
#     
#     ftable = dict()
#     for line in f:
#         if "import " in line and ">>>" not in line:
#             """
#             For import statements at the top of file
#             """
#             split_line = line.split()
#             if  "from" not in line:
#                 """
#                 For statements which have a single "import"
#                 """
#                 if "as" in line:
#                     ftable[split_line[-1]] = split_line[1]
#             else:
#                 if "as" not in split_line:
#                     """
#                     For statements which use a from and do not have an "as" 
#                     Can be of the form from x import a,b,c
#                     """
#                     for i in range(3,len(split_line)):
#                         split_line[i] = split_line[i].replace(",","")
#                         ftable[split_line[i]] = sanitize(".".join([split_line[1],split_line[i]]))
#                 else:
#                     """
#                     For statements which use a from and use lots of "as" statements
#                     """
#                     for ele in location(split_line,"as"):
#                         ftable[split_line[ele+1]] = sanitize(".".join([split_line[1],split_line[ele - 1]]))
#         else:
#             """
#             Now the function calls inside the file
#             """
#             pass
#     print()
#     print(yaml.dump(ftable , indent = 6, default_flow_style=False))
#
    
# 
#     with open ("/home/chettyharish/workspace/aalgo-4/numpy/numpy/polynomial/hermite.py", "r") as myfile:
#         data=myfile.read()
        
    with open ("temp.py", "r") as myfile:
        data=myfile.read()

    tree = ast.parse(data)
    fobj = FindFuncs()
    fobj.visit(tree)
    all_funcs = fobj.get_list()
    
#     for i,line in enumerate(all_funcs):
#         print(str(i) + "\t" + str(line))
    
    fobj = FindImports()
    fobj.visit(tree)
    all_Imports = fobj.get_list()
    
    print(yaml.dump(all_Imports , indent = 6, default_flow_style=False))