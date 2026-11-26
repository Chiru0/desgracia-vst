# Instalación (Windows 10/11, 64-bit)

## Opción A: instalador
1. Bajate `Desgracia-Setup-Windows-x64.exe` del release **latest** del repo.
2. Ejecutalo (pide permisos de administrador). Copia `Desgracia.vst3` a `C:\Program Files\Common Files\VST3\`.
3. Windows SmartScreen puede avisar porque el instalador no está firmado: "Más información" → "Ejecutar de todas formas".

## Opción B: zip
1. Bajate `Desgracia-Windows-x64.zip` y descomprimilo.
2. Doble click en `INSTALAR.bat` (pide administrador y copia el plugin a la carpeta VST3),
   o copiá a mano la carpeta `Desgracia.vst3` entera a `C:\Program Files\Common Files\VST3\`.

## FL Studio
1. `Options` → `Manage plugins` → asegurate de que `C:\Program Files\Common Files\VST3` esté en las rutas.
2. `Find more plugins` (o `Find installed plugins`). Aparece **DESGRACIA** (fabricante Ciro), marcalo con la estrella.
3. En el Channel Rack: `+` → DESGRACIA. Los presets están en el navegador de la interfaz y también en el menú
   de presets de FL (flechitas del wrapper), porque el plugin los expone como "programas".

### Vocoder con tu voz en FL Studio
1. En el plugin: página **VOCODER**, prendé **VOCODER ON** y poné **QUIEN HABLA = SIDECHAIN** (o cargá el preset
   *Tu Voz (Sidechain)*).
2. En el Mixer, seleccioná el canal donde está tu voz y, con el canal de Desgracia elegido, hacé click derecho en la
   flechita de ruteo → **Sidechain to this track**.
3. En el wrapper del plugin (engranaje → *Processing*), en **Input** asigná ese sidechain a la entrada
   *Sidechain (vocoder)*.
4. Tocá acordes mientras suena la voz.

## Otros DAWs
Ableton Live 11/12, Reaper, Bitwig, Studio One, Cubase: activá la carpeta VST3 del sistema y re-escaneá.

## Standalone
`Desgracia.exe` (en el zip) abre el sinte sin DAW. Configurá audio y MIDI en el menú de opciones de la ventana.

## Presets de usuario
- **GUARDAR** guarda en `Documentos\Desgracia\Presets\<nombre>.desgracia` y aparece en la categoría USER.
- Podés compartir presets copiando esos archivos. En el menú del nombre del preset: "rescan user presets"
  y "open user preset folder".
- La carpeta `Presets` del zip tiene el banco de fábrica exportado como archivos (backup / para compartir);
  no hace falta instalarla, el banco ya viene adentro del plugin.

## Desinstalar
Borrá `C:\Program Files\Common Files\VST3\Desgracia.vst3` (o usá "Agregar o quitar programas" si usaste el instalador).
