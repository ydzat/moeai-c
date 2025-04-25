# MoeAI-C Gesamtdesigndokument

## Dokumentationszweck

Dieses Dokument zielt darauf ab, die Modulverantwortungsteilung und die Beziehungen zwischen den Komponenten des MoeAI-C-Projekts detailliert zu beschreiben und die Anforderungen der Meilensteine 2 und 3 in design.md zu erfüllen:
- Zielfunktionalität für jedes Modul klar definieren
- Funktionsschnittstellen und Strukturdaten für jede `.c`-Datei spezifizieren
- Parameterübergabemethoden festlegen
- Modulübergreifendes Header-Datei-Pfaddesign definieren

## Dokumentstruktur

Dieses Dokument ist nach folgender Struktur organisiert:
1. Überblick über die Gesamtarchitektur
2. Modulverantwortungsverteilung
3. Komponentenkommunikation und -interaktion
4. Detailliertes Schnittstellendesign

Für eine detailliertere Implementierung jeder Komponente verweisen Sie bitte auf die entsprechenden Unterdokumente:
- [Kernmoduldesign](./core_design.md)
- [Funktionsmoduldesign](./modules_design.md)
- [Kommunikationsmechanismusdesign](./communication_design.md)
- [Design von Werkzeugen und Hilfskomponenten](./utils_design.md)
- [Datenverarbeitungsdesign](./data_design.md)

## 1. Überblick über die Gesamtarchitektur

Das MoeAI-C-Projekt übernimmt ein modulares Design, unterteilt in die folgenden Hauptteile:

```
            +--------------------+
            |   Benutzerebene    |
            | (CLI / Anwendungen)|
            +----------^---------+
                       |
            +----------v---------+
            | Benutzerraumangent  |
            | (Agent / Daemon)    |
            +----------^---------+
                       |
+----------------------------------------------+
|         Kernelraummodul MoeAI-C              |
|  +--------+    +--------+    +---------+    |
|  | Kern-  |<-->|Funktions|<-->| Komm.  |    |
|  | schicht|    | schicht |    | schicht |    |
|  +--------+    +--------+    +---------+    |
|  |        |    |        |    |         |    |
|  | Modul- |    |Speicher|    | Procfs  |    |
|  | verwalt|    |überw.  |    | Netlink |    |
|  | Event- |    |Netzwerk|    |         |    |
|  | planun.|    |Dateisy.|    |         |    |
|  +--------+    +--------+    +---------+    |
|                     ^                       |
|                     |                       |
|            +--------v---------+             |
|            |Werkzeug & Datenschicht|        |
|            |Logger/Ringpuffer      |        |
|            |Stats/Snapshot/Historie|        |
|            +------------------+             |
+----------------------------------------------+
```

## 2. Modulverantwortungsverteilung

### 2.1 Kernmodul (`src/core/`)

- **init.c**: Kontrolle der Modulinitialisierungssequenz
- **module_loader.c**: Dynamische Verwaltung von Funktionsuntermodulen
- **scheduler.c**: Ereignisplanungs- und Bearbeitungsframework
- **event_loop.c**: Ereignisschleife und Verwaltung geplanter Aufgaben

### 2.2 Funktionsmodule (`src/modules/`)

- **mem_monitor.c**: Speicherzustandsüberwachung und -verwaltung
- **net_guard.c**: Netzwerkaktivitätsüberwachung und -schutz
- **fs_logger.c**: Dateisystemaktivitätsüberwachung und -protokollierung

### 2.3 Kommunikationsmodul (`src/ipc/`)

- **procfs.c**: Implementierung der Procfs-Dateisystemschnittstelle
- **netlink.c**: Implementierung des Netlink-Kommunikationsprotokolls

### 2.4 Werkzeugmodul (`src/utils/`)

- **logger.c**: Implementierung eines einheitlichen Protokollierungssystems
- **ring_buffer.c**: Ringpuffer-Datenstruktur

### 2.5 Datenmodul (`src/data/`)

- **stats.c**: Echtzeit-Statistikdatenerfassung
- **snapshot.c**: Systemzustandsschnappschussverwaltung
- **history.c**: Historische Datenaufzeichnung und -analyse

### 2.6 Benutzerraumkomponenten

- **CLI-Tools** (`cli/`): Befehlszeilenschnittstellenwerkzeuge
- **Intelligenter Agent** (`agent/`): Implementierung des KI-Assistenten im Benutzerraum

## 3. Komponentenkommunikation und -interaktion

### 3.1 Interne Kommunikationsmethoden

