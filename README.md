# Dualis 3070 — Meteora Plugins

© 2026 Meteora Plugins. Tutti i diritti riservati. Vedi `LICENSE.txt`.

Compressore/limiter dual mono (VST3 / AU / Standalone) ricavato dalle catture Scriba Sweep del Fostex 3070.
I due canali sono diversi tra loro, come sull'hardware, e si possono collegare in stereo con la levetta LINK.

## Compilazione

Carica la cartella su GitHub: il workflow `.github/workflows/build.yml` parte da solo a ogni push
(oppure da *Actions → Build Dualis 3070 → Run workflow*). Negli *Artifacts* trovi:

- `MeteoraPlugins-Dualis3070-Windows` → VST3 + Standalone + LICENSE.txt
- `MeteoraPlugins-Dualis3070-macOS` → VST3 + AU + Standalone + LICENSE.txt (Intel e Apple Silicon)

Windows: copia `Dualis 3070.vst3` in `C:\Program Files\Common Files\VST3`.
macOS: il plugin non è firmato, dopo averlo copiato esegui `xattr -cr` sul file.

## Cosa viene dalle catture

| | CH 1 | CH 2 |
|---|---|---|
| Soglia (INPUT a 0 dB) | −23 dBFS | −21 dBFS |
| Ratio reale con manopola su 8 | 6:1 | 8:1 |
| Knee | 6 dB | 4 dB |
| Attacco / rilascio (burst −6 dBFS, 63%) | 28 / 93 ms | 27 / 89 ms |
| 2ª / 3ª armonica a −18 dBFS | −59,8 / −63,6 dB | −72,3 / −80,6 dB |
| Shelving acuti (misurato) | +6,4 dB a 15 kHz | +6,2 dB a 15 kHz |
| Acuti in MOD | +0,7 / +1,1 / +1,2 dB a 5 / 10 / 15 kHz | uguale |
| Bassi in MOD | +2,7 dB a 70 Hz | +2,7 dB a 74 Hz |

- **Detector RMS** con carica e scarica separate: riproduce l'attacco "veloce poi lento", il rilascio lineare in dB e
  i tempi che cambiano con la quantità di compressione (errore sugli inviluppi misurati: 0,26 dB CH1, 0,30 dB CH2).
- **Curva statica** con knee morbido presa dai 31 gradini della cattura DYN.
- **Colore**: 2ª e 3ª armonica nello stadio d'ingresso (al netto della scheda Tascam), uno shelving di primo ordine
  sugli acuti (ammorbidito rispetto alla misura) e una piccola campana sui 70 Hz.
- **Profondità**: il VCA aggiunge un filo di 2ª e 3ª armonica che cresce con la compressione (circa −50 / −64 dB
  con 10 dB di riduzione), come nei VCA analogici: quando il compressore lavora il suono prende corpo.

Tutti i numeri stanno in `Source/DualisModel.h`: se rifai una cattura basta aggiornare quella tabella.

## Comandi

- **COMP IN/OUT** (levetta sotto la scritta COMP): giù = IN, il compressore lavora; su = OUT, compressione
  esclusa. Il colore del canale e il gate restano attivi, come sull'hardware.
- **EXP GATE**: modellato sulle catture con la manopola su 7 (soglia −38 dBFS) e su 4 (soglia −60 dBFS): ogni
  tacca sposta la soglia di circa 7,3 dB (su 10 circa −16 dBFS, su 1 circa −82 dBFS). Sotto soglia c'è un expander
  morbido per circa 12 dB (fino a −8 dB di attenuazione), poi il gate si chiude di colpo con un range di ~28 dB.
  Apertura ~6 ms, hold ~200 ms, chiusura lineare ~25 dB/s. 0 = spento.
- **COMP RATIO**: 1, 2, 4, 8, 20, ∞ come sul pannello. Il CH1 ha la stessa pendenza "corta" misurata (8 → 6:1).
- **ATTACK** 0,2–20 ms e **RELEASE** 0,05–2 s: scalano i tempi misurati partendo dalla posizione della cattura
  (ATTACK su 20 ms, RELEASE circa 0,07 s, lette dalle foto).
