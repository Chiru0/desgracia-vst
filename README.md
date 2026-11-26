# DESGRACIA

Sintetizador VST3 (Windows 64-bit) de sonido digital sucio, inspirado en el under argentino / trap experimental.
Motor de síntesis propio (sin samples), 55 presets de fábrica en 10 categorías, vocoder, efectos integrados,
interfaz Frutiger Aero / Y2K (cielo, agua, burbujas, vidrio) y el personaje **Desgracia** animado: canta con el audio, rebota con cada nota, parpadea, se duerme si no tocás nada,
tiembla con el DRIVE y se pixela con el CRUSH.

- **Instalar:** [docs/INSTALACION.md](docs/INSTALACION.md)
- **Cómo funciona el motor y cómo hacer sonidos:** [docs/MOTOR.md](docs/MOTOR.md)
- **Análisis de las referencias y decisiones de diseño:** [docs/ANALISIS.md](docs/ANALISIS.md)
- **Compilar:** [docs/BUILD.md](docs/BUILD.md)

## Descargar

Cada push a `main` compila en GitHub Actions (Windows, Visual Studio 2022) y publica el release
[`latest`](../../releases/tag/latest) con:

- `Desgracia-Setup-Windows-x64.exe`: instalador (copia el plugin a `C:\Program Files\Common Files\VST3`).
- `Desgracia-Windows-x64.zip`: `Desgracia.vst3` + versión Standalone + `INSTALAR.bat` + banco de presets como archivos + docs.

Antes de empaquetar, el mismo build corre en Windows los tests de audio/MIDI/estado (`DesgraciaTest`)
y valida el VST3 con [pluginval](https://github.com/Tracktion/pluginval) (strictness 7). Si algo falla, no se publica.

## Novedades v2

- **Interfaz Frutiger Aero / Y2K** con 4 páginas: **TOCAR** (categorías y lista de sonidos), **SYNTH** (todo el motor),
  **VOCODER** y **FX**. Cada perilla tiene una ayuda en castellano al pasar el mouse.
- **Dado por categoría:** cada tarjeta de categoría tiene un dado que genera un sonido nuevo *de esa categoría*
  (parte de un preset de la categoría, lo muta y respeta sus reglas: bajos mono y sin reverb, plucks sin sustain, etc.).
- **RANDOM TOTAL:** random en todo sentido (ondas, filtro, FX, vocoder, modo de voz...).
- **6 perillas fáciles** (macros): BRILLO, MUGRE, DESAFINE, MOVIMIENTO, ESPACIO, LARGO. Se suman encima del preset,
  en el centro no cambian nada.
- **Vocoder** de 6 a 24 bandas: voz interna que "habla" sílabas en tempo (A-E-I-O-U, DES-GRA-CIA, U-A, WAH...) o
  **tu propia voz por sidechain**.
- Categorías nuevas **SYNTHS** y **VOCODER** y bajos synth nuevos.

## Presets de fábrica

| Categoría | Presets |
|---|---|
| SYNTHS | Arcade Feliz · Feria Nocturna · Pixel Brass · Marimba Turbo · Burbuja · Cassette Lead · Teclado Escolar |
| VOCODER | Robot Pobre · Desgracia Habla · Coro de Latas · Wah Robot · Vocoder Pobre II · Telefono · Tu Voz (Sidechain) |
| ICONIC LEADS | Desgracia · Nariz de Lata · Juguete Roto · Alarma Hueca · Vocoder Pobre · Mosquito · Silbido Alien · Cuadrada Barata |
| WEIRD PLUCKS | Gota de Vidrio · Pluck Pixel · Pinchazo · Bip Bip · Marimba de Plastico |
| DIGITAL KEYS | Teclado de Feria · Organito Triste · Piano Desafinado · Rhodes de Lata |
| METALLIC / FM | Campana Oxidada · Lata FM · Chapa · Engranaje |
| DARK TEXTURES | Pasillo Vacio · Fantasma Digital · Heladera |
| AMBIENT | Niebla Barata · Satelite · Respira |
| BASS | Sub Sucio · 808 Desgracia · Cuadrado Grave · Reese de Juguete · Bajo Pixel · Bajo de Feria · Acid Barato · Bajo Goma · Sub Synth 2000 · Wobble Juguete |
| EXPERIMENTAL | Error 404 · Lavarropas · Grillo · Caida Libre |

Los presets están compilados dentro del plugin (cargan siempre, sin archivos externos) y también aparecen en el
menú de programas del DAW. Los presets de usuario se guardan en `Documentos\Desgracia\Presets` (botón **GUARDAR**).

## Estructura del código

```
Source/
  Params.*                  parámetros (IDs, rangos, textos)
  PluginProcessor.*         plugin: MIDI, bloques, estado, programas del host
  engine/DSPCore.h          osciladores PolyBLEP, filtro SVF, envolventes, biquads
  engine/SynthEngine.*      voces, unison, glide, mono/legato, LFOs, modulación
  fx/FxChain.*              drive 2x oversampling, crush, EQ, chorus, delay, reverb, width
  presets/FactoryPresets.*  banco de fábrica (valores diseñados a mano)
  presets/PresetManager.*   cargar / navegar / guardar / random total / dado por categoría / init
  fx/Vocoder.*              vocoder de canales (voz interna o sidechain)
  ui/                       look & feel, editor, personaje animado
tests/RenderTest.cpp        tests offline + render de cada preset a WAV
assets/desgracia.png        el dibujo (convertido a máscara alfa)
```
