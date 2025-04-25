# MoeAI-C Projekt-Designdokument

## 📌 Projektübersicht

**MoeAI-C** ist ein KI-Assistentenprojekt auf Systemebene, das auf Linux-Kernelmodulen basiert. Es zielt darauf ab, ein intelligentes Modul mit Ressourcenmanagement, Systemüberwachung, Ereignisreaktionen und KI-Entscheidungsfähigkeiten aufzubauen. In Zukunft wird die Integration mit LLM-Modellen im Benutzerbereich unterstützt, um dynamische Richtliniensteuerung und intelligente Systemreaktionen zu ermöglichen.

---

## 📁 Projektstrukturübersicht

```text
moeai-c/
├── include/                         # Alle öffentlichen Header-Schnittstellen
│   ├── core/                        # Kernel-Kerndefinitionen (Modul, Event usw.)
│   │   ├── module.h
│   │   ├── scheduler.h
│   │   └── state.h
│   ├── modules/                     # Schnittstellen für verschiedene Funktionsmodule
│   │   ├── mem_monitor.h
│   │   ├── net_guard.h
│   │   └── fs_logger.h
│   ├── ipc/                         # Kernel-Userspace-Kommunikationsstrukturen
│   │   ├── netlink_proto.h
│   │   └── procfs_interface.h
│   ├── utils/                       # Gemeinsame Datenstrukturen, Makros, Log-Definitionen
│   │   ├── logger.h
│   │   ├── ring_buffer.h
│   │   └── common_defs.h
│   └── data/                        # Statussnapshots, Verlaufsdatenstrukturen
│       ├── stats.h
│       ├── snapshot.h
│       └── history.h

├── src/                             # Kernelmodul-Quellcode-Implementierung
│   ├── core/                        # Initialisierung, Modulverwaltung, Event-Scheduling
│   │   ├── init.c
│   │   ├── module_loader.c
│   │   ├── scheduler.c
│   │   └── event_loop.c
│   ├── modules/                     # Spezifische Modulimplementierungen
│   │   ├── mem_monitor.c
│   │   ├── net_guard.c
│   │   └── fs_logger.c
│   ├── ipc/                         # Kommunikationsimplementierung (Interaktion mit Userspace)
│   │   ├── netlink.c
│   │   └── procfs.c
│   ├── utils/                       # Gemeinsame Funktionsimplementierungen
│   │   ├── logger.c
│   │   └── ring_buffer.c
│   ├── data/                        # Datenspeicherimplementierung
│   │   ├── stats.c
│   │   ├── snapshot.c
│   │   └── history.c
│   └── main.c                       # Moduleingang/-ausgang (init_module/cleanup_module)

├── cli/                             # Userspace-Befehlszeilentools (ähnlich systemctl/dnf)
│   ├── moectl.c                     # Hauptbefehlstool (moectl status / clean)
│   └── parser.c                     # Parameteranalyse

├── agent/                           # Userspace KI-Assistent (Daemon)
│   ├── main.py                      # Hauptdaemon, startet LLM-Schnittstelle
│   ├── handler/
│   │   ├── event_handler.py         # Empfängt Kernel-Events, reagiert
│   │   ├── net_guard_handler.py
│   │   └── memory_policy.py
│   └── model/
│       ├── openai_wrapper.py        # OpenAI-Schnittstellenumhüllung
│       └── local_llm.py             # llama.cpp / ggml Modellverwaltung

├── test/                            # Unit-Tests / Modultests
│   ├── test_runner.c
│   ├── test_mem.c
│   ├── test_utils.c
│   └── mocks/                       # Stub-Code, der Kernelverhalten simulieren kann
│       └── fake_kernel.c

├── build/                           # Build-Ausgabe (.ko-Dateien usw., automatisch durch Makefile erstellt)

├── Makefile                         # Hauptbuild-Skript (kann .ko + CLI + Tests kompilieren)
├── README.md                        # Projekteinführung und Verwendungsanweisungen
├── LICENSE                          # Projektlizenz (z.B. BSD / MIT / LGPL)
├── .gitignore                       # Konfiguration für zu ignorierende Dateien
└── .gitlab-ci.yml                   # GitLab CI/CD-Workflow (Build + Test)

```

---

## 🔧 Kern-Modulverantwortlichkeiten

| *Bereich \| Funktionsfokus \| Unterstützter Inhalt* |
| -------------------- |

include/ | Schnittstellen- und Abstraktionsschicht | Modulübergreifende/komponentenübergreifende einheitliche Header-Dateiverwaltung

src/ | Kernimplementierungsschicht | Linux-Modul-Hauptlogik, einschließlich Scheduling, Modulregistrierung, Kommunikation usw.

data/ | Statusmomentaufnahmen, Richtlinienaufzeichnungen | Daten und historische Stichproben zur Unterstützung von KI-Modellentscheidungen

ipc/ | Userspace-Interaktionsschnittstelle | Unterstützung für Netlink, /proc, Shared Memory usw.

cli/ | Terminal-Befehlstools | Unterstützung für moectl-Stil, kompatibel mit Kommandozeilenaufrufen

agent/ | KI-Ausführungsschicht (Userspace) | Ereignisse empfangen → KI-Modellinferenz → Steuerungsrichtlinien zurücksenden

test/ | Automatisierte Testunterstützung | Unterstützung für Mocks, Modultests, nutzbar für CI/CD

---

## 📐 Kommunikationsmechanismusdesign

