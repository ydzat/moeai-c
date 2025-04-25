# MoeAI-C

*In anderen Sprachen lesen: [简体中文](../../../README.md), [English](../en/README.md)*

**MoeAI-C** ist ein auf dem Linux-Kernelmodul basierendes KI-Assistentenprojekt auf Systemebene. Es zielt darauf ab, ein intelligentes Modul mit Ressourcenmanagement, Systemüberwachung, Ereignisreaktion und KI-Entscheidungsfähigkeiten aufzubauen. In Zukunft wird die Integration mit LLM-Modellen im Benutzerbereich unterstützt, um dynamische Richtliniensteuerung und intelligente Systemreaktionen zu ermöglichen.

## Funktionen

- Überwachung der Systemressourcen (Speicher, Netzwerk, Dateisystem)
- Ereigniserkennung und intelligente Entscheidungsfindung
- Bidirektionale Kommunikation zwischen Benutzerraum und Kernelraum
- KI-Modellintegration (über Benutzeragentur)
- Unterstützung für Kommandozeilentools

## Entwicklungsplan

Das Projekt befindet sich derzeit in der MVP-Phase (0.2.0-MVP) mit implementiertem Kernfunktionalitätsrahmen, grundlegenden Überwachungsfähigkeiten, Selbsttest-Funktionen und CI/CD-Testfunktionen. Ausführliche Aktualisierungshistorie finden Sie im [CHANGELOG](../../../CHANGELOG).

## Schnellstart

### 1. Repository klonen

```bash
git clone https://gitlab.dongzeyang.top/ydzat/moeai-c.git
# oder
git clone https://github.com/ydzat/moeai-c.git
cd moeai-c
```

### 2. Projekt erstellen

Zuerst kompilieren Sie das Kernelmodul und das Kommandozeilentool:

```bash
make all     # Kernelmodul erstellen
make cli     # Kommandozeilentool erstellen
```

Wenn Sie es zum ersten Mal verwenden, können Sie die automatische Konfiguration ausführen, um die Systemumgebung zu erkennen:

```bash
make configure    # Automatisch die .config.mk Konfigurationsdatei erstellen
```

### 3. QEMU-Testumgebung

MoeAI-C bietet zwei QEMU-Testmethoden, beide ohne Änderung des Hostsystems:

#### Methode 1: Standardtest

```bash
make qemu-test    # Vollständige QEMU-Testumgebung
```

Dies wird:
- Das Kernelmodul kompilieren
- Das Kommandozeilentool erstellen
- Test-initramfs erstellen
- QEMU-Virtuelle Maschine für Tests starten

#### Methode 2: CI Automatisierte Tests (Empfohlen)

```bash
make qemu-ci-test    # Automatisierte Tests für CI/CD-Umgebungen geeignet
```

Diese Methode ist für automatisierte Umgebungen konzipiert und führt einen vollständigen Selbsttestprozess aus.

### 4. Lokale Systemtests

> Wenn Sie sich für die Ausführung in QEMU entschieden haben, können Sie die Schritte 4/5 überspringen.

Um in einem tatsächlichen System zu testen (erfordert Root-Rechte, bitte mit Vorsicht vorgehen):

```bash
sudo make test    # Modul laden und auf dem lokalen System testen
```

### 5. Installation und Deinstallation

Installieren Sie das Modul in einem tatsächlichen System (erfordert Root-Rechte):

```bash
sudo make install    # Modul im System installieren
sudo make uninstall  # Modul vom System deinstallieren
```

### 6. Aufräumen

```bash
make clean          # Build-Dateien bereinigen
make clean-test     # Testumgebung bereinigen
```

## Modulverwendung

Nachdem das Modul geladen wurde, kann es auf folgende Weise verwendet werden:

### Kommandozeilentool

Das `moectl`-Tool bietet verschiedene Funktionen:

```bash
# Modulstatus anzeigen
moectl status

# Selbsttest durchführen
moectl selftest

# Speicherüberwachungsinformationen anzeigen
moectl mem-status

# Hilfeinformationen anzeigen
moectl help
```

### procfs-Schnittstelle

Das Modul bietet eine procfs-Schnittstelle zur Statusanzeige und Steuerung:

```bash
# Modulstatus anzeigen
cat /proc/moeai/status

# Steuerbefehl ausführen
echo "command_name" > /proc/moeai/control

# Detaillierte Hilfe anzeigen
cat /proc/moeai/help
```

## Erweiterte Konfiguration

