# DESGRACIA

Sintetizador VST3 (Windows 64-bit) de sonido digital sucio, inspirado en el under argentino / trap experimental.
Motor de síntesis propio (sin samples), 35 presets de fábrica, efectos integrados y el personaje **Desgracia**
animado en la interfaz: canta con el audio, rebota con cada nota, parpadea, se duerme si no tocás nada,
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

## Presets de fábrica

| Categoría | Presets |
|---|---|
| ICONIC LEADS | Desgracia · Nariz de Lata · Juguete Roto · Alarma Hueca · Vocoder Pobre · Mosquito · Silbido Alien · Cuadrada Barata |
| WEIRD PLUCKS | Gota de Vidrio · Pluck Pixel · Pinchazo · Bip Bip · Marimba de Plastico |
| DIGITAL KEYS | Teclado de Feria · Organito Triste · Piano Desafinado · Rhodes de Lata |
| METALLIC / FM | Campana Oxidada · Lata FM · Chapa · Engranaje |
| DARK TEXTURES | Pasillo Vacio · Fantasma Digital · Heladera |
| AMBIENT | Niebla Barata · Satelite · Respira |
| BASS | Sub Sucio · 808 Desgracia · Cuadrado Grave · Reese de Juguete |
| EXPERIMENTAL | Error 404 · Lavarropas · Grillo · Caida Libre |

Los presets están compilados dentro del plugin (cargan siempre, sin archivos externos) y también aparecen en el
menú de programas del DAW. Los presets de usuario se guardan en `Documentos\Desgracia\Presets` (botón **SAVE**).

## Estructura del código

```
Source/
  Params.*                  parámetros (IDs, rangos, textos)
  PluginProcessor.*         plugin: MIDI, bloques, estado, programas del host
  engine/DSPCore.h          osciladores PolyBLEP, filtro SVF, envolventes, biquads
  engine/SynthEngine.*      voces, unison, glide, mono/legato, LFOs, modulación
  fx/FxChain.*              drive 2x oversampling, crush, EQ, chorus, delay, reverb, width
  presets/FactoryPresets.*  banco de fábrica (valores diseñados a mano)
  presets/PresetManager.*   cargar / navegar / guardar / random / init
  ui/                       look & feel, editor, personaje animado
tests/RenderTest.cpp        tests offline + render de cada preset a WAV
assets/desgracia.png        el dibujo (convertido a máscara alfa)
```
