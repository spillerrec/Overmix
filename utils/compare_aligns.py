import sys
from xml.dom import minidom

def parse(path):
	doc = minidom.parse(path)
	offsets = doc.getElementsByTagName('offset')
	return [(float(offset.attributes['x'].value), float(offset.attributes['y'].value)) for offset in offsets]
	
file1 = parse(sys.argv[1])
file2 = parse(sys.argv[2])

diff_x = [x[0]-y[0] for (x, y) in zip(file1, file2)]
diff_y = [x[1]-y[1] for (x, y) in zip(file1, file2)]
error_x = sum([abs(x) for x in diff_x])
error_y = sum([abs(y) for y in diff_y])
print(error_x, error_y)