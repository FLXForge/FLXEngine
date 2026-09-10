# Modelo espacial de RuntimeObject

**Estado:** Especificación normativa\
**Ámbito:** RuntimeObject, Drawing/Shapes, Collision, Mechanics, Bounds
y sistemas consumidores de posición y tamaño\
**Versión objetivo:** FLX v0.3.0

------------------------------------------------------------------------

## 1. Propósito

Este documento define el modelo espacial común de `RuntimeObject`.

Su objetivo es fijar de forma transversal:

-   el sistema de coordenadas del mundo;
-   el significado de `origin`, `x`, `y`, `width` y `height`;
-   la referencia espacial o **pivot** de un `RuntimeObject`;
-   la relación entre el pivot y la geometría local;
-   la transformación de Shapes y Colliders;
-   el comportamiento espacial de `resize`;
-   las invariantes que otros subsistemas DEBEN respetar.

Este contrato pertenece a `RuntimeObject`.

Drawing, Shapes, Collision, Mechanics, Bounds y otros subsistemas
consumen este modelo, pero NO DEBEN redefinir el significado de las
coordenadas del objeto.

------------------------------------------------------------------------

## 2. Principio fundamental

Un `RuntimeObject` posee una referencia espacial estable denominada
**pivot**.

La posición viva del objeto, expuesta como:

``` text
x
y
```

representa la posición del pivot en el espacio del mundo.

Por tanto:

> El significado espacial de `x` e `y` pertenece a `RuntimeObject` y NO
> depende de la existencia, número o tipo de Shapes, Colliders u otras
> geometrías asociadas.

Un objeto sin representación visual, un `block`, un `circle`, un `text`
y un objeto con varios colliders comparten exactamente el mismo
significado de `x` e `y`.

------------------------------------------------------------------------

## 3. Sistema de coordenadas del mundo

FLX utiliza un sistema de coordenadas 2D con origen en la esquina
superior izquierda del espacio de juego:

``` text
(0,0) ─────────────────────────→ +X
  │
  │
  │
  │
  ↓
 +Y
```

Por tanto:

-   X aumenta hacia la derecha;
-   Y aumenta hacia abajo;
-   `(0,0)` representa el origen espacial del mundo/pantalla lógica.

Este origen del sistema de coordenadas NO debe confundirse con `origin`
de un `RuntimeObject`.

### 3.1 Origen del mundo frente a origin del objeto

Son conceptos distintos:

``` text
World origin
    (0,0)
      │
      │
      └───────────────→ RuntimeObject pivot
                              (x,y)
```

El origen del mundo establece desde dónde se miden las coordenadas
globales.

`RuntimeObject.origin` establece dónde comienza inicialmente el pivot de
una instancia concreta.

------------------------------------------------------------------------

## 4. Pivot de RuntimeObject

El pivot es la referencia espacial del objeto.

``` text
                         width
                 <----------------->

                 ┌─────────────────┐
                 │                 │
                 │        ●        │
                 │      pivot      │
                 │      (x,y)      │
                 │                 │
                 └─────────────────┘
                         height
```

En FLX v0.3.0, para las geometrías dimensionadas ordinarias, el pivot se
considera situado en el centro de su espacio geométrico local.

Esta decisión NO significa que `x/y` representen conceptualmente «el
centro del objeto».

Un `RuntimeObject` puede:

-   no tener Shape;
-   contener una geometría asimétrica;
-   tener varios Colliders;
-   tener Colliders desplazados;
-   representar únicamente estado lógico.

Por ello, la definición normativa es:

> `x/y` representan la posición mundial del pivot del RuntimeObject.

No:

> `x/y` representan el centro geométrico del RuntimeObject.

------------------------------------------------------------------------

## 5. Preparación para pivots desplazables

La posición mundial del pivot y su ubicación dentro de la geometría
local son conceptos diferentes.

