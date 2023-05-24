import ffmpeg
import os

CANON_W = 3888
CANON_H = 2592

def inFolder(folder):
    return folder + "/in"

def outFolder(folder):
    return folder + "/out"

def vidFolder(folder):
    return folder + "/vid"

def renameFiles(folder):
    n = 0
    for filename in sorted(os.listdir(folder)):
        if filename[-4:] != ".jpg":
            continue
        os.rename(folder + "/" + filename, folder + "/" + f"{n:0>4}.jpg")
        n += 1

def generateTransforms(folder, preview):
    print("Generate transforms")
    scaleFactor = 4 if preview else 2
    outfile = "/transforms_preview.trf" if preview else "/transforms.trf"
    return (
        ffmpeg.input(inFolder(folder) + "/%04d.jpg", r=25)
              .filter("scale", CANON_W//scaleFactor, CANON_H//scaleFactor)
              .filter("vidstabdetect", shakiness=10, accuracy=10, result=(folder + outfile))
              .output("", f="null")
              .run_async()
    )

def combine(folder, outfile, frames, preview):
    print("Combine everything")

    scaleFactor = 4 if preview else 2
    tffile = "/transforms_preview.trf" if preview else "/transforms.trf"
    w = 960 if preview else 1920
    h = 540 if preview else 1080
    b = "2M" if preview else "10M"

    inInput = ffmpeg.input(inFolder(folder) + "/%04d.jpg", r=25)
    videoInput = ffmpeg.input(folder + "/recording.mjpeg", f="mjpeg", r=25)
    outInput = ffmpeg.input(outFolder(folder) + "/%04d.jpg", r=25)
    blurInput = ffmpeg.input(outFolder(folder) + "/0011.jpg", r=25)
    return (
        ffmpeg.concat(
                    inInput.filter("scale", CANON_W//scaleFactor, CANON_H//scaleFactor)
                           .filter("vidstabtransform", smoothing=5, input=(folder + tffile))
                           .filter("reverse")
                           .filter("crop", w, h, "(in_w-"+str(w)+")/2", "(in_h-"+str(h)+")/2"),
                    videoInput.trim(start_frame=40, end_frame=40+frames)
                              .filter("scale", w, h),
                    outInput.filter("scale", CANON_W//scaleFactor, CANON_H//scaleFactor)
                            .filter("vidstabtransform", smoothing=5, input=(folder + tffile))
                            .filter("crop", w, h, "(in_w-"+str(w)+")/2", "(in_h-"+str(h)+")/2"),
                    blurInput.filter("scale", CANON_W//scaleFactor, CANON_H//scaleFactor)
                             .filter("crop", w, h, "(in_w-"+str(w)+")/2", "(in_h-"+str(h)+")/2")
                             .filter("dblur", angle=0, radius=500)
              )
              .setpts("N/25/TB")
              .output(outfile, pix_fmt="yuv420p", preset="veryfast", b=b)
              .overwrite_output()
              .run_async()
    )

if __name__ == "__main__":
    import sys
    renameFiles(inFolder(sys.argv[1]))
    renameFiles(outFolder(sys.argv[1]))
    generateTransforms(sys.argv[1])
    combine(sys.argv[1], sys.argv[2])
