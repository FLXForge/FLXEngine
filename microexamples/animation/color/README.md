# Color heredado y explícito / Inherited and explicit color

## Español

El rectángulo exterior interpola rojo/azul. El centro blanco tiene color explícito y permanece blanco.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/color/color.flx
flx compile --output=color.flxc microexamples/animation/color/color.flx
flx run-compiled color.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

The outer rectangle interpolates red/blue. The explicitly white center stays white.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