FLX v0.3.0 fija la ubicación local del pivot y NO proporciona una
capacidad pública para modificarla.

Sin embargo, el modelo espacial DEBE conservar esta separación
conceptual para permitir que una versión futura pueda desplazar el pivot
dentro de la geometría del objeto sin redefinir `x/y`.

Conceptualmente existen dos cuestiones independientes:

``` text
1. ¿Dónde está el pivot en el mundo?
       → object.x / object.y

2. ¿Dónde se encuentra la geometría respecto al pivot?
       → transformación local
```

En v0.3.0:

``` text
             geometría local

          ┌─────────────────┐
          │                 │
          │        ●        │
          │      pivot      │
          │       local     │
          │                 │
          └─────────────────┘
```

Una extensión futura podría permitir:

``` text
             geometría local

          ┌─────────────────┐
          │    ● pivot      │
          │                 │
          │                 │
          │                 │
          └─────────────────┘
```

sin alterar el significado de:

``` text
object.x
object.y
origin
position
```

`x/y` continuarían representando la posición mundial del pivot.

La geometría sería la que adquiriría una transformación local diferente
respecto a ese pivot.

### 5.1 Regla de compatibilidad futura

Una futura capacidad para desplazar el pivot DEBE ser una extensión
aditiva del modelo local.

NO DEBE requerir reinterpretar:

-   `x/y`;
-   `origin`;
-   las operaciones de posición;
-   la posición mundial del RuntimeObject.

Este documento NO define el nombre, formato JSON, API ni mecanismo
mediante el que un pivot desplazable pudiera declararse o modificarse en
el futuro.

No existe tal capacidad pública en v0.3.0.

------------------------------------------------------------------------

## 6. Origin y posición viva

`origin` representa la posición inicial del pivot del RuntimeObject.

Al materializar inicialmente el objeto:

``` text
position = origin
```

Conceptualmente:

``` text
origin.x / origin.y
        │
        │ materialización
        ↓
pivot inicial
        │
        │ movimiento
        ↓
x / y vivos
```

Por tanto:

-   `origin` es estado espacial inicial;
-   `x/y` son estado espacial vivo;
-   ambos se refieren al mismo pivot;
-   mover el objeto modifica su posición viva, no redefine su pivot.

Una operación que restaure la posición al origen debe devolver el pivot
a su posición inicial.

------------------------------------------------------------------------

## 7. Tamaño vivo

`width` y `height` representan las dimensiones vivas del
`RuntimeObject`.

``` text
width
height
```

son independientes de:

``` text
x
y
```

Cambiar el tamaño NO cambia la posición del pivot.

Para una geometría dimensionada centrada en el pivot:

``` text
antes                         después

    ┌───────┐             ┌───────────────┐
    │   ●   │      →      │       ●       │
    └───────┘             └───────────────┘
       x,y                         x,y
```

El objeto crece o decrece respecto a su referencia espacial.

------------------------------------------------------------------------

## 8. Resize

Las operaciones de resize modifican las dimensiones vivas del
RuntimeObject.

Normativamente:

``` text
resize:
    puede cambiar width
    puede cambiar height
    NO cambia x
    NO cambia y
    NO cambia origin
```

Por tanto, resize NO realiza una corrección de posición para conservar
una esquina.

Esto evita que el significado de la posición dependa del tamaño.

### 8.1 Consecuencia

Si:

``` text
x = 100
y = 50
width = 20
height = 10
```

y posteriormente:

``` text
width = 40
```

el pivot continúa en:

``` text
(100,50)
```

La geometría que herede ese ancho cambia alrededor de la misma
referencia espacial.

------------------------------------------------------------------------

## 9. Espacio local del RuntimeObject

Las geometrías asociadas a un RuntimeObject se expresan respecto a su
espacio local.

En el modelo actual, un punto local se transforma conceptualmente al
mundo mediante:

``` text
worldPoint =
    object.position
    + rotate(localPoint, object.angle)
```