- **INPUT / OUTPUT**: 0–10, al centro (5) 0 dB, 4 dB per tacca. L'INPUT spinge nella soglia e nelle armoniche.
- **LINK**: levetta giù (verso DUAL MONO) = canali indipendenti (sinistro = CH1, destro = CH2). Levetta su
  (verso LINK) = comanda il CH1: le sue
  manopole e la sua curva valgono per entrambi i lati, con un solo detector che ascolta sinistro e destro, quindi
  l'immagine stereo resta ferma. Le manopole e la levetta COMP del CH2 si muovono insieme a quelle del CH1 (non si
  toccano direttamente); togliendo il LINK il CH2 resta dove l'ha portato il CH1 e torna indipendente. Il colore
  (armoniche e acuti) resta quello di ciascun canale, quindi i due lati suonano comunque un po' diversi.
- **POWER**: il tasto con il LED ambra è il bypass. LED acceso = plugin attivo, LED spento = bypass. Il passaggio
  è una dissolvenza di 20 ms, senza click; il parametro "Bypass" è anche quello che usa la DAW.
- **Traccia mono**: lavora solo il CH1, anche se la DAW passa la traccia mono come stereo (L e R uguali entro
  −60 dB). Il rilevamento ha un'isteresi: diventa mono dopo 50 ms di L = R e torna stereo solo dopo 1 s di segnale
  chiaramente stereo, quindi non salta avanti e indietro sui colpi di cassa. Nel silenzio lo stato non cambia.
- **ORIGINAL / MOD** (levetta nel CH1, al posto della vecchia levetta decorativa, vale per entrambi i canali):
  su = ORIGINAL, il suono misurato del 3070 (acuti +6,4 dB, niente campana sui bassi, niente colore del VCA);
  giù = MOD, la versione modificata (+0,7 dB a 5 kHz, +1,1 dB a 10 kHz, +1,2 dB a 15 kHz, +2,7 dB a 70 Hz,
  armoniche del VCA). Il passaggio è una
  dissolvenza di 30 ms. Parte su MOD.
- **AUTO GAIN**: tasto con LED verde accanto a POWER. Confronta il volume (RMS, media ~0,8 s) di ingresso e
  uscita e corregge l'uscita perché esca allo stesso volume con cui è entrata, qualunque sia la regolazione di
  INPUT, OUTPUT, ratio o colore (correzione massima ±24 dB, ferma nel silenzio). Con LINK la correzione è
  uguale sui due lati.
- **GAIN REDUCTION**: i 5 LED si accendono da sinistra a 12 / 6 / 4 / 2 / 1 dB di riduzione; il LED quadrato a
  sinistra della fila si accende oltre 20 dB, quello EXP GATE / TAP GATE quando il gate sta chiudendo.

## Grafica

Lo sfondo è `Resources/panel.jpg`, ricavato dalla tua immagine: le manopole fotografate sono state tolte e al loro
posto ci sono scale e tacche nuove con i valori veri (ratio 1–∞, attack 0,2–20 ms, release 0,05–2 s, 0–10 per
EXP GATE, INPUT e OUTPUT). Le manopole sono `Resources/knob.png` (plastica nera zigrinata con luce e ombra, generata in 3D), centrate sulla
scala; l'indice dritto lo disegna il plugin. Lo sfondo è a doppia risoluzione per restare nitido sugli schermi Retina/4K.
Le posizioni sono nel blocco `Layout` all'inizio di `Source/PluginEditor.cpp`, in unità di layout (2514 x 283; l'immagine è 5028 x 566).

- Sul CH2 la quarta manopola è serigrafata **RELEASE**; i LED della gain reduction sono 12 / 6 / 4 / 2 / 1.
- Scritta POWER ridisegnata, tasto bypass (`Resources/power_on.png` / `power_off.png`) e logo Meteora Plugins sotto.
- Dove due manopole sono vicine (CH2) qualche numero intermedio resta solo come tacca; gli estremi ci sono sempre.
- La vecchia levetta decorativa "EXP GATE INPUT" del CH1 è diventata ORIGINAL / MOD.
- Passando sopra una manopola compare il valore esatto.

## Limiti

- Una sola posizione catturata per RATIO, ATTACK e RELEASE: le altre posizioni sono estrapolate.
- Il gate è stato catturato su 4 e su 7: tra le due e oltre la scala è interpolata in modo lineare.
- La modalità STEREO dell'hardware non è stata catturata.
- Lo shelving è adattato a 48 kHz (a 44,1 kHz cambia di 0,1–0,2 dB); a 96 kHz gli acuti sopra 10 kHz risultano
  circa 0,5 dB più morbidi.
