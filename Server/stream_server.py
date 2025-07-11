from flask import Flask, Response
import subprocess
import struct
import ctypes
import opuslib

app = Flask(__name__)

SAMPLE_RATE = 24000
CHANNELS = 2
FRAME_DURATION_MS = 20
FRAME_SIZE = int(SAMPLE_RATE * FRAME_DURATION_MS / 1000)
BYTES_PER_SAMPLE = 2
FRAME_BYTES = FRAME_SIZE * CHANNELS * BYTES_PER_SAMPLE


@app.route("/")
def index():
    return "ESP32-C3 Streaming Server"


@app.route("/stream.opus")
def stream_opus():
    def generate():
        input_mp3 = "./sample.mp3"  # ←←← Can be changed to another file path

        # Step 1: Start ffmpeg to decode MP3 to PCM stream in real time
        ffmpeg = subprocess.Popen(
            [
                "ffmpeg",
                "-i",
                input_mp3,
                "-f",
                "s16le",
                "-acodec",
                "pcm_s16le",
                "-ac",
                "2",
                "-ar",
                str(SAMPLE_RATE),
                "-loglevel",
                "quiet",
                "-",
            ],
            stdout=subprocess.PIPE,
            bufsize=0,
        )

        # Step 2: Initializing Opus encoder
        encoder = opuslib.Encoder(SAMPLE_RATE, CHANNELS, opuslib.APPLICATION_AUDIO)

        while True:
            raw = ffmpeg.stdout.read(FRAME_BYTES)
            if len(raw) < FRAME_BYTES:
                break
            frame = struct.unpack("<" + "h" * (len(raw) // 2), raw)
            c_array = (ctypes.c_int16 * len(frame))(*frame)
            encoded = encoder.encode(c_array, FRAME_SIZE)
            yield struct.pack(">H", len(encoded)) + encoded

        ffmpeg.stdout.close()
        ffmpeg.wait()

    return Response(generate(), mimetype="application/octet-stream")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