donde:

-   `object.position` es la posición mundial del pivot;
-   `localPoint` está expresado respecto al pivot;
-   `object.angle` transforma el espacio local del objeto.

La ubicación futura del pivot dentro de una geometría podrá modificar
cómo se obtiene `localPoint`, pero NO cambiará el significado de
`object.position`.

------------------------------------------------------------------------

## 10. Shapes

Un Shape consume el modelo espacial del RuntimeObject.

Un Shape NO define el significado de `x/y`.

### 10.1 Regla común

Todos los tipos de Shape DEBEN utilizar la misma referencia espacial.

El significado de `x/y` NO PUEDE variar según `shape.type`.

Esto implica que:

``` text
block
rectangle
triangle
circle
polygon
line
text
```

deben interpretarse respecto al mismo pivot.

### 10.2 Shapes dimensionados

En v0.3.0, las geometrías dimensionadas ordinarias se construyen
alrededor del pivot.

Conceptualmente:

``` text
local left   = -width  / 2
local right  = +width  / 2
local top    = -height / 2
local bottom = +height / 2
```

antes de aplicar las transformaciones correspondientes.

### 10.3 Block

`block` conserva su identidad como Shape rectangular simple.

La diferencia entre `block` y `rectangle` NO es el significado de `x/y`.

Ambos comparten el mismo pivot.

Las diferencias de capacidades ---por ejemplo, el tratamiento de la
rotación--- pertenecen al contrato de Shapes/Drawing y no modifican el
modelo espacial de RuntimeObject.

### 10.4 Text

La caja lógica de un Shape `text` se sitúa respecto al mismo pivot que
el resto de Shapes.

Para una caja de tamaño `width × height`, su esquina superior izquierda
se deriva de la referencia central actual:

``` text
topLeft.x = x - width  / 2
topLeft.y = y - height / 2
```

La caja de texto NO redefine `x/y` como esquina superior izquierda.

### 10.5 Polygon y geometrías asimétricas

Los puntos de un polígono son geometría local respecto al pivot.

Un polígono puede ser asimétrico y su centro geométrico puede no
coincidir con el pivot.

Esto NO cambia el significado de `x/y`.

Ejemplo:

``` text
              *
             / \
        ●---*   \
      pivot      *
```

El pivot sigue siendo la referencia espacial aunque el centro geométrico
de los puntos se encuentre en otra posición.

------------------------------------------------------------------------

## 11. Colliders

Collision consume el modelo espacial de RuntimeObject.

Collision NO redefine `x/y`.

Cada Collider posee una transformación local respecto al pivot del
RuntimeObject.

### 11.1 Collider sin offset

Un Collider cuyo `offset` sea cero o no esté declarado tiene su centro
local en el pivot.

``` text
        ┌─────────────┐
        │             │
        │      ●      │
        │ pivot       │
        │ collider    │
        └─────────────┘
```

### 11.2 Offset

`collider.offset` representa una traslación local respecto al pivot.

``` text
          pivot                collider
            ●---------------------○
                     offset
```

El offset:

-   está expresado en el espacio local del RuntimeObject;
-   utiliza unidades espaciales normales;
-   NO es una proporción del tamaño;
-   NO se escala automáticamente durante `resize`.

### 11.3 Transformación mundial del Collider

La posición mundial de su centro se obtiene conceptualmente mediante:

``` text
worldColliderCenter =
    object.position
    + rotate(collider.offset, object.angle)
```

Su orientación mundial es:

``` text
worldColliderAngle =
    object.angle
    + collider.angle
```

Por tanto, un offset local rota con el espacio local del RuntimeObject.

Ejemplo:

``` text
angle = 0°

pivot ●────────○ collider
```

tras rotar el objeto:

``` text
pivot ●
      │
      │
      ○ collider
```

El collider no conserva el offset en ejes mundiales.

