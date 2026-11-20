# Skeuomorphic Space Panner

Skeuomorphic Space Panner 1.1.0 è un plug-in audio di Giuseppe “Gettone” Ambrosino (gett1), scritto in C++ con JUCE. Il pad controlla panorama stereo, altezza percepita e profondità. Il pannello ADV permette di regolare le prime riflessioni della stanza virtuale.

| Piattaforma | Formati | Progetto |
| --- | --- | --- |
| Windows x64 | VST3 | [windows](windows/) |
| macOS Apple Silicon e Intel | AU, VST3, Standalone | [macos](macos/) |

## Risorse per compilare

I progetti includono immagini e texture neutre per consentire la compilazione dai sorgenti. Il font Pixelify Sans è incluso con la propria licenza SIL Open Font License. Per riprodurre la grafica della release puoi sostituire le risorse neutre con le corrispondenti immagini originali da una copia a cui hai accesso.

## Windows

Usa Visual Studio 2026 con Desktop development with C++, Windows SDK, CMake con il generatore Visual Studio 18 2026 e una copia locale dei sorgenti JUCE. Dalla cartella `windows`:

```powershell
.\Windows\Build-Windows.ps1 -JucePath "D:\JUCE" -PluginOnly
```

Per preparare anche l'installer, aggiungi il PDF finale `Manuali/Manuale_Utente.pdf`, installa Inno Setup e WinRAR, quindi esegui lo stesso comando senza `-PluginOnly`.

## macOS

Usa Xcode, CMake e Python 3. Dalla cartella `macos` esegui:

```sh
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build --config Release --parallel 2
```

Usa `x86_64` al posto di `arm64` per la build Intel. Per ricostruire il pacchetto macOS, segui i passaggi di staging e packaging nel [workflow originale](macos/.github/workflows/macos.yml). Il workflow è conservato nella sottocartella macOS come riferimento.

## Licenze

Il codice del progetto usa la [GNU Affero General Public License, versione 3](LICENSE). JUCE e Pixelify Sans sono distribuiti con i rispettivi termini.
