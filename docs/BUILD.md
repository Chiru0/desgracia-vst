# Compilar

Requisitos: CMake 3.22+, un compilador C++17 y Git. JUCE 8.0.4 se descarga solo (o usa `./JUCE` si existe).

## Windows (Visual Studio 2022)
```bat
cmake -B build -A x64
cmake --build build --config Release --target Desgracia_VST3 Desgracia_Standalone
```
Resultado: `build\Desgracia_artefacts\Release\VST3\Desgracia.vst3` y `...\Standalone\Desgracia.exe`.

## GitHub Actions
`.github/workflows/build.yml` hace lo mismo en `windows-latest` en cada push, corre los tests en Windows,
valida con pluginval, arma zip + instalador Inno Setup y publica el release `latest` (solo desde `main`).

## Tests (cualquier plataforma)
```bash
cmake -B build -DDESGRACIA_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target DesgraciaTest
./build/DesgraciaTest_artefacts/Release/DesgraciaTest renders/            # tests + un WAV por preset
./build/DesgraciaTest_artefacts/Release/DesgraciaTest out/ --export-presets  # banco como archivos .desgracia
xvfb-run ./build/DesgraciaTest_artefacts/Release/DesgraciaTest shots/ --snapshot   # capturas de la UI (Linux)
```
El test usa el mismo `DesgraciaProcessor` que el plugin y verifica: los 35 presets suenan y no pasan 0 dBFS,
no hay NaN, no quedan notas colgadas, robo de voces (16), pedal de sustain, all-notes-off, velocity 0,
mono/legato, 44.1/48/88.2/96 kHz con buffers de 1 a 4096, guardar/restaurar estado, presets de usuario,
50 random y el peor caso de CPU.

En Linux hacen falta: `libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev`.
