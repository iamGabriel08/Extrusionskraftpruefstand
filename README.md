# Extrusionskraftprüfstand (3D-Druck) – Firmware + GUI

Ein Prüfstand zur Untersuchung von **Extrusionsverhalten** im FDM-3D-Druck:  
Wir erfassen **Temperatur**, **Vorschub**, **Extrusionskraft** (über Wägezelle) und **Filament-Schlupf** (über Encoder) und visualisieren die Messung live in einer **Python-GUI**.

> Status: Prototyp / Laboraufbau  
> Ziel: reproduzierbare Messreihen für Material- und Parametervergleiche

---

## Inhalte / Features

- **Live-Messung** & Plotting (GUI)
- Parametrierung (Solltemperatur, Vorschub / Feedrate, Filamentlänge)
- Logging/Export der Messdaten (CSV)
- Modulare Firmware-Architektur (Hotend / LoadCell / Stepper / Encoder / Kommunikation)

---

## Repository-Struktur

Dieses Repo ist als PlatformIO-Projekt + Python-GUI organisiert:

- `src/` – Firmware (Entry/Tasks/Main)
- `include/` – Header / Schnittstellen (Klassen für HotEnd, LoadCell, ExtruderStepper, Encoder, GUI-Kommunikation)
- `lib/` – Externe/zusätzliche Libraries 
- `Python/` – PC-Software (GUI, Plotting, Logging, Tools/Notebooks)
- `Manual/` – Doku/Anleitungen/Build-Infos
- `platformio.ini` – PlatformIO Build-Konfiguration

---

## Hardware 

- Mikrocontroller: **ESP32-S3 DevKitC-1** 
- Extruder-Antrieb: Stepper + Treiber (**TMC2209**)
- Hotend/Heater: Heizpatrone + Lüfter + Temperaturmessung (**104NT-Thermistor**)
- Kraftmessung: Wägezelle + **HX711**
- Filamentweg/Schlupf: Rotary-Encoder am Filamentpfad
- Leistungselektronik: MOSFET-Treiber + MOSFET (für Heizer und Lüfter)
- Netzteil passend zu Heater/Stepper (Leistung beachten--> Prüfstand hat ca.1665W)

---

## Software-Voraussetzungen

### Firmware (PlatformIO)
- VS Code + **PlatformIO** Extension

### PC-GUI (Python)
- Python **3.10+** empfohlen
- Pakete: `pyserial`, `matplotlib`, `numpy` 

---

## Quickstart

### 1) Firmware flashen (PlatformIO)

### 2) ESP32 per USB Kabel verbinden

### 3) .exe des GUI ausführen

1. Repo klonen:
   ```bash
   git clone https://github.com/iamGabriel08/Extrusionskraftpruefstand.git
   cd Extrusionskraftpruefstand
