# Microejemplos Collision v0.3

Estos ejemplos no pretenden ser juegos completos. Su objetivo es validar el contrato de Collision con el menor ruido posible.

- `01_directed_passive.json`: fuente dirigida contra target pasivo.
- `02_multicollider.json`: dos superficies con una sola identidad RuntimeObject.
- `03_state_collider.json`: collider disponible sólo en un State.
- `04_component_collision.json`: componentes siguen siendo RuntimeObjects distintos para Collision.
- `05_ray.js`: consulta Ray y lectura de RayHit.
- `06_contact.js`: inspección de `contacts[]` sin depender de su orden.
