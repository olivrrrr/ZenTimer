# Menüs, dauerhafte Ablage und USB-Export

Die Sitzung endet auf dem Gerät, ihre Aufzeichnung bleibt erhalten. ZenTimer
legt Einstellungen und Sitzungen im zusätzlichen Flash des XIAO ab. Eine
Verbindung zum Mac ist zum Meditieren nicht nötig. Sie dient zum Sichern der
Daten, zum Setzen der Uhrzeit und zum Erzeugen einer importierbaren Datei.

## Bedienung am Gerät

In **Bereitschaft** die Mitte etwa **eine Sekunde halten**, dann loslassen.
Ein kurzer Tap startet weiterhin den Timer; Minus und Plus behalten ihre
bisherigen großen seitlichen Touchbereiche. Während einer Sitzung bleiben
Tap zum Pausieren/Fortsetzen und Doppeltipp zum Abbrechen erhalten. Über USB
öffnet `menu` dasselbe Menü, solange der Timer bereit ist.

| Menü | Inhalt |
| --- | --- |
| Profile | 5, 10, 20, 30, 45 oder 60 Minuten; Auswahl übernimmt die Dauer und kehrt zum Timer zurück |
| Anzeige | Restzeit ein/aus, Steinbild ein/aus, Helligkeit 40/70/100 % |
| Gespeicherte Sitzungen | Vier Einträge je Seite, neueste zuerst; Tap öffnet Dauer, geplante Zeit, Ergebnis und Datum |
| Daten / System | Sitzungsliste, USB-Export und Speicher-/Uhrzeitstatus |

Die unteren Bereiche `<` und `>` blättern in Profilen und Historie. Unten in
der Mitte geht es zurück. Das Hauptmenü schließt über die untere Fläche.
Die Restzeit ist auch bei ausgeblendeten Ziffern intern vollständig verfügbar.
Die Helligkeit bleibt beim dreimaligen Abschlussblinken erhalten.

Profile sind zunächst lokale Dauer-Vorlagen. Start-/Schlussgong, Intervallgong,
Vorbereitungs- und Nachruhephasen sowie Timefully-Profilimport bleiben spätere
Erweiterungen. Audio ist nicht angeschlossen; Bluetooth, Headsets und
Brustgurte sind nicht Bestandteil dieses Builds. Eine angebliche
Timefully-Profil-API oder ein unbestätigtes Profil-Dateiformat wird nicht verwendet.

## Was gespeichert wird

- Eingestellte Dauer, Restzeitanzeige, Hintergrund und Helligkeit.
- Pro Sitzung eine stabile Geräte-/Sitzungskennung, geplante Zeit, tatsächlich
  gemessene aktive Dauer, Ergebnis und verfügbare Startzeit.
- Ergebnisse: abgeschlossen, abgebrochen oder Stromverlust.

Pausen zählen nicht zur aktiven Dauer. Einstellungen werden nach Änderungen
wiederhergestellt. Ohne gespeicherte Einstellung gilt die Startdauer von
`1200` Sekunden aus `ZenTimer.ino`.

Der externe P25Q16H hat 2 MiB und ist vom internen Programmflash getrennt.
Die Bild- und Fontassets bleiben im Programmflash. Das Journal verwendet
64-Byte-Einträge mit CRC32 und einer zuletzt geschriebenen Abschlussmarkierung.
Es wird angehängt, niemals automatisch gelöscht oder überschrieben. Start,
Ende und Pause/Fortsetzen erzeugen Einträge; während des Laufs entsteht
etwa jede Minute ein Zwischenstand. Beim Neustart wird eine offene Sitzung
als Stromverlust bis zum letzten gültigen Zwischenstand abgeschlossen. Die
verbleibenden Sekunden seit diesem Zwischenstand sind unbekannt. Es wird
keine Sitzung automatisch fortgesetzt. Pro neu gestarteter Aufzeichnung wird
ein freier Ende-Eintrag reserviert; Zwischenstände verbrauchen diese Reserve nicht.

Der erste Start prüft den gesamten externen Flash. Nur vollständig leerer
Flash oder ein erkanntes ZenTimer-Journal wird verwendet. Fremde Daten,
unbekannte Formate oder ein unvollständiger erster Journal-Header sperren die
Speicherung; es findet **keine Formatierung** statt. Ein volles Journal bleibt
lesbar, neue Einträge werden abgewiesen. Der Timer bleibt bedienbar, der
Speicherstatus zeigt den Fehler. Ein ausdrücklicher Hinweis erscheint beim
Start einer nicht speicherbaren Sitzung auch über USB. Eine spätere
Archivierung/Löschfunktion mit eigener Bestätigung ist noch nicht eingebaut.

2 MiB entsprechen 32.768 Einträgen. Die Zahl der Sitzungen hängt von Dauer,
Pausen, Einstellungsänderungen und Neustarts ab; bei 20 Minuten ohne Pause
liegt sie grob bei 1.400–1.500 Sitzungen. Ein Stromverlust kann den gerade
beschriebenen Eintrag verlieren. Die Prüfsummen-/Commit-Prüfung und das
Überspringen unvollständiger Einträge wurden mit simulierten Schreibabbrüchen
getestet; echte Stromverlustversuche am Gerät stehen noch aus.

