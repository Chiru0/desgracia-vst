@echo off
rem Copia Desgracia.vst3 a la carpeta VST3 estandar de Windows (pide permisos de administrador)
net session >nul 2>&1
if %errorLevel% neq 0 (
  echo Pidiendo permisos de administrador...
  powershell -Command "Start-Process '%~f0' -Verb RunAs"
  exit /b
)
set DEST=%CommonProgramFiles%\VST3
if not exist "%DEST%" mkdir "%DEST%"
xcopy /E /I /Y "%~dp0Desgracia.vst3" "%DEST%\Desgracia.vst3"
echo.
echo Listo. DESGRACIA quedo instalado en %DEST%\Desgracia.vst3
echo Abri FL Studio ^> Options ^> Manage plugins ^> Find more plugins.
pause
