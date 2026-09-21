# Mechanics Demo

Microejemplo de FLX v0.3.0 para validar y enseñar la familia Mechanics.

## Ejecutar

```text
flx run examples/language/mechanics/mechanics-demo.flx
```

## Controles

- `W` / `UP`: thrust de la nave polar.
- `A` / `LEFT`: rotación izquierda.
- `D` / `RIGHT`: rotación derecha.
- `SPACE`: crea un probe.

## Qué demuestra

### Nave blanca — polar

Declara `speed.start = 0`, `speed.limit`, `acceleration` e `inertia`.

`input_direction()` entrega intención y `accelerate()` genera movimiento. Al soltar thrust, la velocity adquirida continúa como movimiento libre y decae según inertia. La rotación cambia orientación sin redirigir retroactivamente el momentum adquirido.

### Probe amarillo — inherit creation

Nace como hijo de la nave con:

```json
"inherit": {
  "creation": {
    "angle": "copy",
    "velocity": "compose"
  }
}
```

Tiene movimiento polar propio y compone además la velocity viva de la nave en el instante de creación.

### Barra cyan — direct

`born()` usa:

```js
apply_velocity(object, 90, 55);
```

Su vector lineal apunta a la derecha. `rotate()` cambia su orientación visual, pero su trayectoria sigue siendo horizontal porque el objeto es `direct`.

`reflect_x()` invierte la componente horizontal en los límites de la demostración.

### Cuadrado verde — apply / restore

Parte de:

```json
"speed": {
  "start": 45,
  "limit": 90
}
```

Alterna automáticamente:

```js
apply_speed(ball, 80);
restore_speed(ball);
```

`restore_speed()` vuelve siempre al `start` declarado, no al valor anterior.

## Alcance

El proyecto es deliberadamente pequeño. No pretende demostrar Physics, Navigation, Bounds o Collision. Sólo ejercita el contrato Mechanics v0.3.0 y las operaciones relacionadas ya consolidadas.
