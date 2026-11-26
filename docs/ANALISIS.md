# Análisis de las referencias → decisiones de diseño

Se extrajo el audio de los dos videos (ffmpeg, 44.1 kHz) y se analizó con librosa: energía por bandas, centroide
espectral, planitud, ancho estéreo (side/mid), seguimiento de pitch (pYIN) sobre la parte armónica, perfil de armónicos
y espectrogramas por tramos.

**Video 1** (stream armando un beat en FL Studio, 2:06, mucha voz hablada encima):
- Casi mono (side/mid 0.007). Energía concentrada en 250 Hz–1 kHz (74%), muy poco arriba de 4 kHz (1%).
- Tonos de juguete y plucks con **2º armónico más fuerte que el fundamental** (perfil 0.41 / 0.61 / 0.31…): sonido
  hueco, "octavado".
- Motivos muy agudos: bips casi senoidales alrededor de 2.8 kHz con otro tono a una **quinta** (≈4.2 kHz), pulsados
  ~4 por segundo, secos.
- 17% de los frames con movimiento de pitch de 0.2–1 semitono: glides cortos y vibrato. Desvío de afinación ±10 cents.
- Un pluck tipo campanita con parciales enteros (512/1024/1600 Hz) y un sostenido muy armónico con vibrato arriba.

**Video 2** ("alardE" de Cero*, 0:43):
- Más ancho (side/mid 0.24), más brillo y ruido entre 4–8 kHz (hats, aire), centroide ~2.8 kHz.
- Tonalidad en **Fa menor** (F, G, G#, A#, C dominan). Desafinación ±40 cents de rango intercuartil: pitch inestable.
- Capa melódica con **armónicos impares dominantes** (3º el más fuerte): pulso/cuadrada nasal.
- 808 con fundamental ~45–55 Hz y armónicos claros a 2× y 3×: sub saturado, con glides escalonados.

## Cómo se tradujo
| Observación | En el sinte |
|---|---|
| 2º armónico fuerte, sonido hueco | SAW con SHAPE "OCT+", TOY con componente 2×, SINE + B una octava arriba en varios presets |
| Impares dominantes, nasal | PULSE con WIDTH, filtro BP/VOWEL, presets "Nariz de Lata", "Cuadrada Barata", "Mosquito" |
| Bips a 2.8 kHz con quinta | preset "Bip Bip": dos senos a una quinta (+7 st), decay 90 ms, sin efectos |
| Pitch inestable ±10–40 cents | parámetro DRIFT (deriva por voz + entrada desde abajo + desafinación por unison) |
| Glides cortos, vibrato | LEGATO + GLIDE 30–80 ms, LFO1 a pitch con DELAY en casi todos los leads |
| 808 con 2º/3º armónico | "808 Desgracia" y "Sub Sucio": seno + DRIVE TUBE (asimétrico = armónicos pares), mono |
| Medios mandan, agudos secos | filtros con KEY TRACK, EQ con low cut en pads, reverb/delay solo como detalle |
| Mono / poco ancho | WIDTH por preset (bajos en 0), unison con SPREAD moderado |
| Lo-fi, clipping leve | CRUSH, DRIVE y un techo suave en la salida |

Los niveles se nivelaron midiendo el render de cada preset con la misma frase en Fa menor (melodía, acorde,
nota grave, motivo agudo, legato): leads ≈ −17 dB RMS activo, plucks/metal ≈ −19, pads ≈ −20, bajos ≈ −13.