### Benutzerdefinierte Kompilierungsoptionen

Sie können Kompilierungsoptionen anpassen, indem Sie die Datei `.config.mk` erstellen oder bearbeiten:

```bash
# Beispiel .config.mk Datei
KERNEL_DIR=/path/to/kernel/source
QEMU_KERNEL_SRC=/path/to/qemu/kernel
DEBUG=1  # Debug-Modus aktivieren
```

### Debug-Modus

Die Aktivierung des Debug-Modus liefert mehr Protokollinformationen:

```bash
make DEBUG=1 all    # Mit aktiviertem Debug-Modus kompilieren
```

## Fehlerbehebung

### Modulladefehler

Wenn das Modul nicht geladen werden kann, mögliche Ursachen und Lösungen:

1. **Versionskonflikt** - Stellen Sie sicher, dass das Modul mit der aktuellen Kernelversion kompatibel ist:
   ```bash
   make qemu-check    # Kompatibilität prüfen
   ```

2. **Fehlende Abhängigkeiten** - Installieren Sie notwendige Entwicklungspakete:
   ```bash
   # Für Debian/Ubuntu-Systeme
   sudo apt install linux-headers-$(uname -r) build-essential
   
   # Für RHEL/Fedora-Systeme
   sudo dnf install kernel-devel kernel-headers gcc make
   ```

3. **Signierungsprobleme** - Wenn der Kernel signierte Module erzwingt, müssen Sie möglicherweise Secure Boot deaktivieren oder das Modul signieren

### QEMU-Testprobleme

1. **"No working init found" Fehler** - Stellen Sie sicher, dass initramfs korrekt erstellt wurde:
   ```bash
   make clean-test    # Testumgebung bereinigen
   make qemu-test     # Testumgebung neu erstellen
   ```

2. **moectl-Tool nicht gefunden** - Stellen Sie sicher, dass das CLI-Tool kompiliert wurde:
   ```bash
   make cli           # CLI-Tool neu kompilieren
   ```

3. **procfs-Schnittstelle existiert nicht** - Prüfen Sie, ob das Modul korrekt geladen wurde:
   ```bash
   # In der QEMU-Umgebung
   dmesg | grep moeai
   ls -la /proc | grep moeai
   ```

## Entwicklungsleitfaden

Wenn Sie Code zu MoeAI-C beitragen möchten, beachten Sie bitte den folgenden Prozess:

1. Branch-Namenskonvention: `feature/your-feature` oder `fix/your-fix`
2. Führen Sie Tests vor dem Einreichen durch: `make qemu-ci-test`
3. Befolgen Sie den Kernel-Codierungsstil: `make check`
4. Fügen Sie bei der Einreichung eines PR eine detaillierte Beschreibung hinzu

## Anforderungen

- Linux-Kernelversion >= 5.4
- GCC 7.0+
- make 4.0+
- QEMU 4.0+ (nur für Tests)
- Kernel-Entwicklungspaket (passend zur Ziel-Kernelversion)
- Optional: busybox (zum Erstellen der Testumgebung)

## Projektarchitektur

MoeAI-C verwendet ein modulares Design, das hauptsächlich die folgenden Komponenten enthält:

- **Kernsystem** (`src/core/`) - Bietet Basisframework und Initialisierungslogik
- **Funktionsmodule** (`src/modules/`) - Implementiert verschiedene Funktionen, wie Speicherüberwachung
- **IPC-Schnittstelle** (`src/ipc/`) - Beinhaltet procfs- und netlink-Kommunikation
- **Dienstprogrammbibliothek** (`src/utils/`) - Bietet allgemeine Funktionen wie Protokollierung, Puffer usw.
- **Kommandozeilentool** (`cli/`) - Bietet benutzerfreundliche Verwaltungsschnittstelle

Für Details sehen Sie bitte [design.md](./design.md), um die detaillierte Projektarchitektur und das Design zu verstehen.

## Lizenz

Dieses Projekt steht unter der **GNU GENERAL PUBLIC LICENSE Version 2** Lizenz. Siehe die [LICENSE](../../../LICENSE)-Datei für Details.

## Beiträge

Beiträge sind durch folgende Methoden willkommen:
- Reichen Sie Issues ein, um Fehler zu melden oder neue Funktionen vorzuschlagen
- Reichen Sie Pull Requests ein, um den Code zu verbessern
- Verbessern Sie Dokumentation und Testfälle

## Betreuer

- @ydzat - Projektgründer und Hauptbetreuer

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