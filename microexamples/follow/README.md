# Follow Demo

## Ejecutar
`flx run examples/scripting/follow/follow-demo.flx`

El target amarillo se desplaza horizontalmente y el objeto cyan lo sigue.

```js
const targets = find_name("target");
follow_x(object, targets[0]);
```

`find_name()` resuelve. `follow_x()` actúa sobre dos RuntimeObjects concretos. `follow` no conoce nombres, ids ni jerarquía.
