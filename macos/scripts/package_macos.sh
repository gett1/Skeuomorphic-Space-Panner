#!/bin/bash
# Local integrity signatures are NOT Developer ID signatures or notarization.
set -euo pipefail

cpu="${1:?Usage: bash scripts/package_macos.sh arm64|x86_64}"
case "$cpu" in
  arm64) label=AppleSilicon-arm64 ;;
  x86_64) label=Intel-x86_64 ;;
  *) echo "Unsupported architecture: $cpu" >&2; exit 1 ;;
esac

name="Skeuomorphic Space Panner"
version=1.1.0
out="$(pwd)/dist"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
root="$work/root"
mkdir -p "$root/Library/Audio/Plug-Ins/Components" \
  "$root/Library/Audio/Plug-Ins/VST3" "$root/Applications" \
  "$root/Library/Application Support/gett1/SpacePanner"

# Sign the exact bundles distributed in the ZIP and installed by the PKG.
for bundle in "$out/AU/$name.component" "$out/VST3/$name.vst3" "$out/Standalone/$name.app"; do
  test -d "$bundle"
  executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$bundle/Contents/Info.plist")
  chmod 755 "$bundle/Contents/MacOS/$executable"
  archs=$(lipo "$bundle/Contents/MacOS/$executable" -archs)
  test "$archs" = "$cpu"
  codesign --force --sign - --timestamp=none "$bundle"
  codesign --verify --deep --strict --verbose=2 "$bundle"
done

ditto "$out/AU/$name.component" "$root/Library/Audio/Plug-Ins/Components/$name.component"
ditto "$out/VST3/$name.vst3" "$root/Library/Audio/Plug-Ins/VST3/$name.vst3"
ditto "$out/Standalone/$name.app" "$root/Applications/$name.app"
cp INSTALLAZIONE_MACOS.txt "$root/Library/Application Support/gett1/SpacePanner/"
cp Resources/OFL.txt "$root/Library/Application Support/gett1/SpacePanner/PixelifySans-OFL.txt"

# Disable relocation so an older copy elsewhere cannot redirect the installation.
pkgbuild --analyze --root "$root" "$work/components.plist"
python3 - "$work/components.plist" <<'PY'
import plistlib, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    components = plistlib.load(f)
for component in components:
    component['BundleIsRelocatable'] = False
with open(path, 'wb') as f:
    plistlib.dump(components, f)
PY
pkgbuild --root "$root" --component-plist "$work/components.plist" \
  --identifier com.gett1.skeuomorphicspacepanner.installer \
  --version "$version" --install-location / --ownership recommended \
  "$work/payload.pkg"

cat > "$work/Distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Skeuomorphic Space Panner $version — gett1</title>
  <options customize="never" require-scripts="false" hostArchitectures="$cpu"/>
  <allowed-os-versions><os-version min="12.0"/></allowed-os-versions>
  <domains enable_localSystem="true" enable_currentUserHome="false" enable_anywhere="false"/>
  <choices-outline><line choice="default"/></choices-outline>
  <choice id="default" visible="false"><pkg-ref id="com.gett1.skeuomorphicspacepanner.installer"/></choice>
  <pkg-ref id="com.gett1.skeuomorphicspacepanner.installer" version="$version" onConclusion="none">payload.pkg</pkg-ref>
</installer-gui-script>
XML

installer="$out/SkeuomorphicSpacePanner-$version-macOS-$label-Installer.pkg"
productbuild --distribution "$work/Distribution.xml" --package-path "$work" "$installer"
pkgutil --payload-files "$work/payload.pkg" > "$out/installer-payload.txt"
pkgutil --expand "$installer" "$work/expanded"
test -f "$work/expanded/Distribution"
shasum -a 256 "$installer" > "$out/installer-sha256.txt"
echo "Created installer: $installer"
echo "TEST BUILD: ad-hoc signed bundles, unsigned installer, not notarized."
