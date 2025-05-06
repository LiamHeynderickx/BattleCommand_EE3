#online sources used
#https://github.com/neemiasbsilva/object-detection-opencv
#https://www.geeksforgeeks.org/opencv-python-tutorial/

#Chat gpt was used to generate portions of code that was then adjusted and edited according to 
# our needs

import cv2
import numpy as np
import argparse
import json

#Parse command-line argument for image path.
parser = argparse.ArgumentParser(description="Process board image to JSON")
parser.add_argument("--image", required=True, help="Path to input image")
args = parser.parse_args()
#fetch image
imageName = args.image  # Use the image path provided
image = cv2.imread(imageName)
image = cv2.flip(image, 1)
#array to store boat info
boats_data = []
#Convert image to HSV
hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

#Detect corners of board (red, may change)
#this defines the range for red we are searching for
#test pics in GT to see how the lighting affects the performance.
lower_red_1 = np.array([0, 60, 60])
upper_red_1 = np.array([20, 255, 255])

lower_red_2 = np.array([140, 60, 60])
upper_red_2 = np.array([180, 255, 255])

#red has 2 colour ranges so we create a mask for both
#mask sets all red pixels to white and all other pixels to black (0)
mask1 = cv2.inRange(hsv, lower_red_1, upper_red_1)
mask2 = cv2.inRange(hsv, lower_red_2, upper_red_2)
mask = cv2.bitwise_or(mask1, mask2)  # OR combines masks
contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)  # finds boundaries of regions in mask

#get central coordinates of red regions, these will be used as corners of the grid. (red dots on board)
coordinates = []
for contour in contours:
    if cv2.contourArea(contour) > 10:  # Ignore small red areas (adjust according to more testing) (10 to 15 is best)
        M = cv2.moments(contour)
        if M["m00"] != 0:
            cx = int(M["m10"] / M["m00"])
            cy = int(M["m01"] / M["m00"])
            coordinates.append((cx, cy))

#defines fixed board size. (not sure if this is ideal but let's see)
#used to re-map corner positions for determining the grid
width, height = 500, 500
#Sort the corners (Top-left, Top-right, Bottom-right, Bottom-left)
pts = np.array(coordinates, dtype="float32")
# the idea: sum and diff of (x,y) give you unique signatures, allowing to determine order
s = pts.sum(axis=1)
diff = np.diff(pts, axis=1)
ordered = np.zeros((4, 2), dtype="float32")
ordered[0] = pts[np.argmin(s)]  # top-left  has smallest  x+y
ordered[2] = pts[np.argmax(s)]  # bot-right has largest   x+y
ordered[1] = pts[np.argmin(diff)]  # top-right has smallest  x−y
ordered[3] = pts[np.argmax(diff)]  # bot-left has largest   x−y
#apply ideal positions to each coordinate (fit image to 500x500)
ideal = np.float32([[0, 0],
                    [width, 0],
                    [width, height],
                    [0, height]])
M = cv2.getPerspectiveTransform(ordered, ideal)
#Warp image to fit 500x500 size
warped = cv2.warpPerspective(image, M, (width, height))

#now we determine the grid within the image.
#calc size of grid cell
cell_size = width // 10
#create 2D grid array with actual coordinates of the grid (used later to map where the boats are)
grid = [[(col * cell_size, row * cell_size) for col in range(10)] for row in range(10)]


#Boat Detection below:
#convert warped image to HSV for black boat detection
warped_hsv = cv2.cvtColor(warped, cv2.COLOR_BGR2HSV)

#define the range for black boats (this may need to be changed depending on testing and we may change boat colour)
lower_black = np.array([0, 0, 0])
upper_black = np.array([180, 255, 130])

#Create mask for black color
black_mask = cv2.inRange(warped_hsv, lower_black, upper_black)
#Find contours of black boats
boat_contours, _ = cv2.findContours(black_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

#Perform connected components analysis to handle boats touching each other
num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(black_mask, connectivity=8)

#find boats in grid cells:
threshold = 0.6 #defines min ammount of cell that is black for it to register as a boat.
cell_area = cell_size * cell_size

for label in range(1, num_labels):  # Skip label 0 (background)
    component_mask = (labels == label).astype("uint8") * 255
    occupied_cells = set()
    #loop through each cell and determine if boat is present
    for row in range(10):
        for col in range(10):
            cell_x, cell_y = grid[row][col]
            cell_roi = component_mask[cell_y:cell_y + cell_size, cell_x:cell_x + cell_size]
            overlap_area = cv2.countNonZero(cell_roi)
            if overlap_area > threshold * cell_area:
                occupied_cells.add((row, col))

    #now the cell positions of boats are known, we need to determine the size and orientaation of the boat
    if occupied_cells:
        boat_size = len(occupied_cells)
        rows = [cell[0] for cell in occupied_cells]
        cols = [cell[1] for cell in occupied_cells]

        boat_width = max(cols) - min(cols) + 1
        boat_height = max(rows) - min(rows) + 1

        if boat_width > boat_height:
            orientation = "Horizontal"
        elif boat_height > boat_width:
            orientation = "Vertical"
        else:
            orientation = "Single Cell"

        #send boat positions
        for cell in occupied_cells:
            #Optional visualization:
            top_left = (grid[cell[0]][cell[1]][0], grid[cell[0]][cell[1]][1])
            bottom_right = (top_left[0] + cell_size, top_left[1] + cell_size)
            cv2.rectangle(warped, top_left, bottom_right, (255, 0, 0), 2)
            #populate array for sending to json
            boats_data.append({
                "occupied_cells": sorted(list(occupied_cells)),  # sort for readability
                "size": boat_size,
                "orientation": orientation
            })
#send
output = {"boats": boats_data}
print(json.dumps(output, indent=4))