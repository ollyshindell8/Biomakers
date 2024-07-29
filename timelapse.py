###IMPORTANT NOTES PLS READ THIS###
#Two packages are required to be installed before running this code: "opencv-python"
#Photos with timestamps will be saved to the same folder that code is located in
#Code should automatically detect attached webcam
#Code will keep running until it is manually stopped, press Ctrl + C to stop the code.
#Documentation on how to use this code is included in the Operational Manual, for more detail please review that document.
#Try not to change anything outside of Initialization.
###END NOTES###

import cv2
import time
from datetime import datetime
import os

###VARIABLE INITIALIZATION###
time_int = 'second' #Change this value between 'second', 'min', 'hour' to get desire measurement interval.
CaptureInt = 2 #Change this value to get one photo every second or min or hour.
res = [1920,1080] #set the resolution of the picture (pixel count)
overwrite = False #set 'True' or 'False' if the program will start overwriting all the pictures inside the folder
webcam = 0 #Changes which webcam this laptop uses, usually 0 is the laptops own camera and 1 is the external webcam.
###END INITIALIZATION###

#user define functions
#reads the number of existing images in the folder and enable continue counting
def count_files():
    files_in_dir = 0
    for filename in os.listdir():
        if filename.endswith('.png'):
            files_in_dir += 1
    return files_in_dir

#converting the time interval from anything else to seconds
if time_int == 'min':
    CaptureInt = CaptureInt * 60
elif time_int == 'hour':
    CaptureInt = CaptureInt * 3600

#overwrite existing images or not
if overwrite == True:
    ImageCount = 0
else:
    ImageCount = count_files ()

#turn on webcam
cam = cv2.VideoCapture (webcam)
time.sleep (1)

#setting the resolution
cam.set(cv2.CAP_PROP_FRAME_WIDTH,res[0])
cam.set(cv2.CAP_PROP_FRAME_HEIGHT,res[1])

#infinite while loop
i = 1
while i == 1:

    result, image = cam.read ()

    if result:
        image = cv2.putText (image, str (datetime.now()), (0,30), cv2.FONT_HERSHEY_COMPLEX, 1, (255,255,255), 2)
        cv2.imwrite (str (ImageCount) + '.png', image)
        print ('success') 

    else:
        print ('fail')
    ImageCount += 1
    time.sleep (CaptureInt)