------------------------------------------------------------------------

## 12. Herencia de tamaño de Collider

Cada dimensión de un Collider se resuelve independientemente.

Conceptualmente:

``` text
effectiveCollider.width =
    declaredCollider.width ?? object.width

effectiveCollider.height =
    declaredCollider.height ?? object.height
```

Por tanto, un Collider sin tamaño explícito hereda las dimensiones vivas
del RuntimeObject.

### 12.1 Relación con resize

Si un Collider hereda una dimensión:

``` text
resize object.width
        ↓
cambia effectiveCollider.width
```

No existe una sincronización adicional.

El Collider simplemente consume el estado vivo del RuntimeObject.

### 12.2 Dimensión explícita

Una dimensión explícita del Collider mantiene su independencia.

Ejemplo:

``` text
object.width = 100

collider.width declarado = 40
```

produce:

``` text
effectiveCollider.width = 40
```

aunque el objeto cambie posteriormente de ancho.

### 12.3 Herencia parcial

La herencia se aplica por eje.

Un Collider puede declarar:

``` text
width
```

y omitir:

``` text
height
```

En ese caso:

``` text
effectiveCollider.width  = declared width
effectiveCollider.height = object.height
```

------------------------------------------------------------------------

## 13. Resize y offset son conceptos independientes

`resize` NO modifica `collider.offset`.

Si:

``` text
collider.offset = (20, 0)
```

significa:

> el centro local del collider está veinte unidades a la derecha del
> pivot.

No significa:

> el collider está situado a una determinada proporción del ancho del
> objeto.

Por tanto:

``` text
resize(object)
```

NO debe escalar ni recalcular ese offset.

Una futura capacidad para expresar anclajes relativos, alineación o
constraints deberá disponer de un contrato explícito propio.

No debe introducirse implícitamente reinterpretando `offset`.

------------------------------------------------------------------------

## 14. Posicionamiento por bordes

Dado que `x/y` representan el pivot, los bordes de una geometría
rectangular centrada se derivan del tamaño.

Sin rotación:

``` text
left   = x - width  / 2
right  = x + width  / 2
top    = y - height / 2
bottom = y + height / 2
```

Los subsistemas o scripts que necesiten razonar sobre bordes NO deben
reinterpretar `x/y` como una esquina.

------------------------------------------------------------------------

## 15. Helpers espaciales

Cualquier helper cuya intención sea comparar, seguir o copiar la
posición espacial de RuntimeObjects DEBE operar sobre sus pivots.

No debe reconstruir un supuesto centro mediante:

``` text
position + size / 2
```

porque `position` ya representa la referencia espacial.

Las dimensiones sólo deben intervenir cuando la operación requiera
explícitamente:

-   bordes;
-   extensión geométrica;
-   separación;
-   área;
-   bounds;
-   otra magnitud dependiente del tamaño.

------------------------------------------------------------------------

## 16. Creation y Grid

Los sistemas de creación posicionan referencias espaciales de
RuntimeObjects.

Una creación en grid puede determinar el origen de un child mediante:

``` text
parent.position
+ cell position
+ child creation offset
```

El resultado establece la posición inicial del pivot del child.

Creation NO debe depender del tipo de Shape del objeto creado.

El tamaño de una celda y el tamaño de un Shape son conceptos diferentes.

------------------------------------------------------------------------

## 17. Bounds

Bounds consume la posición espacial del RuntimeObject.

El significado de `x/y` continúa siendo el pivot.

Cuando una regla de bounds necesite conocer la extensión geométrica del
objeto deberá derivarla expresamente del tamaño o de la geometría
efectiva correspondiente.

Bounds NO debe reinterpretar `position` como esquina superior izquierda.

------------------------------------------------------------------------

## 18. RuntimeObject JavaScript

La interfaz JavaScript observa el mismo modelo espacial.

Conceptualmente:

``` text
object.x
object.y
```

