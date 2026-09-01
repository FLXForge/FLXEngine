# Structural Relationship Demo

## Ejecutar
`flx run examples/scripting/structural/structural-demo.flx`

El bloque blanco es parent estructural de dos hijos. Cada hijo resuelve su parent con `find_parent()` y el parent cuenta sus hijos vivos con `find_children()`.

A los 2 segundos mata un hijo. A los 4 segundos muere el parent y su `dead()` usa `find_children(parent)` para matar explícitamente al hijo restante.

Esto valida que el RuntimeObject entregado por Runtime a `dead()` puede servir como contexto estructural final, mientras los resultados de `find_*` siguen siendo exclusivamente vivos.

La jerarquía no implica ownership automático.
