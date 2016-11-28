import inspect
import notf

class Switcheroo:
    def __init__(self):
        self._lines = []

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

    def add_method(self, name, obj):
        docs = inspect.getdoc(obj)
        if not docs:
            self._lines.extend([
"    def {}(self):".format(name),
"        pass",
"    ",
            ])
            return
        doc_lines = [line for line in docs.split('\n') if len(line) > 0]
        self._lines.append(
"    def {}:".format(doc_lines[0]))
        self._lines.append(
"        \"\"\"",
            )
        for line in doc_lines[1:]:
            self._lines.append(
"        {}".format(line),
            )
        self._lines.extend([
"        \"\"\"",
"        pass",
"        ",
        ])

    def add_class(self, name, obj):
        self._lines.extend([
"class {}:".format(name),
        ])
        member_count = 0
        for name, value in inspect.getmembers(obj):
            if name.startswith('_'):
                continue
            if inspect.isbuiltin(value):
                member_count += 1
                self.add_method(name, value)
        if member_count == 0:
            self._lines.extend([
"    pass",
"    ",
            ])

def produce():
    import os
    switcheroo = Switcheroo()
    for keyword, obj in notf.__dict__.items():
        if keyword.startswith('_'):
            continue
        if inspect.isclass(obj):
            switcheroo.add_class(keyword, obj)
    dirpath = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(dirpath, "notf.py"), 'w') as f:
        print("Writing 'notf.py' to {}".format(os.path.join(dirpath, "notf.py")))
        f.write(switcheroo.produce())
