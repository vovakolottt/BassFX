@echo off
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
echo.
echo Plugin: build\BassFX_artefacts\Release\VST3\BassFX.vst3
pause
