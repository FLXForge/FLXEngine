# Palette Composition

Tres tótems reutilizan una paleta externa. El segundo selecciona un miembro y el tercero aplica un override local; el primero permanece intacto.

Ejecutar desde la raíz del repositorio:

```sh
flx run microexamples/drawing/bitmap/palette-composition/palette-composition.flx
```

No requiere entrada ni audio. Cerrar la ventana para terminar.

También puede compilarse y ejecutarse sin los JSON fuente:

```sh
flx compile --output=out/palette-composition.flxc microexamples/drawing/bitmap/palette-composition/palette-composition.flx
flx run-compiled out/palette-composition.flxc
```