exponen la posición mundial viva del pivot.

``` text
object.width
object.height
```

exponen las dimensiones vivas.

El hecho de que el objeto posea un Shape determinado NO modifica el
significado de estas propiedades.

Las operaciones de posición modifican la posición del pivot.

Las operaciones de resize modifican dimensiones y no desplazan el pivot.

------------------------------------------------------------------------

## 19. Separación respecto a Attach

El espacio local de Colliders definido en este documento NO modifica por
sí mismo el contrato de Attach.

En particular, una regla existente de Attach puede definir su propio
tratamiento del offset y del ángulo.

La existencia de:

``` text
worldColliderCenter =
    object.position + rotate(collider.offset, object.angle)
```

NO implica automáticamente que cualquier otro `offset` del sistema deba
rotar de la misma manera.

Cada subsistema debe declarar la naturaleza de sus offsets.

Una futura revisión de Attach podrá adoptar transformaciones jerárquicas
diferentes si se decide expresamente, pero no forma parte de este
contrato.

------------------------------------------------------------------------

## 20. Separación respecto a Drawing

Drawing y Shapes consumen el pivot y el espacio local definidos aquí.

Drawing puede definir:

-   cómo se rasteriza cada Shape;
-   qué Shapes admiten rotación;
-   modos fill/outline;
-   propiedades específicas de geometría;
-   comportamiento visual.

Pero Drawing NO puede cambiar el significado de `RuntimeObject.x/y`.

------------------------------------------------------------------------

## 21. Separación respecto a Collision

Collision consume:

-   posición del pivot;
-   tamaño vivo;
-   ángulo vivo;
-   transformación local del Collider.

Collision define:

-   geometrías de colisión;
-   interacción;
-   contactos;
-   broad phase;
-   narrow phase;
-   ray queries.

Pero Collision NO define qué significa la posición del RuntimeObject.

------------------------------------------------------------------------

## 22. Futuras cámaras, scroll y espacios de render

Una futura Camera, Scroll, transformación de viewport o sistema de capas
puede transformar la representación del mundo en pantalla.

Estas capacidades NO deben redefinir la posición mundial del
RuntimeObject.

Conceptualmente:

``` text
RuntimeObject local geometry
        ↓
RuntimeObject world transform
        ↓
world space
        ↓
future camera / scroll / viewport transform
        ↓
screen
```

La posición `x/y` continúa perteneciendo al espacio del mundo salvo que
una futura especificación introduzca explícitamente espacios
adicionales.

------------------------------------------------------------------------

## 23. Invariantes normativas

Las siguientes reglas son invariantes del modelo espacial de FLX:

1.  `x/y` representan la posición mundial viva del pivot del
    RuntimeObject.
2.  `origin` representa la posición inicial de ese mismo pivot.
3.  El origen `(0,0)` del mundo y `RuntimeObject.origin` son conceptos
    diferentes.
4.  El significado de `x/y` NO depende de `shape.type`.
5.  El significado de `x/y` NO depende de la existencia de un Collider.
6.  Un RuntimeObject puede existir sin geometría y conservar exactamente
    el mismo modelo espacial.
7.  `width/height` son dimensiones vivas independientes de la posición.
8.  `resize` NO desplaza el pivot.
9.  En v0.3.0, las geometrías dimensionadas ordinarias se construyen
    alrededor del pivot.
10. Un Collider sin offset tiene su centro local en el pivot.
11. Un Collider con offset expresa una traslación local respecto al
    pivot.
12. El offset local de un Collider rota con `object.angle`.
13. `collider.angle` se compone con `object.angle`.
14. Un Collider hereda por eje las dimensiones vivas no declaradas.
15. Una dimensión explícita del Collider es independiente del resize de
    ese eje.
16. `resize` NO escala `collider.offset`.
17. Los helpers de posición operan sobre pivots y no reconstruyen
    centros mediante `size/2`.
