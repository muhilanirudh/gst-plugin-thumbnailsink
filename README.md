# GStreamer JPEG Thumbnail Sink Plugin

A GStreamer element that takes raw video buffers and writes out JPEG thumbnail files on disk.

## Overview

**`thumbnailsink`** is a simple GStreamer sink element that:

- Accepts raw video frames (`video/x-raw`)
- Encodes each frame to a JPEG image
- Writes each JPEG to a file using a user‑specified filename pattern

This plugin is ideal for pipelines where you want to capture periodic snapshots (thumbnails) from a live or recorded video stream.

## Features

- **Automatic JPEG encoding** of raw RGB/BGR/YUV frames
- **Customizable filename pattern** (with timestamp or sequential numbering)
- **Configurable snapshot interval** (e.g. every N frames or milliseconds)
- **Thread‑safe file writes**  
- **LGPL‑2.1‑or‑later** licensed

## Element Properties

| Property              | Type    | Default            | Description                                      |
|-----------------------|---------|--------------------|--------------------------------------------------|
| `location`            | string  | `"thumb-%05d.jpg"` | `printf`‑style pattern for output filenames      |
| `snapshot-interval`   | guint   | `1`                | Take a thumbnail every N buffers                 |
| `start-on-first-buffer` | boolean | `true`           | Begin writing on the very first buffer           |

Examples:

- `location="frame-%04d.jpg"` → `frame-0001.jpg`, `frame-0002.jpg`, …
- `snapshot-interval=30` → write every 30th frame

## Requirements

- **GStreamer** ≥ 1.24  
- **Meson** ≥ 0.60.0 and **Ninja** for building  
- libjpeg (usually provided by `libjpeg-dev` or `libjpeg-turbo-devel`)

## Building & Installing

```bash
# Create build directory
meson setup build

# Compile
ninja -C build

# Install system‑wide (may need sudo)
sudo ninja -C build install

# Or set GST_PLUGIN_PATH to local build directory:
export GST_PLUGIN_PATH="$PWD/build"
