
# reads particle data from ../output/particle_data.csv and 
# displays it in a graph

# https://stackoverflow.com/questions/21827515/plot-circles-with-matplotlib-from-text-file


import csv
# from Tkinter import Tk, Canvas, Frame, BOTH

import matplotlib.pyplot as plt

print 'hello brother'

particles = []

with open('../output/particle_data.csv', 'r') as csvfile:
    r = csv.reader(csvfile)
    for row in r:
        float_row = []
        for s in row:
            float_row.append(float(s))
        particles.append(float_row[:])
    xy = particles.pop(0)
    x_box = xy[0]
    y_box = xy[1]

for p in particles:
    circle = plt.Circle((p[0], p[1]), p[2])
    circle.set_facecolor('none')
    fig = plt.gcf()
    fig.gca().add_artist(circle)

xmin = 0
xmax = x_box
ymin = 0
ymax = y_box
plt.xlim(xmin, xmax)
plt.ylim(ymin, ymax)
plt.show()

# # https://stackoverflow.com/questions/9215658/plot-a-circle-with-pyplot
# circles = []
# for p in particles:
#     circles.append(plt.Circle((p[0], p[1]), p[2]))

# fig, ax = plt.subplots() # note we must use plt.subplots, not plt.subplot

# for c in circles:
#     ax.add_artist(c)

# plt.show()



# # print particles


# ## y is flipped
# class Example(Frame):
  
#     def __init__(self, parent):
#         Frame.__init__(self, parent)   
         
#         self.parent = parent        
#         self.initUI()
        
        
#     def initUI(self):

#         particles = [] # [[x, y, radius]...]
#         x_box = 0.
#         y_box = 0.

#         with open('../output/particle_data.csv', 'r') as csvfile:
#             r = csv.reader(csvfile)
#             for row in r:
#                 float_row = []
#                 for s in row:
#                     float_row.append(float(s))
#                 particles.append(float_row[:])
#             xy = particles.pop(0)
#             x_box = xy[0]
#             y_box = xy[1]
      
#         self.parent.title("Lines")        
#         self.pack(fill=BOTH, expand=1)

#         canvas = Canvas(self)

#         # 700 X 700 canvas. scale particles to fit
#         # may produce oval shaped "circles"
#         for p in particles:
#             x = (p[0] / x_box) * 700.
#             y = 700 - ((p[1] / y_box) * 700.)
#             x_rad = (p[2] / x_box) * 700.
#             y_rad = (p[2] / y_box) * 700.
#             canvas.create_oval(x - x_rad, y - y_rad, x + x_rad, y + y_rad)

#         # draw marking lines
#         for xx in x_box
        
#         canvas.pack(fill=BOTH, expand=1)




# root = Tk()
# ex = Example(root)
# root.geometry("700x700+0+0")
# root.mainloop() 

