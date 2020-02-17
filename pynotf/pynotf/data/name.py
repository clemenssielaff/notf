class Name:
    """
    A valid Name identifier in notf must not be empty and must not control any Path control characters.
    """

    def __init__(self, name: str):
        if not isinstance(name, str):
            raise NameError('Names must be strings')
        if name == "":
            raise NameError('Names may not be empty')
        for path_control_character in ('.', '/', ':'):
            if path_control_character in name:
                raise NameError('Names may not contain Path control characters dot [.], slash [/] and colon [:]')
        self._value: str = name

    def __repr__(self):
        return self._value
