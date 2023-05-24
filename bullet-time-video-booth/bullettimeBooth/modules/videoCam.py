import ffmpeg
import os

def setupVideo():
    #os.system("v4l2-ctl -d /dev/video2 -c brightness=0,contrast=128,saturation=128,hue=0")
    os.system("v4l2-ctl -d /dev/video2 -c brightness=-11,contrast=148,saturation=128,hue=0")

def recordVideo(frames, out):
    print("Record video")
    return (
        ffmpeg.input("/dev/video2", f="v4l2", r=25, video_size=(1920, 1080), input_format="mjpeg", an=None)
              .output(out, vframes=frames+60, vcodec="copy", acodec="none")
              .run_async()
    )
