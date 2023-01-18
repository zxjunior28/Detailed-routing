import matplotlib.pyplot as plt
import matplotlib.path as mpath
import sys

def draw_polyline(start_x, start_y, delta_x, delta_y, c):
    Path = mpath.Path
    path_data = [
    (Path.MOVETO, (start_x, start_y)),
    (Path.MOVETO, (start_x + delta_x, start_y + delta_y)),
    ]
    codes, verts = zip(*path_data)
    path = mpath.Path(verts, codes)
    x, y = zip(*path.vertices)
    line, = ax.plot(x, y, color=c, lw=3)

def get_cmap(n, name='hsv'):
    return plt.cm.get_cmap(name, n)

class Point :
    x = 0
    y = 0
    def __init__(self, x, y):
        self.x = x
        self.y = y


class Edge :
    p1 = Point(0, 0)
    p2 = Point(0, 0)
    def __init__(self, p1, p2):
        self.p1 = p1
        self.p2 = p2

system_arg = sys.argv
input_file = system_arg[1]

edge_list = dict()

# parser
fread = open(input_file,'r')
f = fread.read().split("\n")
net_num = 0
for line in f :
    line_split = line.split(" ")
    if line_split[0] == ".begin":
        net_num = line_split[1]
        edge_list[net_num] = list()
    if line_split[0] == ".V":
        p1 = Point(line_split[1], line_split[2])
        p2 = Point(line_split[1], line_split[3])
        edge_list[net_num].append(Edge(p1, p2))
    if line_split[0] == ".H":
        p3 = Point(line_split[1], line_split[2])
        p4 = Point(line_split[3], line_split[2])
        edge_list[net_num].append(Edge(p3, p4))

# draw
fig, ax = plt.subplots(1,1, figsize = (8,8))
plt.subplots_adjust(right=0.9)
cmap = get_cmap(len(edge_list))
i = 0
key_list = list()
for key, value in edge_list.items():
    plt.plot(0, 0, color=cmap(i), label = key) 
    for edge in value:
        draw_polyline(int(edge.p1.x), int(edge.p1.y),
        int(edge.p2.x)-int(edge.p1.x), int(edge.p2.y)-int(edge.p1.y), cmap(i))
    i = i + 1

plt.legend(loc=(1.01, 0)) 
plt.savefig('test.png')