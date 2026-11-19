# Pausa y reanudación / Pause and resume

## Español

Se pausa al segundo, espera un segundo y reanuda sin reiniciar. El color verde señala la reanudación.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/pause-resume/pause-resume.flx
flx compile --output=pause-resume.flxc microexamples/animation/pause-resume/pause-resume.flx
flx run-compiled pause-resume.flxc
```

Animation afecta únicamente a la apariencia. `born` inicia la reproducción explícitamente; no hay autoplay.

## English

It pauses at one second, waits one second and resumes without restarting. Green signals resumption.

Run the same source/compile/binary commands above from the repository root.
Animation affects appearance only. `born` explicitly starts playback; there is no autoplay.
