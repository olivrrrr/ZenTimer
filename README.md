# ZenTimer

![ZenTimer-Designentwurf: Holzgehäuse mit Display, Fortschrittskreis und Restzeit](docs/images/zentimer-design.png)

*Designentwurf für Gehäuse und Oberfläche.*

Kleine modulare Meditationstimer-Firmware für den Seeed XIAO nRF52840.
Branch: `feature/timer-core`. LCD und Touch wurden auf der Hardware getestet.
Die aktuelle Version mit Kreis, Wischgesten und Abschlussblinken wurde am
06.10.2026 erfolgreich auf das XIAO geflasht. Die detaillierten Abnahmeprüfungen
stehen im [Hardware-Dokument](docs/hardware-demo.md#lokale-prüfung-und-abnahme-am-gerät).

Die [bebilderte Anschlussanleitung für das Waveshare-Touchdisplay](docs/wiring.md#bebilderte-anschlussanleitung)
zeigt Steckerorientierung, Signalzuordnung und die stromlose Prüfung.

Der aktuelle [20-Minuten-Präsentations-Build](docs/hardware-demo.md) verwendet
Hardware-SPI mit zunächst 8 MHz und Software-SPI-Rückfalloption, Touch und einen orangefarbenen Kreis
ab 6 Uhr. Im Zustand Ready lässt sich
die Dauer über dezente graue Minus-/Plus-Symbole zwischen 1 und 60 Minuten
einstellen. Große seitliche Touchflächen reagieren schon beim Aufsetzen.
Ein Doppeltipp bricht eine laufende oder pausierte Sitzung ab.
Roboto-Ziffern und ein breiterer Fortschrittsbogen über einem verblasst
orangefarbenen Ring bilden die Anzeige. Dort stehen Build-Befehle, Farbtest,
Software-SPI-Fallback und die noch nötigen Hardwareprüfungen.

## Aufbau und Verhalten

- `TimerCore.h`: reine C++-Timerlogik ohne Arduino- oder I/O-Abhängigkeiten.
- `SerialConsole.h/.cpp`: USB-Serial-Befehle und Statusausgabe.
- `TimeSource.h`: austauschbare monotone Millisekundenquelle.
- `ZenTimer.ino`: verbindet Timer, Serial, Anzeige und Touch mit einer
  `millis()`-Zeitquelle. Keine Wartezeit auf USB und keine blockierende
  Eingabelese-Schleife; feste Resetwartezeiten gibt es nur beim Hardwarestart.

Zustände: **bereit**, **läuft**, **pausiert**, **beendet**. Standarddauer auf dem
Gerät: **20 Minuten** (1200 Sekunden), eingestellt in `ZenTimer.ino`. Der unveränderte Timerkern und
Mac-Simulator starten weiterhin mit 10 Minuten. Die Dauer wird im RAM gehalten
und nach einem Neustart zurückgesetzt.
Pausen zählen nicht zur Meditationsdauer. Der Timer läuft auch bei getrenntem
Monitor weiter. `cancel` setzt ihn auf bereit mit der eingestellten vollen Dauer.
Nach beendet beginnt `start` eine neue Sitzung mit derselben Dauer.

Unsigned Differenzen von `millis()` behandeln den Überlauf nach etwa 49,7 Tagen.
Die Hauptschleife muss mindestens einmal pro Überlaufperiode aufgerufen werden.
Restsekunden werden aufgerundet, damit 0 erst bei tatsächlichem Ende erscheint.

## Serielle Bedienung

115200 Baud; Befehle in Kleinbuchstaben mit Enter abschließen. LF, CR und CRLF
werden akzeptiert. Maximal 63 Zeichen pro Zeile; längere Zeilen werden verworfen.

| Befehl | Wirkung |
| --- | --- |
| `duration 600` | Dauer in ganzen Sekunden setzen (1–86400); nur bereit/beendet, setzt auf bereit |
| `start` | Aus bereit/beendet starten |
| `pause` | Laufenden Timer pausieren |
| `resume` | Pausierten Timer fortsetzen |
| `cancel` | Aus jedem Zustand abbrechen/zurücksetzen |
| `status` | Zustand, eingestellte Dauer und Restsekunden ausgeben |
| `help` | Befehle anzeigen |

Beispiel: `duration 60`, `start`, `pause`, `status`, `resume`, `cancel`
(jeweils eine eigene Zeile). Ungültige Befehle, Werte oder Übergänge erzeugen
eine Fehlermeldung. Ist die Dauer beim Pausieren bereits abgelaufen, bleibt
der Zustand beendet.

Beim Verbinden erscheinen Hilfe und Status. Weitere Ausgaben erfolgen bei
Befehlen/Zustandswechseln und während des Laufs alle fünf Sekunden. Bereit,
pausiert und beendet erzeugen keine regelmäßigen Ausgaben.

## Build und Monitor (macOS)

Verwendete Umgebung: Arduino CLI 1.5.1, Seeeduino:nrf52 Core 1.1.13,
Python 3.13.7. `~/.local/bin/python` verweist auf Python; dieser Ordner muss
für den Build im PATH sein. Sketchordner und Hauptdatei heißen `ZenTimer` bzw.
`ZenTimer.ino`.

```sh
PATH="$HOME/.local/bin:$PATH" arduino-cli compile \
  --fqbn Seeeduino:nrf52:xiaonRF52840Sense --build-path build/arduino .
arduino-cli board list
arduino-cli monitor --port /dev/cu.usbmodem2401 --config baudrate=115200
```

Den aktuellen Port vor Monitorstart und jedem späteren Upload prüfen;
`/dev/cu.usbmodem2401` ist nur der zuletzt bekannte Port. Build und Monitor
laden keine Firmware hoch. Der Monitor benötigt die Timer-Firmware auf dem Board; sie ist inzwischen
auf dem angeschlossenen XIAO installiert.

In VS Code den Projektordner `ZenTimer` öffnen. **Run Build Task** startet
`ZenTimer: Build` mit dem nötigen PATH. Unter **Run Task** stehen zusätzlich
`ZenTimer: USB Ports` und `ZenTimer: Serial Monitor` mit Portabfrage bereit.
Den Monitor mit Ctrl+C beenden. Es gibt absichtlich keinen Upload-Task.

Der hardwareunabhängige Test prüft Zustandsübergänge, Pause, Abbruch,
Neustart, Dauergrenzen, Ablauf beim Pausieren und den Zeitüberlauf:

```sh
c++ -std=c++11 -Wall -Wextra -pedantic tests/timer_core_test.cpp \
  -o /tmp/zentimer-core-test
/tmp/zentimer-core-test
```

## Grafischer Simulator (macOS, ohne Hardware)

Der native Simulator verwendet **AppKit/Cocoa** mit Objective-C++ und bindet
dieselbe Datei `TimerCore.h` wie die Firmware ein. Es gibt keine zweite
Timerimplementierung und keine JavaScript-Abhängigkeit. AppKit zeichnet den
Kreisbogen mit [NSBezierPath](https://developer.apple.com/documentation/appkit/nsbezierpath/appendarc%28withcenter%3Aradius%3Astartangle%3Aendangle%3Aclockwise%3A%29).
Dies ist eine funktionale UI-Simulation, keine nRF52840-Emulation.

Benötigt werden macOS und Apples **Xcode Command Line Tools** (Clang/C++ und
macOS-SDK mit Cocoa). Diese sind auf diesem Entwicklungsrechner vorhanden.
Bei einer neuen Einrichtung mit `xcode-select --install` installieren.
Keine Homebrew-Grafikpakete, Arduino-Hardware oder Python sind für den
Simulator erforderlich.

Im Projektordner starten:

```sh
sh simulator/run.sh
```

Alternativ in VS Code **Run Task → ZenTimer: Simulator**. Der Task kompiliert
und öffnet das Fenster. `ZenTimer: Simulator Build` kompiliert nur.
Beenden über das Fenster oder Cmd+Q. Das Programm liegt unter
`build/simulator/ZenTimerSimulator`; Arduino nutzt separat `build/arduino`,
damit dessen Build-Bereinigung den Simulator nicht entfernt.

Bedienung mit der Maus:

- Dauer in ganzen Sekunden eingeben, dann **Übernehmen** oder **Start**.
  Start übernimmt auch eine noch unbestätigte Eingabe. Werte: 1–86400 Sekunden.
- **Start**, **Pause**, **Fortsetzen**, **Abbrechen** entsprechen den
  Zustandsübergängen des gemeinsamen Kerns. Während Lauf/Pause ist die
  Dauer gesperrt; zum Ändern erst abbrechen.
- **Restzeit anzeigen** blendet die Zeit in der Kreismitte ein oder aus.
- **Zeitablauf** wählt 1×, 10× oder 60×. Ein Wechsel erzeugt keinen Zeitsprung;
  Pausen zählen auch im beschleunigten Modus nicht.

Der Fortschrittsbogen beginnt leer bei 12 Uhr und wächst im Uhrzeigersinn
bis zum geschlossenen Kreis am Ende. Er verwendet die millisekundengenaue
Restzeit, die Textanzeige aufgerundete Sekunden im Format Minuten:Sekunden.
Die Oberfläche aktualisiert mit etwa 30 Hz. Das Fenster hat einen neutralen
Hintergrund; `TimerView::drawRect` ist die Stelle für ein späteres statisches
Steinmotiv. Das Fenster ist zunächst bewusst nicht größenveränderbar.

`TimerCore` erhält eine `TimeSource&`, deren Lebensdauer den Timer umfasst.
Die Firmware liefert `millis()`, der Simulator verwendet
`std::chrono::steady_clock` und skaliert nur die Zeitquelle. Die Zeit wird als
Millisekunden modulo 2³² geliefert; Ablauf und Pausen berechnet weiterhin
ausschließlich der Kern. Schlaf-/Ruhezustand des Macs ist kein simulierter
Gerätestandby und wird nicht gesondert modelliert.

Zusätzlicher deterministischer Test für Beschleunigung, Geschwindigkeitswechsel,
Überlauf und Pausen mit dem gemeinsamen Kern:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic tests/simulator_clock_test.cpp \
  -o /tmp/zentimer-clock-test
/tmp/zentimer-clock-test
```

Automatisierter GUI-Smoke-Test (benötigt eine macOS-Grafiksitzung):

```sh
sh simulator/build.sh
./build/simulator/ZenTimerSimulator --self-test
```

Er prüft die Bedienelemente und rendert eine Halbzeit-Vorschau des Kreisbogens nach
`build/simulator/preview.png`. Danach beendet er sich wieder.

## Spätere Erweiterungen

Waveshare 1,69-Zoll-Touch-LCD und CST816S-Touch sind angeschlossen und laut
Hardware-Handoff einzeln getestet. Der neue Präsentations-Build verbindet sie
mit dem Timerkern und ist geflasht; die vollständige Hardwareabnahme ist noch
nicht dokumentiert. Proximity-Sensor
und LiPo sind vorhanden; genaue Varianten sind noch offen. Audio, Sensorsteuerung,
Sleep und Synchronisierung sind noch nicht implementiert.

Zieloberfläche: langsam schließender Fortschrittskreis, zuschaltbare Restzeit
und dezentes statisches Steinmotiv. Geplant sind Timefully-Profilimport,
Zeitanpassung am Gerät und Rückübertragung von Sitzungen. Automatische
Synchronisierung ist ein Ziel; eine verfügbare Schnittstelle ist bislang nicht
nachgewiesen. Es wird keine API angenommen.
