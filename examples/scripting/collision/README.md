# Collision — microejemplos ejecutables

Estos ejemplos complementan `docs/spec/scripting/collision.md`.

A diferencia de los fragmentos históricos de `examples/microexamples/`, cada
carpeta contiene un proyecto FLX completo y ejecutable, incluido su archivo
`.flx`.

## Ejemplos

- `01-directed-passive`: collider fuente dirigido contra un collider target pasivo.
  También muestra una respuesta manual con `normal` y `penetration`.
- `02-multicollider`: un RuntimeObject fuente posee varios colliders y usa
  `contact.collider` para distinguir cuál produjo el contacto.
- `03-state-collider`: un collider dirigido sólo existe de forma efectiva en un
  State concreto.
- `04-ray`: `ray()` consulta la misma geometría efectiva sin usar `with` ni
  disparar `collision()`.

Los ejemplos están deliberadamente separados: cada uno intenta demostrar una
sola idea de Collision con el mínimo ruido posible.

## Debug

Son especialmente útiles con el debug de Collision activado, porque permite ver
los `EffectiveCollider`, puntos/normales de contactos y Rays ejecutados.

## Principio común

Collision detecta y describe; el script decide la reacción. Ningún ejemplo
depende de resolución física automática.
