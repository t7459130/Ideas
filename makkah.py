import whisper
import numpy as np
import time

MODEL = "base"
AUDIO_FILE = "/tmp/kabah.raw"
SRT_FILE = "/tmp/live.srt"

model = whisper.load_model(MODEL)

chunk_seconds = 5
sample_rate = 16000
chunk_size = chuck_seconds * sample_rate

def write_srt(idx, start, end, text):
	with open(SRT_FILE, "a", encoding="utf-8") as f:
		f.write(f"{idx}\n")
		f.write(f"{start:.3f} --> {end:.3f}\n")
		f.write(text.strip() + "\n\n")

idx = 1
offset = 0.0

while True:
	with open(AUDIO_FILE, "rb") as f:
		f.seek(int(offset * sample_rate * 2))
		raw = f.read(chunk_size * 2)
	
	if len(raw) < chunk_size * 2:
		time.sleep(1)
		continue
	
	audio = np.frombuffer(raw, np.int16).astype(np.float32) / 32768.0

	result = model.transcribe(
		audio,
		task="translate",
		language="ar",
		fp16=False
	)

	text = result["text"]
	write_srt(idx, offset, offset + chunk_seconds, text)

	idx += 1
	offset += chunk_seconds






