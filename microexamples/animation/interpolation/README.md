# Interpolación / Interpolation

## Español

El cuadrado se desplaza y gira durante dos segundos; DONE conserva la pose.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/interpolation/interpolation.flx
flx compile --output=interpolation.flxc microexamples/animation/interpolation/interpolation.flx
flx run-compiled interpolation.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

The square moves and rotates for two seconds; DONE retains the pose.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
