# References Demo

## Ejecutar
`flx run examples/scripting/references/references-demo.flx`

El cuadrado amarillo se mueve horizontalmente. El marco cyan lo localiza sin conservar ningún `RuntimeObject` entre hooks.

El script raíz localiza `target` con `find_name()`, conserva únicamente su `id` como string y en hooks posteriores vuelve a resolver la instancia viva mediante `find_id()`.

> Los RuntimeObjects se resuelven, no se persisten.

El ejemplo evita `follow()` deliberadamente: el marco se recoloca con `position()` para mantener el foco en References.