18. Drawing, Collision, Mechanics, Bounds y Creation consumen este
    modelo y NO lo redefinen.
19. La ubicación local del pivot dentro de la geometría está fijada en
    v0.3.0, pero es conceptualmente independiente de su posición
    mundial.
20. Una futura capacidad de desplazar el pivot dentro de la geometría
    DEBE conservar el significado actual de `x/y` y `origin`.

------------------------------------------------------------------------

## 24. Ejemplo integrado

Objeto:

``` json
{
  "origin": {
    "x": 100,
    "y": 80
  },
  "shape": {
    "type": "block",
    "size": {
      "width": 40,
      "height": 20
    }
  },
  "collisions": {
    "body": {
      "type": "box"
    }
  }
}
```

Estado inicial:

``` text
pivot = (100,80)
width = 40
height = 20
```

Representación conceptual:

``` text
          x=100
            │
      ┌───────────────┐
      │               │
      │       ●       │ y=80
      │               │
      └───────────────┘

       width = 40
       height = 20
```

El Shape y el Collider heredado comparten la misma referencia.

Tras:

``` text
resize_width(object, 80)
```

el resultado conceptual es:

``` text
              pivot sigue en (100,80)
                       │
      ┌────────────────●────────────────┐
      │                                 │
      └─────────────────────────────────┘

               width = 80
```

El Collider heredado adquiere también ancho efectivo 80.

No se modifica:

``` text
x
y
origin
collider.offset
```

------------------------------------------------------------------------

## 25. Ejemplo de Collider desplazado

Supóngase:

``` text
object.position = (100,80)
object.angle = 90°
collider.offset = (20,0)
```

Antes de aplicar la rotación, el offset significa veinte unidades sobre
el eje X local.

La transformación efectiva es:

``` text
worldColliderCenter =
    object.position
    + rotate((20,0), 90°)
```

Por tanto, el Collider gira alrededor del pivot junto con el espacio
local del RuntimeObject.

El offset no se interpreta como una coordenada mundial.

------------------------------------------------------------------------

## 26. Evolución futura del pivot

El modelo se diseña deliberadamente para admitir una evolución
posterior.

Hoy:

``` text
              geometry
        ┌─────────────────┐
        │                 │
        │        ●        │
        │      pivot      │
        │                 │
        └─────────────────┘
```

En una versión futura podría permitirse:

``` text
              geometry
        ┌─────────────────┐
        │ ● pivot         │
        │                 │
        │                 │
        │                 │
        └─────────────────┘
```

sin cambiar:

``` text
x/y = posición mundial del pivot
```

El cambio afectaría a la transformación local de la geometría respecto
al pivot.

Esta separación evita que incorporar pivots desplazables obligue
posteriormente a redefinir:

-   movimiento;
-   posición;
-   Collision;
-   RuntimeObject JS;
-   `origin`;
-   coordenadas del mundo.

La futura capacidad deberá especificar de forma independiente cómo se
expresa la ubicación local del pivot y cómo interactúa con resize,
Shapes y otras geometrías.

Hasta que exista esa especificación, el pivot local permanece fijado
según las reglas de v0.3.0.

------------------------------------------------------------------------

## 27. Regla de diseño

Toda nueva capacidad espacial de FLX debe responder separadamente a
estas preguntas:

``` text
¿Dónde está el RuntimeObject en el mundo?
        → posición mundial de su pivot

¿Dónde está la geometría respecto al RuntimeObject?
        → transformación local respecto al pivot

¿Cómo se transforma el mundo para representarlo?
        → transformación de cámara/scroll/viewport, si existe
```

Estas tres cuestiones NO deben mezclarse.

Mantener esta separación es la base para que futuras capacidades como
pivots desplazables, nuevas Shapes, cámaras, scroll o transformaciones
adicionales puedan incorporarse sin alterar el contrato fundamental de
`RuntimeObject`.
