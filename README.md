# BassFX (VST3) — Distortion + Wide Reverb для баса

## Сборка на Windows
1. Установи: Visual Studio 2022 Community (галочка "Разработка классических приложений на C++"), CMake, Git.
2. Двойной клик по `build.bat` (или в терминале из папки проекта):
       cmake -B build -G "Visual Studio 17 2022" -A x64
       cmake --build build --config Release
   Первая сборка долгая (скачивается JUCE).
3. Готовый плагин:  build\BassFX_artefacts\Release\VST3\BassFX.vst3
4. Скопируй папку BassFX.vst3 в  C:\Program Files\Common Files\VST3\
5. FL Studio: Options -> Manage plugins -> Find installed plugins (Start scan). Плагин появится как BassFX.
