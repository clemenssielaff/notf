import inspect
import notf


class Switcheroo:
    def __init__(self):
        self._lines = []

    def _cleanup(self, line):
        result = line.replace("notf.", "")
        result = result.replace("notf::", "")
        return result

    def produce(self):
        self.add_switcheroo()
        return "\n".join(self._lines)

    def add_switcheroo(self):
        self._lines.extend([
            "def _the_old_switcheroo():",
            "    \"\"\"",
            "    Nifty little hack, replacing all public Classes and Functions in the docwrapper with their actual NoTF",
            "    implementation without your IDE catching on ;)",
            "    \"\"\"",
            "    import sys",
            "    import os",
            "    sys.path.append(os.path.dirname(__file__))",
            "    replacements = __import__(\"notf\", globals(), locals(), ['*'], 0).__dict__",
            "    globals().update({{item: replacements[item] for item in replacements if not item.startswith('_')}})",
            "",
            "_the_old_switcheroo()",
            "",
        ])

    def add_enum(self, name, obj):
        self._lines.append("    class {}:".format(name))
        value = 0
        for enum in [member[0] for member in inspect.getmembers(obj) if not member[0].startswith('_')]:
            self._lines.append("        {} = {}".format(enum, value))
            value += 1
        self._lines.append("    ")

    def add_method(self, name, obj):
        docs = inspect.getdoc(obj)
        if not docs:
            self._lines.extend([
                "    def {}(self, *args, **kwargs):".format(name),
                "        pass",
                "    ",
            ])
            return
        doc_lines = []
        for line in [line for line in docs.split('\n') if len(line) > 0]:
            doc_lines.append(self._cleanup(line))

        # make sure to note the return type, even on overloaded functions (its always the same)
        return_type = ""
        if len(doc_lines) > 2 and doc_lines[1] == "Overloaded function.":
            return_type = doc_lines[2][doc_lines[2].rfind("->"):]

        self._lines.append("    def {}{}:".format(doc_lines[0], return_type))
        self._lines.append("        \"\"\"")
        for line in doc_lines[1:]:
            self._lines.append("        {}".format(line))
        self._lines.extend([
            "        \"\"\"",
            "        pass",
            "        ",
        ])

    def add_class(self, name, obj):
        self._lines.append("class {}:".format(name))
        member_count = 0
        for name, value in inspect.getmembers(obj):
            if name.startswith('_') and name != "__init__":
                continue
            member_count += 1
            if inspect.isbuiltin(value):
                self.add_method(name, value)
            elif inspect.isclass(value):
                self.add_enum(name, value)
            else:
                member_count -= 1
        if member_count == 0:
            self._lines.extend([
                "    pass",
                "    ",
            ])

    def add_function(self, name, obj):
        docs = inspect.getdoc(obj)
        if not docs:
            self._lines.extend([
                "def {}(*args, **kwargs):".format(name),
                "    pass",
                "",
            ])
            return

        # produce clean documentation
        doc_lines = []
        for line in [line for line in docs.split('\n') if len(line) > 0]:
            doc_lines.append(self._cleanup(line))

        # make sure to note the return type, even on overloaded functions (its always the same)
        return_type = ""
        if len(doc_lines) > 2 and doc_lines[1] == "Overloaded function.":
            return_type = doc_lines[2][doc_lines[2].rfind("->"):]

        self._lines.append("def {}{}:".format(doc_lines[0], return_type))
        self._lines.append("    \"\"\"")
        for line in doc_lines[1:]:
            self._lines.append("    {}".format(line))
        self._lines.extend([
            "    \"\"\"",
            "    pass",
            "    ",
        ])


def produce():
    import os
    switcheroo = Switcheroo()
    for keyword, obj in notf.__dict__.items():
        if keyword.startswith('__'):
            continue
        if inspect.isclass(obj):
            switcheroo.add_class(keyword, obj)
        elif inspect.isbuiltin(obj):
            switcheroo.add_function(keyword, obj)
    dirpath = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(dirpath, "notf.py"), 'w') as f:
        print("Writing 'notf.py' to {}".format(os.path.join(dirpath, "notf.py")))
        f.write(switcheroo.produce())
