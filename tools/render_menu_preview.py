"""Compose an overview from the PPM frames exported by device_menu_test.cpp.

Requires Pillow. Firmware pixels are displayed at native size without alteration.
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input-dir', type=Path, default=ROOT/'build/menu-preview')
    parser.add_argument('--output', type=Path, default=ROOT/'build/menu-preview/menu-overview.png')
    args = parser.parse_args()
    background, card, border, orange, white, muted = '#111518', '#1a2024', '#354047', '#f5ab78', '#eceded', '#9eacb5'
    image = Image.new('RGB', (1400,1180), background)
    draw = ImageDraw.Draw(image)
    def font(size):
        return ImageFont.truetype(str(ROOT/'assets/fonts/Roboto.ttf'), size)
    draw.text((30,22), 'ZenTimer · Menüstruktur', font=font(36), fill=white)
    draw.text((30,72), 'Originales C++-Rendering · 280 × 240 Pixel pro Bildschirm · fiktive Sitzungen', font=font(19), fill=muted)
    draw.rounded_rectangle((24,120,355,1120), radius=14, fill=card, outline=border)
    draw.text((44,140), 'Navigation', font=font(24), fill=orange)
    nodes = [
        (0,'Bereit',True),(1,'Mitte 1 Sekunde halten',False),(0,'Hauptmenü',True),
        (1,'Profile',True),(2,'5 / 10 / 20 / 30 Minuten',False),(2,'45 / 60 Minuten',False),
        (1,'Anzeige',True),(2,'Restzeit an / aus',False),(2,'Steinbild an / aus',False),(2,'Helligkeit 40 / 70 / 100 %',False),
        (1,'Gespeicherte Sitzungen',True),(2,'4 Einträge pro Seite',False),(2,'Eintrag öffnen: Details',False),
        (1,'Daten / System',True),(2,'Sitzungen lesen',False),(2,'USB-Export',False),(2,'Speicher / Uhrzeit',False),
    ]
    for i,(level,text,emphasis) in enumerate(nodes):
        y=192+i*34; x=44+level*16
        if level:
            draw.line((x-10,y+11,x-4,y+11), fill=border, width=2)
        draw.text((x,y),text,font=font(18),fill=white if emphasis else muted)
    note = ['Bedienung unten:', '< / > blättern', 'Mitte: zurück oder schließen', '', 'USB-Export bleibt im Menü.', 'Die Datei entsteht auf dem Mac.', '', 'Lokaler Test ohne Hardware.', 'Keine Firmwareänderung,', 'kein Flashen.']
    for i,line in enumerate(note):
        draw.text((44,818+i*25),line,font=font(16),fill=muted)
    cards = [
        ('01-main','1 · Hauptmenü','Einstieg nach langem Druck'),
        ('02-profiles-1','2 · Profile, Seite 1','Auswahl übernimmt die Dauer'),
        ('03-profiles-2','3 · Profile, Seite 2','Weitere Vorlagen: 45 und 60 Minuten'),
        ('04-display','4 · Anzeige','Optionen werden dauerhaft gespeichert'),
        ('05-history-1','5 · Sitzungshistorie','Neueste Sitzung zuerst'),
        ('06-detail','6 · Sitzungsdetails','Aktive Dauer, Ergebnis, Startdatum'),
        ('07-data','7 · Daten / System','Historie, USB-Export, Speicherstatus'),
        ('08-storage','8 · Speicher / Uhrzeit','Hier: Uhrzeit noch nicht gesetzt'),
        ('09-history-2','9 · Historie, weitere Seite','Pfeile blättern, Mitte geht zurück'),
    ]
    for i,(name,title,caption) in enumerate(cards):
        x=378+(i%3)*336; y=120+(i//3)*338
        draw.rounded_rectangle((x,y,x+318,y+318),radius=12,fill=card,outline=border)
        draw.text((x+16,y+12),title,font=font(18),fill=orange)
        frame=Image.open(args.input_dir/(name+'.ppm')).convert('RGB')
        if frame.size!=(280,240):
            raise ValueError('Erwartet: 280x240 Firmwarebild')
        image.paste(frame,(x+19,y+42))
        draw.text((x+14,y+290),caption,font=font(14),fill=muted)
    draw.text((30,1142),'Die Screenshots zeigen den aktuellen Implementierungsstand, keine Hardwareaufnahme.',font=font(17),fill=muted)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    image.save(args.output)
    print(args.output)

if __name__=='__main__':
    main()
