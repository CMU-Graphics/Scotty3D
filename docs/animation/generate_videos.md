---
layout: default
title: "Generating Videos"
permalink: /generate_videos/
---

# Generating Videos

## Converting frames to video with ffmpeg

Once you've rendered all the frames of your video, you can combine them into a video by using:

`ffmpeg -r 60 -f image2 -s 800x600 -i ./Video_[TIME]_%4d.png -vcodec libx264 out.mp4`

Where `[TIME]` should be replaced by the timestamp in each frame's filename (they should all be identical). If you don't have ffmpeg installed on your system, you can get it through most package managers, or you can [download it directly](https://ffmpeg.org/download.html), or use your own video creation program.
