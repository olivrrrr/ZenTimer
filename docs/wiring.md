# Anschlussplan: Waveshare 1.69″ Touch LCD → XIAO nRF52840

Stand: 06.10.2026, Branch `feature/timer-core`. **Überprüfbarer Entwurf;
das konkrete Modul und Kabel sind noch nicht per Foto bestätigt.**
Nur Dokumentation: Timerkern, Firmware und Mac-Simulator bleiben erhalten.
Kein Upload und noch kein Display-Testprogramm.

## Modul und elektrische Eckdaten

Die Prospector-Stückliste nennt das separate Waveshare **1.69inch Touch LCD
Module, Artikel 27057**. Gemeint ist weder das achtpolige Modul ohne Touch
noch ein Modul mit integriertem ESP32/RP2040/RP2350.
[Quelle: Prospector-Stückliste](https://github.com/carrefinho/prospector#bill-of-materials).

Waveshare nennt **ST7789V2**, 240 × 280 sichtbare Pixel und 4-Draht-SPI.
Touch verwendet I²C mit der 7-Bit-Adresse **0x15**. Die Dokumentation ist beim
Touch-Chip widersprüchlich: Spezifikation/Ressourcen nennen **CST816S**, der
Abschnitt „Touch and Controller“ nennt **CST816D**. Deshalb gilt die genaue
Touch-Variante noch nicht als am vorhandenen Modul bestätigt.
[Quelle: Waveshare-Wiki](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module).

Das Modul erlaubt 3,3 V oder 5 V, verlangt aber gleiche Versorgung und
Logikpegel. Für den XIAO wird **VCC ausschließlich an 3V3**, GND an GND
angeschlossen; alle Signale arbeiten mit 3,3 V. Nicht an 5V/VBUS oder direkt
an LiPo-BAT anschließen. Der Modul-Schaltplan zeigt LDO, Pegelwandler und
einen Transistor für die Hintergrundbeleuchtung: `LCD_BL` steuert diesen,
nicht den LED-Strom direkt.
[Quellen: Waveshare-Wiki](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module),
[Waveshare-Schaltplan, Seite 1](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module.pdf).

## Stecker und Blickrichtung

Die Nummern unten sind die **elektrischen Kontakt-/Padnummern von P1**, dem
GH1.25-12P-Anschluss im Waveshare-Schaltplan. Nicht mit dem internen
Display-Flexanschluss H1 verwechseln.

**Ansicht zum Zuordnen:** auf die **Rückseite mit Bauteilen und Beschriftung**
schauen, Glas/Anzeigefläche zeigt von dir weg. Platine so drehen, dass der
12-polige Anschluss oben liegt und seine Signalbeschriftungen lesbar sind,
wie auf Seite 2 des offiziellen PDFs. Die Beschriftungsreihe läuft dann
**links nach rechts von VCC (P1.1) bis TP_IRQ (P1.12)**.
Die Bauteilansicht der
[Waveshare-Zeichnung](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module_2D_Drawing.pdf)
zeigt ebenfalls den Anschluss oben.
[Beleg für Signalfolge und Padnummern: Schaltplan/PCB-Ansicht, Seiten 1–2](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module.pdf).

Das ist **keine Nummerierung beim Blick in die Stecköffnung oder auf die
Kontaktseite des losen Kabelsteckers**. Solche Ansichten können gespiegelt
sein. Die konkrete Zuordnung der Kabeladern wird am eingesteckten Kabel
stromlos anhand von Beschriftung und Durchgang geprüft. Kabelfarben werden
nicht als Signalbeweis verwendet; keine Adern abschneiden.

## Vorgeschlagene Zuordnung

Die acht LCD-/Versorgungsleitungen folgen Prospector. Die vier Touch-Leitungen
sind eine **zusätzliche ZenTimer-Planung**, keine aus Prospector übernommene
Touch-Verdrahtung.
[Prospector-Anschlusszeichnung, Abschnitt A](https://github.com/carrefinho/prospector/blob/main/docs/prospector_assembly_manual.jpg).

| Display P1 / Signal | XIAO-Anschluss | nRF52840-Pin | Funktion |
| --- | --- | --- | --- |
| 1 · VCC | 3V3 | — | Modulversorgung 3,3 V |
| 2 · GND | GND | — | Gemeinsame Masse |
| 3 · LCD_DIN | D10 / MOSI | P1.15 | SPI-Daten XIAO → LCD |
| 4 · LCD_CLK | D8 / SCK | P1.13 | SPI-Takt |
| 5 · LCD_CS | D9 | P1.14 | LCD-Auswahl, aktiv LOW |
| 6 · LCD_DC | D7 | P1.12 | LOW: Kommando, HIGH: Daten |
| 7 · LCD_RST | D3 | P0.29 | LCD-Reset, aktiv LOW |
| 8 · LCD_BL | D6 | P1.11 | Backlight-Steuerung, HIGH: an; später PWM |
| 9 · TP_SDA | D4 / SDA | P0.04 | Touch-I²C-Daten, bidirektional |
| 10 · TP_SCL | D5 / SCL | P0.05 | Touch-I²C-Takt |
| 11 · TP_RST | D0 | P0.02 | Eigener Touch-Reset, aktiv LOW; neu geplant |
| 12 · TP_IRQ | D1 | P0.03 | Touch-Interrupt zum XIAO; neu geplant |

Signal-/Kontaktnummern stammen aus dem
[Waveshare-Schaltplan](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module.pdf).
XIAO-Pins wurden gegen die
[Seeed-Pinübersicht](https://wiki.seeedstudio.com/XIAO_BLE/#hardware-overview)
und die lokal installierten `variant.h`/`variant.cpp` von **Seeeduino:nrf52
1.1.13** geprüft. Die D0–D10-Zuordnung ist für XIAO und XIAO Sense gleich.
Für die spätere Arduino-Datei gelten D0=0 bis D10=10; die bereits bewährte
FQBN `Seeeduino:nrf52:xiaonRF52840Sense` bleibt vorerst bestehen.
[Core 1.1.13: variant.h](https://github.com/Seeed-Studio/Adafruit_nRF52_Arduino/blob/1.1.13/variants/Seeed_XIAO_nRF52840_Sense/variant.h),
[variant.cpp](https://github.com/Seeed-Studio/Adafruit_nRF52_Arduino/blob/1.1.13/variants/Seeed_XIAO_nRF52840_Sense/variant.cpp).

**SPI-Sonderfall für den späteren Test:** D9 ist der Standard-MISO-Pin des
Arduino-Cores, hier aber LCD_CS. Das LCD benötigt keine MISO-Leitung.
`SPI.begin()` richtet dennoch standardmäßig D9 als MISO ein; die spätere
Initialisierung muss D9 anschließend korrekt als CS-Ausgang behandeln oder
eine geprüfte SPI-Konfiguration ohne diesen Konflikt verwenden. Nicht blind
Prospectors Zephyr-Konfiguration übernehmen: Diese verlegt MISO auf P1.10.
Belegt durch die lokal gelesene `libraries/SPI/SPI.cpp` (Core 1.1.13) und das
[Prospector-Board-Overlay](https://github.com/carrefinho/prospector-zmk-module/blob/main/boards/shields/prospector_adapter/boards/seeeduino_xiao_ble.overlay).

## Touch, Proximity und Audio

Prospector verwendet zwar das Touch-fähige Modul, lässt aber TP_SDA,
TP_SCL, TP_RST und TP_IRQ in der Anschlusszeichnung unverbunden. Sein
Board-Overlay enthält ST7789, Backlight und APDS9960, keinen Touch-Knoten.
**Die dokumentierte Prospector-Verdrahtung unterstützt deshalb kein Touch.**
[Quellen: Anschlusszeichnung](https://github.com/carrefinho/prospector/blob/main/docs/prospector_assembly_manual.jpg),
[Board-Overlay](https://github.com/carrefinho/prospector-zmk-module/blob/main/boards/shields/prospector_adapter/boards/seeeduino_xiao_ble.overlay).

Für ZenTimer ergibt sich folgendes Pinbudget; dies sind Planungsvorschläge:

- Nach vollständiger LCD-/Touch-Anbindung ist von D0–D10 nur **D2 / P0.28**
  frei. D6/D7 sind belegt; USB Serial benötigt diese UART-Pins nicht.
- Ein späterer **I²C-Proximity-Sensor** kann D4/D5 mit Touch teilen, sofern
  Adresse, Versorgung und Pull-ups passen. Prospector nutzt APDS9960 an
  **0x39**, also ohne Adresskonflikt zu Touch an 0x15. Unsere Sensorvariante
  ist noch unbekannt. Der Waveshare-Schaltplan enthält bereits I²C-Pull-ups;
  zusätzliche Breakout-Pull-ups müssen berücksichtigt werden.
  [APDS9960-Adresse und elektrische Anschlüsse](https://learn.adafruit.com/adafruit-apds9960-breakout?view=all),
  [Touch-Adresse](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module).
- Prospector belegt **D2 mit APDS9960-INT**. Wenn ZenTimer diesen Interrupt
  ebenfalls braucht, bleibt kein frei zugänglicher D-Pin für Audio.
- Alternativ kann ein geeigneter Proximity-Sensor per I²C abgefragt werden,
  sodass **D2 für einfache Ton-/PWM-Steuerung** reserviert bleibt. Art und
  Treiberstufe hängen von der noch festzulegenden Audiohardware ab;
  Lautsprecher nicht direkt an einen GPIO anschließen.
- Ein übliches I²S-Audiomodul benötigt mehrere Signale. Dafür reicht der
  vorliegende Plan nicht ohne Umplanung oder zusätzliche Hardware. Es wird
  noch keine Audioverdrahtung zugesagt.

## Vor dem tatsächlichen Anschließen zu klären

Bitte ein scharfes **Foto der Modulrückseite mit vollständiger Beschriftung,
Revision und 12-poligem Anschluss** sowie **des Kabelsteckers von beiden
Seiten und seiner freien Enden** bereitstellen. Falls vorhanden, Artikelnummer
oder Produktlink der gekauften Variante ergänzen. Damit gleichen wir Platine,
Steckerrichtung und Aderzuordnung mit diesem Entwurf ab; der Widerspruch
CST816S/CST816D bleibt bis zur Variantenprüfung ausdrücklich offen.

Danach bei **abgezogenem USB und getrenntem Akku** verkabeln. Versorgung
und jede Signalader anhand der bestätigten Orientierung zuordnen; VCC/GND
und fehlenden Versorgungskurzschluss stromlos prüfen. Zum ersten Test
Proximity und Audio noch nicht anschließen.

Erst nach bestätigter Verdrahtung folgt ein **separater Display-Testsketch**
mit Farbflächen, Umrandung, Text und Touchposition. Dabei werden LCD-Offsets,
Orientierung und Touch-Koordinaten geprüft. Timer und Simulator bleiben als
eigene funktionierende Anwendungen erhalten.
