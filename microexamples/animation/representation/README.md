# Representación y persistencia / Representation and persistence

## Español

El centro cambia de rectángulo a elipse, desaparece y muestra DONE. El marco azul y el punto verde persistentes permanecen.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/representation/representation.flx
flx compile --output=representation.flxc microexamples/animation/representation/representation.flx
flx run-compiled representation.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

The center changes from rectangle to ellipse, disappears and displays DONE. The persistent blue frame and green dot remain.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
