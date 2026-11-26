# El motor de DESGRACIA y cómo hacer sonidos

Todo se genera en tiempo real, sin samples. Flujo de una voz:

```
OSC B ──FM──▶ OSC A ─┐
  │                  ├─ mix (A, B, RING = A×B) + SUB + NOISE ─▶ FILTRO (drive) ─▶ AMP ENV ─▶ FX ─▶ salida
  └──────────────────┘          × UNISON (hasta 7, con DETUNE/SPREAD y DRIFT)
```

## Osciladores (A y B)
Cada uno: WAVE, OCT (±3), SEMI (±12), FINE (±100 cents), LEVEL y una perilla **SHAPE** que cambia según la onda
(el nombre de la perilla cambia en pantalla):

| Onda | SHAPE hace | Para qué |
|---|---|---|
| SINE | FOLD: auto-FM, la va saturando hasta parecer serrucho suave | bips, subs, silbidos |
| TRI | FOLD: wavefolder | organitos, pads finos |
| SAW | OCT+: suma un serrucho una octava arriba (2º armónico fuerte, sonido hueco) | leads huecos, reese |
| PULSE | WIDTH: 0 = cuadrada, arriba = pulso finito y nasal | el lead nasal típico |
| TOY | STEPS: seno cuantizado tipo teclado de juguete (menos escalones = más roto) | juguetes, plucks 8-bit |
| FORMANT | FORMNT: resonancia tipo Casio CZ, formante que se mueve | voces raras, "vocal" sin vocal |
| BUZZ | NARROW: pulso muy angosto, zumbido | mosquitos, chicharras |
| METAL | CLANG: FM interna inarmónica (ratio 2.76) | campanas, chapas, vidrio |

Todas las ondas con saltos usan PolyBLEP (menos aliasing). TOY y el CRUSH ensucian a propósito.

**B>A MOD:** `FM B>A` modula la fase de A con B (ratios enteros = timbre armónico, ratios raros = metálico).
`FM ENV` suma FM siguiendo la **FILTER ENV**: ataque brillante que se apaga (mallets, rhodes, campanas).
`RING` agrega A×B. **P.ENV**: salto de pitch al ataque (±48 st) con su decay; `P.ENV TO = OSC B` lo aplica
solo a B (chirps de FM).

## Source
SUB (seno/tri/cuadrada, −1 o −2 oct) · NOISE con COLOR (oscuro → brillante) · UNISON 1–7 voces con DETUNE
(hasta ±25 cents) y SPREAD estéreo · **DRIFT**: la afinación imperfecta. Cada voz se mueve sola unos cents,
cada nota entra un poquito desde abajo y cada voz de unison tiene su desafinación propia. Es el ingrediente
principal del carácter "barato": 0.1 = casi afinado, 0.5 = cassette viejo.

## Filtro
SVF por voz (estéreo). Tipos LP12, LP24, BP, HP, NOTCH y **VOWEL** (tres formantes A-E-I-O-U; CUTOFF elige la
vocal y la envolvente/LFO la mueven). RESO, DRIVE (saturación antes del filtro), ENV (±, usa la FILTER ENV),
KEY (seguimiento de teclado), VEL (velocidad → brillo).

## Envolventes, LFOs, voz
- AMP ENV y FILTER ENV: ADSR (ataque lineal, caídas exponenciales). VEL en AMP = cuánto afecta la velocidad al volumen.
- LFO 1 y 2: SINE, TRI, SAW, SQUARE, S&H, DRIFT (random suave). RATE en Hz o SYNC al tempo del DAW (1/1 a 1/32,
  tresillos y puntillo). DEST: PITCH, CUTOFF, SHAPE (PWM), FM, AMP, PAN, B PITCH. DELAY hace entrar el LFO de a poco
  (vibrato que aparece cuando sostenés la nota). La rueda de modulación suma profundidad al LFO 1.
- VOICE: POLY (16 voces, con robo de voces sin clicks), MONO (siempre re-dispara), LEGATO (notas pisadas no re-disparan
  y deslizan). GLIDE, BEND (rango del pitch bend), WIDTH estéreo y MASTER.

## Efectos (en este orden)
DRIVE (Soft / Hard / Fold / Tube, 2x oversampling) → CRUSH (bits + sample rate) → EQ (low shelf 140 Hz, mid
paramétrico, high shelf 5.5 kHz) → CHORUS → DELAY (sync o ms, feedback con filtro, ping-pong) → REVERB.
Prender y apagar cualquier efecto hace un fundido corto (sin clicks) y el delay/reverb dejan sonar la cola.

