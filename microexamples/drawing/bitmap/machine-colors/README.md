# Machine Colors

El mismo JSON se compila con tres Machines: alpha permitido, alpha prohibido y paleta limitada. El damero revela cómo blank deja de ser transparente.

Ejecutar desde la raíz del repositorio:

```sh
flx run microexamples/drawing/bitmap/machine-colors/machine-colors.flx
```

No requiere entrada ni audio. Cerrar la ventana para terminar.

Comparar también `machine-colors-opaque.flx` y `machine-colors-restricted.flx`, sin cambiar el JSON. La paleta visual nombra colores; Machine limita el resultado.

También puede compilarse y ejecutarse sin los JSON fuente:

```sh
flx compile --output=out/machine-colors.flxc microexamples/drawing/bitmap/machine-colors/machine-colors.flx
flx run-compiled out/machine-colors.flxc
```
