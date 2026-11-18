# Persist order

## Español

Una sola idea: `persist:first` se dibuja antes de la sección sustituible y
`persist:last` después, incluso sin reproducir Animation. El JSON declara rojo,
verde y azul en ese orden; el resultado debe mostrar un marco azul, un rectángulo
rojo y un centro verde. Si el orden fuera simplemente el del array, solo se vería azul.
No requiere scripts, entrada ni audio. Cerrar la ventana para terminar.

```sh
flx run microexamples/animation/persist-order/persist-order.flx
flx compile --output=persist-order.flxc microexamples/animation/persist-order/persist-order.flx
flx run-compiled persist-order.flxc
```

Este ejemplo ya funciona. No demuestra reproducción temporal ni APIs JavaScript,
que siguen pendientes. Los recursos de poses animadas están solo en tests/fixtures.

## English

One idea: `persist:first` draws before the replaceable section and `persist:last`
after it, even without Animation playback. JSON declares red, green and blue in
that order; the visible result is a blue border, red rectangle and green center.
Plain array order would show only blue. Run the commands above from the repository
root. No scripts, input or audio are required; close the window to finish.
This example is executable now; temporal playback and JavaScript APIs are not.
