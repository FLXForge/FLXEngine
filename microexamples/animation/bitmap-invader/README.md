# Bitmap invader · cinco fotogramas / five frames

## Español

Una idea: Animation sustituye discretamente cinco bitmaps de 11 × 8 píxeles,
creando un pequeño invasor de estilo arcade. Las antenas, brazos y patas cambian
cada 0.25 segundos; `loop: true` vuelve al primer fotograma al llegar a 1.25 segundos.
No hay interpolación de píxeles ni desplazamiento físico del objeto.

La paleta local usa `G` para verde y `.` para `blank` (transparente). El objeto
mide 132 × 96 unidades: cada celda ocupa 12 × 12 unidades, con filtrado puntual.
`born` inicia `march` explícitamente; no hay autoplay. No necesita imágenes
externas, entrada ni audio. Cierra la ventana para terminar.

Desde la raíz del repositorio:

```sh
flx run microexamples/animation/bitmap-invader/bitmap-invader.flx
flx compile --output=bitmap-invader.flxc microexamples/animation/bitmap-invader/bitmap-invader.flx
flx run-compiled bitmap-invader.flxc
```

## English

One idea: Animation discretely replaces five 11 × 8 pixel bitmaps to create a
small arcade-style invader. Antennae, arms and legs change every 0.25 seconds;
`loop: true` returns to the first frame at 1.25 seconds. Pixels are not interpolated
and the object's physical position stays fixed.

The local palette maps `G` to green and `.` to `blank` (transparent). The object
is 132 × 96 logical units: each cell occupies 12 × 12 units with point filtering.
`born` explicitly starts `march`; there is no autoplay. No external images,
input or audio are needed. Close the window to finish.

Run the same source/compile/binary commands above from the repository root.
