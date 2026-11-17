# Mixed Representations

Tres emblemas combinan primitiva, geometría, bitmap y texto en ese orden. Comparten pivot y rotación; el color base magenta no tiñe el bitmap.

Ejecutar desde la raíz del repositorio:

```sh
flx run microexamples/drawing/bitmap/mixed-representations/mixed-representations.flx
```

No requiere entrada ni audio. Cerrar la ventana para terminar.

También puede compilarse y ejecutarse sin los JSON fuente:

```sh
flx compile --output=out/mixed-representations.flxc microexamples/drawing/bitmap/mixed-representations/mixed-representations.flx
flx run-compiled out/mixed-representations.flxc
```
