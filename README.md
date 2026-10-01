# JerzyGen 1.0 — samogeneratywny syntezator dla KORG drumlogue

Instrument USER ENGINE sam tworzy dźwięki i rozwijający się motyw muzyczny.
Nie wymaga sampli, klawiatury MIDI ani nut w sekwencerze drumlogue.
Ma cztery głosy, własne oscylatory, filtr, obwiednie, FM i stereo echo.

## Wgranie i uruchomienie

1. Wyłącz drumlogue, podłącz USB TO HOST do komputera i włącz urządzenie z przytrzymanym REC.
2. Skopiuj `jerzy_gen.drmlgunit` do `Units/Synths/`.
3. Bezpiecznie wysuń dysk i naciśnij PLAY/STOP, aby opuścić tryb pamięci USB.
4. W sekcji USER wybierz `JerzyGen`. Ustaw głośność partii USER i upewnij się, że nie jest wyciszona.
5. `RUN = On` uruchamia automatyczne granie. `RUN = Off` płynnie wycisza instrument.

Domyślne ustawienia generują melodię w pentatonice molowej od MIDI 48 (C),
w zakresie dwóch oktaw, w rytmie ósemek. Po wyborze instrumentu dźwięk
pojawia się bez konieczności uruchamiania sekwencera.

## Parametry

| Parametr | Działanie |
|---|---|
| RUN | Automatyczne granie On/Off |
| CLOCK | Device: tempo drumlogue; Internal: tempo z parametru BPM |
| BPM | Własne tempo 30–240 BPM; używane przy CLOCK = Internal |
| RATE | Długość kroku: 1/2, 1/4, 1/8 lub 1/16 |
| SCALE | Pentatonika molowa/durowa, molowa, durowa, dorycka lub chromatyczna |
| ROOT | Podstawa motywu, MIDI 24–84 |
| OCTAVES | Zakres motywu: 1–3 oktawy |
| DENSITY | Prawdopodobieństwo zagrania nuty w kroku, 0–100% |
| MUTATION | Prawdopodobieństwo zmiany nuty motywu, 0–100% |
| SEED | Ziarno deterministycznego generatora, 0–32767 |
| WAVE | Sine, Triangle, Saw |
| TONE | Filtr dolnoprzepustowy 100–16000 Hz; 16000 wyłącza filtr |
| ATTACK | Narastanie 2–2000 ms |
| DECAY | Czas opadania obwiedni do około -60 dB, 50–8000 ms |
| FM | Głębokość modulacji fazy sinusoidą o podwójnej częstotliwości |
| DETUNE | Rozstrojenie drugiego oscylatora 0–30 centów |
| ECHO | Ilość stereo echo, 0–75% |
| FEEDBACK | Sprzężenie zwrotne echa, 0–85% |
| WIDTH | Szerokość losowego rozmieszczenia głosów w stereo |
| LEVEL | Poziom wyjściowy 0–100% |

## Jak działa generowanie

Generator tworzy 16 nut motywu przez losowy spacer w wybranej skali.
Każdy krok może zagrać nutę albo pauzę zgodnie z DENSITY.
MUTATION stopniowo podmienia nuty motywu na nowe nuty tej samej skali.
Losowane są także dynamika i panorama. Cztery głosy pozwalają dźwiękom
nakładać się; przy większej liczbie nakładających się nut najstarszy głos
jest zastępowany. Przy MUTATION = 0 układ wysokości pozostaje stały,
ale dynamika, panorama i pauzy nadal mogą się zmieniać.
SEED daje ten sam przebieg od początku po wyłączeniu i włączeniu RUN,
jeśli pozostałe ustawienia są identyczne. Echo może zachować poprzedni ogon;
pełny restart instrumentu czyści także jego pamięć.

## Tempo, MIDI i ograniczenia

- CLOCK = Device pobiera tempo z SDK. Przed pierwszym komunikatem tempa
  obowiązuje 120 BPM. Echo odpowiada 3/4 ćwierćnuty, maksymalnie jednej sekundzie.
- Zgodność tempa nie oznacza automatycznego wyrównania do początku taktu.
  SDK nie dostarcza temu instrumentowi pozycji transportu ani dedykowanego
  zdarzenia PLAY/STOP. Używaj RUN do pewnego startu i zatrzymania.
- Note On ustawia ROOT i rozpoczyna motyw od początku. Gate On również
  uruchamia motyw od początku. Note Off/Gate Off nie przerywają automatycznego grania.
  Nie umieszczaj powtarzanych triggerów na partii USER, jeśli zależy Ci na
  swobodnym przebiegu; każdy trigger może ponownie uruchomić motyw.
- All Notes Off zatrzymuje generowanie. Aby wrócić, przełącz RUN Off/On
  albo wyślij Note On/Gate On. Pitch bend zmienia wysokość o ±2 półtony.
- Wysokości są ograniczone do MIDI 108. Samogeneratywność oznacza własny
  generator melodii i brzmienia; instrument nie zmienia parametrów innych partii.
- Efekty i wiele zainstalowanych jednostek współdzielą zasoby sprzętu.
  Ten instrument używa około 390 kB pamięci na bufor echa i tablicę sinusoidy.

## Sprawdzenie

Kod przeszedł testy generowania dźwięku bez Note On, stereo, powtarzalności,
zatrzymania, obu źródeł tempa i skrajnych ustawień. GitHub Actions uruchamia
AddressSanitizer/UndefinedBehaviorSanitizer, a następnie kompilację ARM
oficjalnym procesem SDK KORG. Paczka zawiera wynik `.drmlgunit`, tę instrukcję,
licencję SDK i SHA-256. Próba na fizycznym drumlogue pozostaje do wykonania.

SDK: KORG logue-sdk `e84899398dc1a71468a79e293fe68e3ab22e19b6`, BSD 3-Clause.
Kontener: `xiashj/logue-sdk@sha256:e4d85a16c38dc4d34b0e93cabb6378df21729688dbcd88808c84e8d41aa0c9ba`.
Lokalny, niezarejestrowany identyfikator dewelopera: `0x4A424B31`.
