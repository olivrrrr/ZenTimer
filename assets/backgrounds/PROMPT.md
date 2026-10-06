# Steinmotiv für ZenTimer

Erzeugt mit dem integrierten Bildgenerierungstool am 07.10.2026.
Das Masterbild bleibt unverändert. `tools/generate_stone_background.py`
passt es auf 280 × 240 Pixel an und erzeugt die RGB565-Daten im Flash.
Die Anzeige reduziert die Bildhelligkeit auf 60 % für die Lesbarkeit.

- Master: `stones-source.png`
- Displayformat: `stones-lcd.png`
- Firmwaredaten: `hardware/StoneBackground.h` im Projektstamm

## Verwendeter Bildprompt

Use case: photorealistic-natural. Asset type: static background image for a small 280 by 240 meditation timer display. Primary request: a quiet, tasteful photograph of smooth dark river stones. Scene: closely arranged natural charcoal and warm grey rounded pebbles, matte surfaces with gentle realistic texture, soft diffused low-key light, no glossy highlights. Composition: landscape with nearly square 7:6 aspect ratio, cropped close, soft-focus stones in the middle with restrained detail so white timer digits remain legible; slightly more stone texture toward the edges. Mood: calm, subtle, very dark, soft low contrast, natural and minimalist. Constraints: background only, no text, no numbers, no progress circle, no icons, no device, no wood housing, no watermarks, no bright or white stones. Opaque image, no transparent background.