Die gebündelte Flash-Bibliothek des Cores 1.1.13 kennt P25Q16H nicht in ihrer
Standard-Erkennungsliste. Deshalb übergibt `QspiJournalFlash` dessen
JEDEC-Parameter explizit, mit konservativem 32-MHz-Limit. Display-SPI bleibt
bei 8 MHz. Initialisierung und Flash-Programmierung benötigen kurze synchrone
Bibliotheksaufrufe; die Timerrechnung bleibt unabhängig davon zeitbasiert.

## Uhrzeit ohne RTC

Die monotone Zeitmessung über `millis()` funktioniert weiterhin überlaufsicher.
Das Kalenderdatum ist eine andere Information: Nach einem stromlosen Start
kennt ZenTimer das Datum nicht. Eine batteriebetriebene RTC wird nicht angenommen.

Die Mac-Bridge setzt `time <Unix-Sekunden>` bei jeder Verbindung. Ab dann haben
neue Sitzungen einen UTC-Startzeitpunkt. Bereits abgeschlossene Sitzungen
**desselben Gerätestarts** kann sie aus gespeichertem Start-Uptime und aktueller
Uptime nachträglich datieren. Die Genauigkeit liegt etwa bei einer Sekunde,
abhängig von USB-Laufzeit und Drift. Die Geräteanzeige zeigt bei ursprünglich
undatierten Einträgen weiterhin „Datum offen“; die nachträgliche Zuordnung
liegt in der Mac-Ablage.

Undatierte Sitzungen aus früheren Gerätestarts werden erhalten, aber nicht mit
erfundenem Datum in Open Sit exportiert. `pending.json` nennt sie mit ihrer UUID.
Ein bekanntes Datum lässt sich auf dem Mac ausdrücklich zuordnen:

```sh
.venv/bin/python tools/session_bridge.py \
  --resolve-date 'UUID-AUS-PENDING=2026-10-07T18:30:00+02:00'
```

## Mac einrichten und Daten lesen

Die Bridge nutzt Python 3 und SQLite aus der Standardbibliothek. Für USB wird
zusätzlich `pyserial` benötigt. Der serielle Monitor muss vor dem Auslesen
geschlossen werden; ein USB-Port kann nur von einem Programm gleichzeitig
benutzt werden.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install pyserial
arduino-cli board list
.venv/bin/python tools/session_bridge.py --port /dev/cu.usbmodem2401
```

Vorher laufende oder pausierte Sitzungen beenden. Alternativ in VS Code
**Run Task → ZenTimer: Sitzungen auslesen**. Der Task fragt den aktuellen Port
ab. Es erfolgt kein Firmwareupload.

Unter `data/sessions/` entstehen:

| Datei | Verwendung |
| --- | --- |
| `sessions.sqlite3` | Lokales Archiv; wiederholtes Auslesen führt anhand stabiler UUIDs nicht zu doppelten Sitzungen |
| `sessions.opensit.json` | Open Sit 1.0 für Timefully und andere kompatible Programme |
| `pending.json` | Einträge ohne Datum, unter einer Sekunde oder mit unvollständiger Checkpoint-Dauer |

Diese persönlichen Daten und die lokale Python-Umgebung sind von Git
ausgeschlossen. Das Auslesen löscht nichts auf dem Gerät. Ein abgebrochener
USB-Export wird verworfen, das bestehende Archiv bleibt erhalten. Die Dateien
lassen sich ohne angeschlossenes Gerät aus der lokalen Datenbank neu erzeugen:

```sh
.venv/bin/python tools/session_bridge.py
```

Stromverlust-Einträge werden standardmäßig nicht als vollständige Sitzungen
exportiert. Mit `--include-interrupted` lässt sich bewusst die bekannte Dauer
bis zum Checkpoint mitnehmen. Regulär abgebrochene Sitzungen mit mindestens
einer Sekunde aktiver Dauer werden exportiert; ihr Status bleibt in `extra`.

## Timefully und spätere Serverablage

Open Sit beschreibt eine **Datei**, keine Synchronisierungs-API. Die Bridge
schreibt Version 1.0 mit UUID, Startzeit einschließlich UTC-Zeitzone, aktiver
Dauer und Herkunft. Unbekannte Zeitzonen, Gesundheitswerte und Segmente werden
weggelassen. Die UUID wird aus Geräte-ID und stabiler Journal-Sitzungsnummer
gebildet und bleibt bei erneutem Export gleich.

`sessions.opensit.json` auf das Telefon übertragen und über die dokumentierte
Importfunktion von Timefully öffnen. Timefully ordnet importierte Sitzungen
dem aktiven Profil zu. Die konkrete Importfähigkeit dieser Bridge-Datei muss
noch in der installierten Timefully-Version geprüft werden; automatischer
Import und Profilübertragung werden nicht zugesagt.

Es gibt derzeit keinen Server. Vorbereitet ist ein ausdrücklich aufrufbarer
Upload an **einen eigenen Endpunkt**, der JSON per HTTP POST annimmt:

```sh
.venv/bin/python tools/session_bridge.py \
  --upload-url 'https://DEIN-SERVER.example/sessions'
