# vtheme — Vista (v4) visual-style compiler, host tool

Compiles a `.vtheme` source description plus images into a **PACKTHEM_VERSION 4**
`.msstyles`, so a Vista-format visual style can be built by the tree like any
other artefact instead of being checked in as a binary or copied onto a VM by
hand. This is `DarkFiresReactOSModules/uxtheme_new/DESIGN.md` §7.4.

```
vtheme info      <style.msstyles>          version, variant, class/part/state/image counts
vtheme list      <style.msstyles>          classes + base classes + part counts
vtheme classmap  <style.msstyles>          the raw CMAP table with indices
vtheme decompile <style.msstyles> -o DIR   -> DIR/theme.vtheme + DIR/images/
vtheme compile   <theme.vtheme>   -o OUT   -> a v4 .msstyles
vtheme import-ini <NormalNormal.INI> --bitmaps DIR -o OUT
```

## Provenance

Imported from `DwmReversingEnvironment/Vista-ThemeWork/VistaThemeTools`, which is
clean-room work informed by the v4 format notes in that tree and by the
MIT-licensed `UxThemeEx` reference. It round-trips real Vista `aero.msstyles`
losslessly — 247 classes, 599 parts, 430 images.

Two files under `libvcompile/src/` are vendored from ReactOS itself and are
**LGPL 2.1**, with their original copyright headers intact:

- `StyleMap.cpp` — from `dll/win32/uxtheme/stylemap.c`
- `schema/tmschema.h`, `schema/schemadef.h` — from `sdk/include/psdk/`

They exist because an XP-format INI names parts and states in text
(`[Button.Pushbutton(Pressed)]`) while v4 stores numeric ids, and ReactOS's own
tables are the authoritative name→id mapping.

## What was left out, and why

**The renderer.** `libvrender` — the nine-grid compositor and PNG encoder — is
the only component that needs **WIC**, and it is only used by `vtheme render`.
The CLI already guards every reference to it behind `VTHEME_HAVE_RENDER`, so
omitting it is a supported configuration rather than a fork. That leaves the
Win32 **PE resource APIs** as this tool's entire OS dependency.

Keep using the out-of-tree `VistaThemeTools` build for `render` — it is also the
verification oracle for `uxtheme_new` (DESIGN.md §9), and an oracle is more
useful when it is not the same code as the thing it checks.

## The carrier

`vcompile` does not synthesise a PE. It copies `themestub.dll` — an empty
resource-only DLL built alongside this tool — and injects the packed tables into
the copy with `UpdateResource`. `themestub.dll` therefore has to sit next to
`vtheme.exe`, which is what `add_host_module` in `CMakeLists.txt` arranges.

Because it is a *host* module, the emitted `.msstyles` carries the build
machine's architecture rather than the target's. That is fine: a visual style is
opened with `LOAD_LIBRARY_AS_DATAFILE`, which maps the image without consulting
its machine type — the same reason one `aero.msstyles` serves both 32- and
64-bit processes on real Vista.

## Known gaps

Carried over from the upstream toolkit, and worth knowing before trusting a
recompiled style:

- `DISKSTREAM`/`STREAM` payloads and animation (`AMAP`) records are parsed but
  **not reconstructed** on compile — they are reported as `skipped`.
- Per-DPI *plateau* images are parsed; only the 96-DPI base is emitted.
- Values that originally lived in a `.mui` are re-emitted **inline**, so a
  compiled style is self-contained and needs no `.mui`.