- MVP-Phase verwendet `/proc/moeai`, um Befehls- und Statusinteraktion zwischen Userspace und Kernel zu implementieren
- Spätere Phasen werden Netlink für strukturierte Nachrichtenübermittlung integrieren, Ereignisberichte und Richtlinienverteilung unterstützen
- Erweiterbare Unterstützung für Shared-Memory-Mechanismus zur Übertragung hochfrequenter Abtastdaten
- Userspace-`agent` interagiert bidirektional durch Ereignisbus-Überwachung oder Abfrageschnittstellen

---

## 🧠 KI- und Datenflussintegrationsplan

- Kernelmodul ist verantwortlich für Ereigniserkennung (wie Speicherüberlauf, verdächtige Portverbindungen usw.) und das Übertragen von Statusinformationen
- Userspace-`agent/` empfängt Ereignisse und ruft externe KI-Modelle zur Inferenz auf
- Modelle umfassen:
  - OpenAI-Schnittstelle (über API-Anfragen)
  - Lokal bereitgestellte Modelle (wie llama.cpp, läuft in agent/main.py)
- KI-Analyseergebnisse werden über Netlink oder `/proc/moeai` an das Kernelmodul zurückgesendet, um das Verhalten zu steuern (wie dynamische OOM-Richtlinien)

---

## 📢 Benutzerinteraktionsschnittstelle

- `cli/moectl` unterstützt Befehlsformate:
  - `moectl status`: Ausgabe des aktuellen Systemanalysestatus
  - `moectl clean`: Auslösung der Speicherfreigabelogik
  - `moectl protect firefox`: Markiert den Firefox-Prozess als hohe Priorität
- `moectl` wird Anweisungen über procfs schreiben, mit zukünftiger Migration zu Netlink-Befehlskapselung

---

## 📋 Logging-System-Design

- `utils/logger.c` implementiert eine einheitliche Protokollierungsschnittstelle (unterstützt printk / Puffer / Userspace-Rücklesung)
- Protokolle können von Userspace-CLI-Tools oder Agent über `/proc/moeai_log` abgerufen werden
- Unterstützung für Protokollebenensteuerung (DEBUG / INFO / WARN / ERROR)
- Protokollstruktur verwendet Ringpufferspeicherung, um sicherzustellen, dass sie den Kernel-Hauptprozess nicht blockiert

---

## 🛠 Bauen und Testen

- Makefile verwaltet das Bauen aller Module, Tools und Tests
  - Unterstützt das Bauen von `.ko`-Modulen (basierend auf Kernel-Headern)
  - Unterstützt Userspace-Testbauen und -ausführung
- Tests werden mit Unity- oder CMocka-Frameworks geschrieben
- GitLab CI führt automatisch Build- und Test-Workflows aus, gibt Build-Artefakte aus

---

## ✅ Zu implementierende Meilenstein-Ziele

1. **Verzeichnisstruktur und Initialisierungsdateien erstellen**
   - Alle Verzeichnisse, grundlegende `.c/.h`-Framework-Dateien erstellen
   - Makefile und Build-Umgebung initialisieren

2. **Funktionale Verantwortlichkeiten der Code-Dateien entwerfen**
   - Zielfunktionalität für jedes Modul klar definieren (Core/Module/IPC/CLI/Agent)
   - Funktionsschnittstellen und Strukturdaten für jede `.c`-Datei klären

3. **Beziehungen zwischen Code-Dateien bestimmen**
   - Parameterübergabe: Strukturen zur Parameterpaketierung verwenden, einheitliche Befehlsstrukturen für das Scheduling verwenden
   - Modulübergreifendes Header-Datei-Pfaddesign (vereinheitlichtes include/...)

4. **Interaktionsmechanismen zwischen Kernkomponenten definieren**
   - Agent löst CLI-Befehle aus → CLI schreibt in /proc oder sendet Netlink
   - Kernelmodul empfängt Nachrichten → ruft Modulprozessoren auf → Statusaktualisierungen oder Feedback

5. **Aufrufsequenz-Anwendungsfälle schreiben (Systembetriebsflussdiagramm)**
   - Systeminitialisierung → Modulregistrierung → Ereigniserkennung → Ereignisverteilung → Datenverarbeitung → Entscheidungsrückmeldung → Userspace-Reaktion

6. **Protokollierungs- und Debugging-Schnittstellen vorbereiten**
   - Unterstützung der `logger()`-Schnittstellenausgabe an Ringpuffer
   - `cat /proc/moeai_log` bereitstellen, um aktuelle Protokolle anzuzeigen

7. **Grundlegendes Testframework erstellen**
   - CMocka / Unity-Testeinstiegspunkte initialisieren
   - Mindestens ein Modul (wie mem_monitor) Unit-Test-Beispiel bereitstellen

8. **GitLab CI/CD-Anfangsvorlagen integrieren**
   - Modul `.ko` + CLI-Tools bauen
   - Tests ausführen und Abdeckungsberichte erstellen

9. **Modulare Erweiterungskonventionen und Code-Spezifikationsdokumente schreiben**
   - Vorlagenstrukturen und Registrierungsmethoden für zukünftige Modulergänzungen klar definieren
   - Codestil standardisieren (Einrückung, Kommentare, Funktionspräfixe usw.)

---

## 📄 Protokoll und Bereitstellung

- Projektquelloffenes Protokoll ist vorläufig als MIT/BSD festgelegt (wird basierend auf KI-Komponentenreferenzen finalisiert)
- Unterstützung für die Bereitstellung von Runner auf GitLab zur Ausführung des kompletten CI/CD-Workflows
- Zukünftige Unterstützung für automatische Modulbereitstellung, Synchronisierung von KI-Berechtigungen und Richtlinienmodellkonfigurationen

---