- **Zwischen Kernmodulen**: Funktionsaufrufe + gemeinsame Datenstrukturen
- **Kern- und Funktionsmodule**: Callback-Registrierung über Modulschnittstellen
- **Funktions- und Datenmodule**: Funktionsaufrufe
- **Kommunikation und andere Module**: Callback-Registrierung + Ereignisbenachrichtigung

### 3.2 Externe Kommunikationsmethoden

- **Kernel und CLI-Tools**: Procfs-Schnittstelle
- **Kernel und intelligenter Agent**: Procfs + zukünftige Netlink-Erweiterung

### 3.3 Datenübertragungspläne

- **Datenkapselung**: Strukturen, die in Header-Dateien definiert sind, auf die von Modulen verwiesen wird
- **Ereignisübertragung**: Einheitliche Ereignisstruktur, einschließlich Typ, Priorität und Datenzeiger
- **Befehlsübertragung**: String-Befehle + Parameter, verarbeitet von einem einheitlichen Parser
- **Zustandsfreigabe**: Über zentralen Zustandsmanager

## 4. Detailliertes Schnittstellendesign

Für das detaillierte Schnittstellendesign jedes Moduls verweisen Sie bitte auf die entsprechenden Unterdesigndokumente. Im Folgenden sind allgemeine Schnittstellendesignprinzipien aufgeführt (folgen dem Linux-Kernel-Codierungsstil):

### 4.1 Funktionsbenennungskonventionen

- Verwenden Sie Kleinbuchstaben, wobei Wörter durch Unterstriche getrennt sind
- Modulpräfix: `moeai_<modulname>_`
- Häufig verwendete Operationsverben: `init`, `exit`, `start`, `stop`, `handle`, `get`, `set`
- Funktionsnamen sollten beschreibend sein und Abkürzungen vermeiden (es sei denn, es handelt sich um anerkannte Abkürzungen)
- Zum Beispiel: `moeai_mem_monitor_init()`, `moeai_logger_write()`

### 4.2 Strukturbenennungskonventionen

- Strukturnamen verwenden Kleinbuchstaben, wobei Wörter durch Unterstriche getrennt sind
- Modulpräfix: `moeai_<modulname>_`
- Verwenden Sie keine ungarische Notation oder Typensuffixe (wie `_t`)
- Strukturtags sind identisch mit Typnamen (Linux-Kernel-Stil)
- Zum Beispiel:
```c
struct moeai_event {
    /* Mitglieder */
};
```

### 4.3 Regeln für die Einbindung von Header-Dateien

- Reihenfolge der Einbindung von Header-Dateien:
  1. Die entsprechende Header-Datei für die aktuelle Datei (falls vorhanden)
  2. Kernel-Header-Dateien (<linux/...>)
  3. Standardbibliotheks-Header-Dateien
  4. Header-Dateien aus anderen Modulen
- Projekt-Header-Dateien verwenden relative Pfade (`#include "utils/logger.h"`)
- Jede Gruppe von Header-Dateien sollte durch eine Leerzeile getrennt sein

### 4.4 Fehlerbehandlungsstrategie

- Funktionsrückgabewerte: Rückgabe 0 bei Erfolg, negativer Fehlercode bei Misserfolg (unter Verwendung der Standard-Linux-Kernel-Fehlercodes wie -EINVAL, -ENOMEM usw.)
- Zeigerfunktionen: Rückgabe gültiger Zeiger bei Erfolg, NULL oder ERR_PTR bei Misserfolg
- Verwenden Sie die Makros `IS_ERR` und `PTR_ERR` zur Behandlung von Fehlerzeigern
- Fehler sollten im Protokollierungssystem protokolliert werden
- Zugewiesene Ressourcen sollten bereinigt werden, um Speicherlecks zu vermeiden
- Fehlerpfade sollten klar gekennzeichnet sein (unter Verwendung von goto-Anweisungen für Bereinigungscode)

### 4.5 Codeformatierungsstandards

- Verwenden Sie Tabs für die Einrückung, Breite von 8 Zeichen
- Eine Codezeile sollte 80 Zeichen nicht überschreiten
- Platzierung der Klammern:
  - Öffnende Klammer in derselben Zeile wie die Kontrollanweisung
  - Schließende Klammer in einer separaten Zeile
  - Klammern können für Einzelanweisungen if/for/while weggelassen werden
- Mehrfache Zuweisungsanweisungen sollten vertikal ausgerichtet sein
- Kommentarstil:
  - Verwenden Sie `/*...*/` für mehrzeilige Kommentare
  - Verwenden Sie `//` für einzeilige Kommentare