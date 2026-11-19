# Loop current

## Español

Cada ciclo suma 40 unidades visuales; la posición física permanece fija. Tras varios ciclos sale del encuadre.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/loop-current/loop-current.flx
flx compile --output=loop-current.flxc microexamples/animation/loop-current/loop-current.flx
flx run-compiled loop-current.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

Each cycle adds 40 visual units; physical position stays fixed. It eventually leaves the viewport.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
