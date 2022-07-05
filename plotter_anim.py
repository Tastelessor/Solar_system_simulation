import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d.axes3d as p3
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D
import random
import numpy as np
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
all_entries={}

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

def update_lines(num, keys, lines):
    global legend
    for line, key in zip(lines, keys):
        # NOTE: there is no .set_data() for 3 dim data...
        line.set_data(all_entries[key][0:2, :num])
        line.set_3d_properties(all_entries[key][2, :num])
        line.set_label(key)
    return lines

def setUpAnimation(frame_step=1):
  # Attaching 3D axis to the figure
  fig = plt.figure()
  ax = p3.Axes3D(fig)

  colours = ['r','b','g','y','m','c']

  max_range=0
  maxes=[]
  for current_key in histories.keys():
    entries=histories[current_key]
    all_entries[current_key]=np.empty((3, int(len(entries) / frame_step)))
    i=0
    total_count=0
    for entry in entries:
      if (total_count % frame_step == 0):
        all_entries[current_key][0, i]=float(entry.get_x())
        all_entries[current_key][1, i]=float(entry.get_y())
        all_entries[current_key][2, i]=float(entry.get_z())
        i+=1
      total_count+=1

    print(current_key)
    if all_entries[current_key].max() > max_range:
      max_range = all_entries[current_key].max()


  lines = [ax.plot(all_entries[current_key][0, 0:1], all_entries[current_key][1, 0:1], all_entries[current_key][2, 0:1], c = random.choice(colours))[0] for current_key in histories.keys()]

  # Setting the axes properties
  ax.set_xlim([-max_range,max_range])    
  ax.set_ylim([-max_range,max_range])
  ax.set_zlim([-max_range,max_range])

  # Creating the Animation object
  line_ani = animation.FuncAnimation(fig, update_lines, None, fargs=(histories.keys(), lines), interval=1, blit=False)

  plt.show()

if (len(sys.argv) == 3):
  parse_input_file(sys.argv[1])
  setUpAnimation(int(sys.argv[2]))
else:
  print("Error: Must provide cosmology output file and frame step size (in number of frames) as command line arguments")