from xml.etree import ElementTree as _ET


class _ElementTreeWrapper:
    def __init__(self, tree):
        self._tree = tree

    def xinclude(self):
        return None

    def getroot(self):
        return self._tree.getroot()

    def __getattr__(self, name):
        return getattr(self._tree, name)


def parse(source, parser=None):
    return _ElementTreeWrapper(_ET.parse(source, parser=parser))


def tostring(element, *args, **kwargs):
    if isinstance(element, _ElementTreeWrapper):
        element = element.getroot()
    return _ET.tostring(element, *args, **kwargs)


Element = _ET.Element
SubElement = _ET.SubElement
Comment = _ET.Comment
ProcessingInstruction = _ET.ProcessingInstruction
fromstring = _ET.fromstring
