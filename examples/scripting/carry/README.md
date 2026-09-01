# Carry Demo

## Ejecutar
`flx run examples/scripting/carry/carry-demo.flx`

La plataforma blanca se desplaza horizontalmente. El pasajero amarillo es un objeto hermano, no un hijo de la plataforma.

Cada hook vuelve a resolver la plataforma y llama `carry(object, platforms[0])`.

`carry()` aplica el delta del carrier de ese frame. No crea parenthood, attachment, ownership ni una relación persistente.
