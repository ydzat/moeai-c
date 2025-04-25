# MoeAI-C

*In anderen Sprachen lesen: [简体中文](../../../README.md), [English](../en/README.md)*

## Projekteinführung

**MoeAI-C** ist ein auf dem Linux-Kernelmodul basierendes KI-Assistentenprojekt auf Systemebene. Es zielt darauf ab, ein intelligentes Modul mit Ressourcenmanagement, Systemüberwachung, Ereignisreaktion und KI-Entscheidungsfähigkeiten aufzubauen. In Zukunft wird die Integration mit LLM-Modellen im Benutzerbereich unterstützt, um dynamische Richtliniensteuerung und intelligente Systemreaktionen zu ermöglichen.

## Funktionen

- Überwachung der Systemressourcen (Speicher, Netzwerk, Dateisystem)
- Ereigniserkennung und intelligente Entscheidungsfindung
- Bidirektionale Kommunikation zwischen Benutzerraum und Kernelraum
- KI-Modellintegration (über Benutzeragentur)
- Unterstützung für Kommandozeilentools

## Build und Installation

### Abhängigkeiten

- Linux-Kernel-Header (`linux-headers-$(uname -r)`)
- GCC-Compiler und Entwicklungstools (`build-essential`)
- CMocka-Testbibliothek (optional, für Unit-Tests)

### Build-Schritte

```bash
# Notwendige Verzeichnisstruktur erstellen
make dirs

# Kernelmodul bauen
make all

# Nur Kommandozeilentool bauen
make cli

# Schnelles Modul-Test (laden und entladen)
make test

# Build-Dateien bereinigen
make clean
```

### Modul installieren

Methode 1: Temporäre Installation (nach Neustart verloren)
```bash
sudo insmod moeai.ko
```

Methode 2: Permanente Installation
```bash
# Modul im System installieren
sudo make install

# Modul laden
sudo modprobe moeai
```

### Modul deinstallieren

Methode 1: Temporäres Entfernen
```bash
sudo rmmod moeai
```

Methode 2: Permanente Deinstallation
```bash
sudo make uninstall
```

## Verwendung

### Modulstatus prüfen

```bash
# Prüfen, ob das Modul geladen ist
lsmod | grep moeai

# Modulprotokolle anzeigen
dmesg | grep moeai

# procfs-Schnittstelle prüfen
ls -la /proc/moeai/
cat /proc/moeai/status
```

### CLI-Tool

Das `moectl`-Kommandozeilentool bietet eine intuitive Möglichkeit, mit dem Kernelmodul zu interagieren:

```bash
# Systemstatus anzeigen
build/bin/moectl status

# Speicherüberwachungsschwelle einstellen
build/bin/moectl set threshold <Prozentwert>

# Speicherrückgewinnung auslösen
build/bin/moectl reclaim

# Hilfeinformationen anzeigen
build/bin/moectl --help
```

### Protokolle anzeigen

```bash
# Aktuelle Modulprotokolle anzeigen
cat /proc/moeai/log
```

## Projektstruktur

Bitte siehe [design.md](design.md) für eine detaillierte Projektarchitektur und -design.

## Entwicklungsplan

Das Projekt befindet sich derzeit in der MVP-Phase (0.1.0-MVP) mit implementiertem Kernfunktionalitätsrahmen und grundlegenden Überwachungsfähigkeiten. Ausführliche Aktualisierungshistorie finden Sie im [CHANGELOG](CHANGELOG).

## Zukunftsvision

Die Vision von MoeAI-C ist es, durch tiefe Integration von Kernel-Ebene und KI-Fähigkeiten eine Schlüsselkomponente für intelligenten Linux-Systembetrieb zu werden und dabei folgende Ziele zu erreichen:

### 1. Autonomes Ressourcenmanagement

- Dynamische Anpassung der Systemressourcenzuweisung basierend auf historischen Daten und Lastmustern
- Intelligente Vorhersage von Systemengpässen und Ergreifen von Präventivmaßnahmen
- Bereitstellung von Ressourcengarantiemechanismen für kritische Anwendungen und Dienste

### 2. Intelligenter Sicherheitsschutz

- Echtzeitüberwachung des Systemverhaltens und Identifizierung abnormaler Aktivitäten und potenzieller Bedrohungen
- Identifizierung verdächtiger Operationen basierend auf historischen Angriffsmustern
- Generierung und Optimierung adaptiver Firewall-Regeln

### 3. Leistungsoptimierung

- Identifizierung von System-Performance-Hotspots und Bereitstellung gezielter Optimierung
- Intelligentes I/O-Scheduling und Cache-Optimierung
- Anwendungsleistungsanalyse und Empfehlungen zur Beseitigung von Engpässen

### 4. Entwickler-Ökosystem

- Bereitstellung einer erweiterbaren modularen Architektur, die die Entwicklung spezialisierter Funktionsmodule durch Dritte ermöglicht
- API-Schnittstellenunterstützung für die Integration mit anderen intelligenten Systemen
- Entwickler-Toolchain und Visualisierungsanalyseplattform

### 5. Erklärbarkeit und Transparenz

- Alle automatisierten Entscheidungen liefern detaillierte Erklärungen und Begründungen
- Umfassendes Protokollierungssystem, das alle Systemeingriffe aufzeichnet
- Bereitstellung von Mechanismen zur Nachverfolgung und zum Zurücksetzen von Eingriffen

MoeAI-C wird diese Vision schrittweise verwirklichen und einen Kernel-Assistenten der nächsten Generation für Linux-Systeme entwickeln, der sowohl sicher und zuverlässig als auch intelligent und effizient ist.

## Lizenz

Dieses Projekt ist unter der [MIT/BSD]-Lizenz lizenziert - siehe die [LICENSE](LICENSE)-Datei für Details.