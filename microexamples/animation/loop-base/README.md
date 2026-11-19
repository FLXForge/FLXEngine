# Loop base

## Español

El cuadrado vuelve al inicio cada dos segundos, sin acumular desplazamiento.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/loop-base/loop-base.flx
flx compile --output=loop-base.flxc microexamples/animation/loop-base/loop-base.flx
flx run-compiled loop-base.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

The square returns to its start every two seconds, without accumulating displacement.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