```

Optional liest die Bridge einen Bearer-Token aus `ZENTIMER_UPLOAD_TOKEN`.
URL und Token werden nicht in Git abgelegt. HTTPS ist erforderlich; HTTP ist
nur für einen lokalen Test auf localhost erlaubt. Dieser URL-Pfad ist ein
Beispiel für einen eigenen Dienst, kein bekannter Timefully-Endpunkt.

Der spätere Server muss Authentifizierung, dauerhafte Ablage, Duplikaterkennung
anhand der UUIDs und einen Download der Open-Sit-Datei bereitstellen. Die
Bridge sendet die exportierbaren Sitzungen; offene Einträge bleiben im lokalen
Archiv. Ohne `--upload-url` wird nichts übertragen. Es läuft weder ein
Hintergrunddienst noch eine automatische Cloud-Synchronisierung.

## Serielle Zusatzbefehle

| Befehl | Wirkung |
| --- | --- |
| `menu` | Menü in Bereitschaft öffnen |
| `sessions` | Bis zu zehn zuletzt gespeicherte Sitzungen ausgeben |
| `storage` / `info` | Geräte-ID, Uptime, Uhrzeit und Speicherzustand als `ZT_INFO` ausgeben |
| `time 1791388800` | UTC-Kalenderzeit setzen, nur für diesen Gerätestart |
| `export` | Im Ruhezustand alle abgeschlossenen Journal-Sitzungen zwischen `ZT_BEGIN` und `ZT_END` ausgeben |

Der Rohdatenexport ist ein eigenes ZenTimer-USB-Protokoll, nicht Open Sit.
Die Umwandlung übernimmt die Mac-Bridge. Im Normalbetrieb gibt es weiterhin
keine regelmäßigen Sitzungsexporte.

## Quellen und Prüfstand

- [Seeed: XIAO nRF52840, Speicher und Ressourcen](https://wiki.seeedstudio.com/XIAO_BLE/)
- [P25Q16H-Datenblatt](https://files.seeedstudio.com/wiki/github_weiruanexample/Flash_P25Q16H-UXH-IR_Datasheet.pdf)
- [Adafruit: P25Q16H-Geräteparameter](https://github.com/adafruit/Adafruit_SPIFlash/blob/master/src/flash_devices.h)
- [Open Sit, offizielle Beschreibung](https://www.timefully.co/open-sit/)
- [Open-Sit-Spezifikation 1.0](https://www.timefully.co/open-sit/SPEC.md)
- [Offizielles JSON-Schema](https://www.timefully.co/open-sit/schema/1.0.json)
- [Timefully: Export und Import](https://www.timefully.co/support/export-import)

Neue Menüs und QSPI-Speicherung sind für den bisherigen Core kompiliert.
Hardwareabnahme: Einstellungen ändern, USB trennen, wieder verbinden und
Wiederherstellung prüfen; kurze Sitzung beenden und in der Historie lesen;
Abbruch/Pause prüfen; danach mit der Bridge auslesen und den Timefully-Import
prüfen. Erst anschließend gezielt einen Stromverlustversuch durchführen.
Timerkern und Mac-Simulator bleiben erhalten; der Simulator bildet die neuen
Geräte-Menüs und QSPI-Hardware derzeit nicht ab.


## Menüvorschau ohne Hardware erzeugen

Der lokale Menütest bedient `DeviceMenu` mit simulierten Touchpositionen und
verwendet `SessionStore` mit einem Flash-Ersatz im Arbeitsspeicher. Er prüft
Profilauswahl, gespeicherte Anzeigeoptionen, Historie mit Seiten und Details,
USB-Export sowie die Sperre während einer Sitzung. Die Screenshots entstehen
aus den vom tatsächlichen `TimerDisplay` übertragenen RGB565-Pixeln. Die
Sitzungen sind fiktive Testdaten; es wird kein Gerät angesprochen.

```sh
mkdir -p build/menu-preview
c++ -std=c++17 -Wall -Wextra -pedantic -I tests/stubs \
  tests/device_menu_test.cpp DeviceMenu.cpp SessionStore.cpp TimerDisplay.cpp \
  -o build/menu-preview/device-menu-test
./build/menu-preview/device-menu-test build/menu-preview
python3 tools/render_menu_preview.py
```

Für die Bildübersicht wird Pillow benötigt (`python3 -m pip install Pillow`
in einer passenden Python-Umgebung). Die Bilddatei liegt unter
`build/menu-preview/menu-overview.png`; die einzelnen Originalansichten
liegen daneben als PPM-Dateien. Die generierten Dateien bleiben lokal unter
`build/` und werden nicht in Git aufgenommen.