## Vocoder (página VOCODER)
Funciona como un talkbox: el synth pasa por 3 filtros de formante que se mueven y "pronuncian" vocales, con un gate
por sílaba. No hay ruido agregado, así que todo lo que suena es tu synth hablando. CLARIDAD (bandas) y METAL
afinan los formantes, RAPIDEZ es qué tan rápido cambia de vocal y T-K agrega un golpecito de consonante.
- Dice sílabas en tempo según QUE DICE: WA-WA-WA (U→A en cada paso), DES-GRA-CIA, A-E-I-O-U, U-A U-A, YOI-YOI (I→O), HABLA RANDOM,
  RUIDOS RANDOM (vocales, alturas y soplidos al azar: ruidos de robot) y WAH LENTO, cambiando cada CAMBIA CADA
  (1/1 a 1/32). Con el DAW en play se engancha a la grilla; sin play arranca con la primera nota.
- Nivel automático: sigue el volumen del synth seco y le agrega armónicos suaves, así suena igual de fuerte
  sobre cualquier sonido (hasta sobre una senoidal).
- El dado de voz (página VOCODER) tira patrón, velocidad, bandas y formantes al azar sobre el sonido actual.
- VOZ +/- corre los formantes (nene / monstruo), METAL es la Q de las bandas, SSSS deja pasar las consonantes,
  MEZCLA entre synth normal y vocoder.

## Perillas fáciles (macros)
Se aplican encima del preset (en el centro = sin cambio): BRILLO (cutoff x2^3 para cada lado), MUGRE (drive del filtro,
saturación y bits del crush), DESAFINE (drift + detune del unison), MOVIMIENTO (más LFO; si no hay, agrega un DRIFT
lento al cutoff), ESPACIO (prende y sube reverb y delay), LARGO (acorta/alarga decays y releases).

## Dados
- **Dado de categoría:** elige un preset de fábrica al azar de esa categoría, mueve sus parámetros dentro de rangos
  de la categoría (puede cambiar la onda por otra que tenga sentido, prender/apagar crush o delay) y aplica reglas:
  BASS = mono/legato, ancho 0, octava baja y sin reverb; plucks y metálicos = sustain 0; ambient = ataque lento;
  VOCODER = vocoder prendido, sobre todo WA-WA-WA y RUIDOS RANDOM, con bandas y formantes al azar;
  CRANK = siempre un gate en tempo sobre el volumen (1/8 a 1/32) y a veces un segundo pulso en filtro, forma, FM o paneo.
- **RANDOM TOTAL:** cualquier cosa en todo el motor, dentro de rangos que suenan (nunca mudo ni saturado a muerte).

## Recetas
- **Lead nasal tipo "lata":** PULSE con WIDTH 0.5–0.7, filtro BP ~1.2k RESO 0.5, KEY alto, LEGATO + GLIDE 40–70 ms,
  DRIFT 0.3, LFO1 PITCH 0.12 con DELAY 0.3 s, DRIVE HARD poquito.
- **Juguete:** TOY + SINE una octava arriba bajito, DRIFT 0.5, CRUSH 7 bits mezclado al 50%.
- **Pluck de vidrio:** METAL CLANG bajo + SINE +2 oct, AMP S=0 D≈0.4, delay 1/8 corto.
- **Mallet FM:** SINE A, SINE B +2 oct con LEVEL 0, FM ENV 0.5, FILTER ENV D ≈0.1, S=0.
- **Campana rara:** SINE A + SINE B SEMI +6 FINE +17 (ratio inarmónico), FM ENV 0.45, decay largo, reverb.
- **808 sucio:** SINE −1 oct, P.ENV +12 con decay 40 ms, AMP S=0 D≈2.5 s, DRIVE TUBE 0.5 (agrega 2º y 3º armónico
  que se escucha en parlantes chicos), LEGATO + GLIDE, WIDTH 0.
- **Pad frío:** SAW OCT+ 0.5, UNISON 5, DETUNE 0.3, LP12 1.5k, ataque 1 s, EQ low −7 dB, reverb grande.

Truco: los dados de cada categoría y RANDOM TOTAL generan sonidos nuevos dentro de rangos musicales. Si te gusta uno, GUARDAR.
Doble click en una perilla la vuelve al valor por defecto.
