import matplotlib.pyplot as plot
from mpl_toolkits.mplot3d import Axes3D
import random
import sys

class point_in_space:
  def __init__(self):
    self.x=0
    self.y=0
    self.z=0

  def set_x(self, x):
    self.x=x

  def set_y(self, y):
    self.y=y

  def set_z(self, z):
    self.z=z

  def get_x(self):
    return self.x

  def get_y(self):
    return self.y

  def get_z(self):
    return self.z

histories={}

def parse_input_file(filename):
  f = open(filename, "r")
  for line in f.readlines():
    body_name=line.split("_")[0]
    if body_name not in histories.keys():
      histories[body_name]=[]
    component=line.split("_")[1].split("=")[0]
    val=line.split("=")[1]
    if (component == "x"):
      pt=point_in_space()
      pt.set_x(val)
      histories[body_name].append(pt);
    elif (component == "y"):
      histories[body_name][-1].set_y(val)
    elif (component == "z"):
      histories[body_name][-1].set_z(val)
  f.close()

def plot_output(outfile = 'img2.png'):
    fig = plot.figure()
    colours = ['r','b','g','y','m','c']
    ax = fig.add_subplot(1,1,1, projection='3d')
    max_range = 0
    for current_key in histories.keys():
        entries=histories[current_key]
        x_entries=[]
        y_entries=[]
        z_entries=[]
        for entry in entries:
          x_entries.append(float(entry.get_x()))
          y_entries.append(float(entry.get_y()))
          z_entries.append(float(entry.get_z()))
        max_dim = max(max(x_entries),max(y_entries),max(z_entries))
        if max_dim > max_range:
            max_range = max_dim
        ax.plot(x_entries, y_entries, z_entries, c = random.choice(colours), label = current_key)        
    
    ax.set_xlim([-max_range,max_range])    
    ax.set_ylim([-max_range,max_range])
    ax.set_zlim([-max_range,max_range])
    ax.legend()        

    if outfile:
        plot.savefig(outfile)
    else:
        plot.show()

if (len(sys.argv) == 2):
  parse_input_file(sys.argv[1])
  plot_output()
else:
  print("Error: Must provide cosmology output file as command line argument")