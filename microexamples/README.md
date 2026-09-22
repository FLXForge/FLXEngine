# FLX Engine --- Microexamples

`microexamples/` contiene demostraciones ejecutables, pequeñas y
autocontenidas de contratos y capacidades concretas de FLX Engine.

No son juegos completos ni una segunda especificación. La especificación
normativa vive en `docs/spec/`; los microejemplos sirven como
contraparte ejecutable para observar algunos de esos contratos de forma
aislada.

Los juegos completos e históricamente representativos viven en
`examples/`.

## Criterio

Un microejemplo intenta responder a una pregunta sencilla:

> Quiero entender X. ¿Cuál es la mínima declaración ejecutable que me
> permite verlo?

Por ello:

-   cada microejemplo debe poder ejecutarse directamente mediante su
    propio `.flx`;
-   sólo incluye Machine o Input propios cuando el concepto lo necesita;
-   evita infraestructura o capacidades ajenas a aquello que pretende
    mostrar;
-   no intenta reproducir exhaustivamente todos los invariantes de una
    especificación;
-   un mismo contrato puede necesitar varios microejemplos cuando las
    diferencias sean conceptualmente relevantes.

La organización del directorio es conceptual. El orden pedagógico
pertenece a este documento, no a prefijos numéricos en los nombres de
los directorios.

## Recorrido recomendado

### 1. Drawing

#### `drawing/basic`

Introducción al dibujo inmediato desde JavaScript.

Muestra las cuatro primitivas básicas de Drawing y la diferencia entre
coordenadas World y coordenadas Local relativas a un `RuntimeObject`.

#### `drawing/visual-representation`

Representación visual declarativa mediante `visual.representation`.

Muestra `primitive`, `geometry`, `text`, composición, color y `depth`
sin utilizar Drawing procedural. Sirve como contrapunto declarativo a
`drawing/basic`.

#### `drawing/showcases`

Pequeñas demostraciones construidas con Drawing que no pretenden aislar
un único contrato:

-   **CRT** --- efecto visual basado en líneas y píxeles inmediatos.
-   **Rain** --- composición procedural mediante Drawing.

### 2. Mechanics

#### `mechanics`

Demostración integrada de Mechanics.

Incluye movimiento `direct` y `polar`, velocidad, aceleración, inercia,
rotación, reflexión, velocidad viva y herencia de movimiento.

#### `mechanics/carry`

Demuestra `carry(object, carrier)`: aplicación del desplazamiento del
carrier durante el frame sin crear una relación persistente entre ambos
objetos.

Collision se utiliza como contexto para decidir cuándo aplicar el
transporte, pero no realiza el carry automáticamente.

### 3. Input

#### `input/digital`

Demuestra lectura de entrada digital y sus estados `pressed`, `down` y
`released`.

#### `input/machine-mapping`

Ejecuta el mismo World y el mismo comportamiento con Machines diferentes
para mostrar que la semántica del juego permanece independiente de una
dirección física `2way` o `4way`.

### 4. Collision

#### `collision/directed-passive`

Demuestra la relación entre collider dirigido y collider pasivo.

#### `collision/multicollider`

Demuestra varios colliders pertenecientes a un mismo `RuntimeObject` y
la identificación del collider implicado en el contacto.

#### `collision/state-collider`

Demuestra un collider cuya participación depende del State efectivo.

#### `collision/ray`

Demuestra una consulta explícita mediante `ray()` y la observación del
impacto sobre geometría Collision efectiva.

### 5. States

#### `states`

Demuestra transición entre States y observación de `state_current`,
`state_entered` y `state_time`.

### 6. Timer

#### `timer`

Demuestra el ciclo de un timer Runtime: creación/redefinición, pausa,
reanudación, finalización natural, repetición y eliminación.

### 7. References

#### `references/resolve`

Demuestra la diferencia entre una referencia temporal `RuntimeObject` y
la identidad persistible mediante `id`, resolviendo posteriormente la
instancia viva con `find_id()`.

#### `references/structural`

Demuestra navegación mediante `find_parent()` y `find_children()` y,
especialmente, que la jerarquía estructural normal no implica ownership
de lifecycle.

### 8. Component

#### `component/lifecycle`

Demuestra pertenencia lógica y cascada de lifetime mediante
`component:true`.

Una cadena de components muere junto con su entidad, mientras que un
child normal corta la cadena Component y conserva un lifecycle
independiente.

### 9. Follow

#### `follow`

Demuestra seguimiento sobre un eje mediante `follow_x()`.

`follow_*` continúa siendo una capacidad Runtime existente. Su posible
ampliación o encaje futuro dentro de Navigation/References no cambia el
comportamiento demostrado aquí.

## Ejecución

Cada microejemplo ejecutable contiene su propio archivo `.flx`.

Desde una instalación o build de FLX capaz de ejecutar proyectos `.flx`,
se debe lanzar el `.flx` correspondiente al ejemplo que se quiera
observar.

Algunos directorios contienen más de una variante ejecutable. Por
ejemplo, `input/machine-mapping` incluye proyectos separados para las
Machines `2way` y `4way`; ambos reutilizan el mismo comportamiento para
hacer observable la diferencia de Machine sin modificar el juego.

## Relación con la especificación

La relación entre documentación y microejemplos es deliberadamente
cercana:

``` text
docs/spec/        <---------->        microexamples/
contrato escrito                      contrato ejecutable
```

No existe, sin embargo, una correspondencia obligatoria uno-a-uno.

Una especificación puede estar cubierta por varios microejemplos, un
microejemplo puede atravesar más de un contrato y algunos contratos
pueden quedar suficientemente protegidos por tests y juegos completos
sin necesitar un microejemplo propio.

Cuando exista discrepancia, `docs/spec/` define el contrato normativo.
Un microejemplo que contradiga la especificación debe considerarse
candidato a revisión, no una fuente alternativa de semántica.

## Microexamples y Examples

``` text
microexamples/
    capacidad aislada
    pequeña
    directamente observable
    orientada a comprender un contrato

examples/
    juego completo
    combinación real de capacidades
    valor histórico o representativo
```

Los microejemplos explican piezas. Los ejemplos muestran qué ocurre
cuando esas piezas forman un juego.
