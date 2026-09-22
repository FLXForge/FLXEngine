# Collision — cierre v0.3.0: diagramas de `CollisionContact`

> **Destino de integración:** `docs/spec/scripting/collision.md`
>
> Este archivo es un fragmento aditivo de cierre. No sustituye la spec existente.
> Se recomienda insertarlo después de **§20 Penetración** y antes de
> **§21 Contacto por pareja de colliders**.

## 20.1 Lectura visual de `CollisionContact`

`CollisionContact` describe el contacto desde la perspectiva de `object`.

### Normal

La normal apunta en una dirección razonable en la que `object` puede desplazarse
para salir de `other`.

```text
                    normal de object
                         ↑ (0,-1)
                         │
                    ┌────●────┐
                    │ object  │
                    └─────────┘
                ────────●────────  ← superficie de other
                        point

                         ↓
                  penetration
             (solape sobre la normal)
```

En este ejemplo, una separación razonable es:

```js
position(
    object,
    object.x + contact.normalX * contact.penetration,
    object.y + contact.normalY * contact.penetration
);
```

Collision **no aplica** esta separación automáticamente.

### Punto representativo

`pointX / pointY` no constituyen un manifold físico completo. Representan un
punto estable y perceptualmente natural de la región real de contacto.

Cuando una caja pequeña solapa una superficie larga:

```text
other
┌──────────────────────────────────────────────┐
│                                              │
└───────────────┬──────────────┬───────────────┘
                │   solape     │
                └──────●───────┘
                       point
                    ┌────────┐
                    │ object │
                    └────────┘
```

El punto debe seguir la **región de contacto**, no una esquina arbitraria de la
geometría grande.

Cuando la intersección forma una región en vez de un único punto:

```text
             object
          ┌────────────┐
          │       ┌────┼────────┐
          │       │////│        │
          └───────┼─●──┘        │
                  │////│  other │
                  └────┴────────┘
                       ↑
              punto representativo
              de la región común
```

FLX puede representar esa región mediante un único punto central razonable.

### Penetración

`penetration` expresa cuánto debe recorrer aproximadamente `object` sobre la
normal publicada para abandonar el solape.

```text
             object
          ┌──────────┐
          │          │
──────────┼──────────┼────────── superficie de other
          │<-- p --> │
          └──────────┘

normal ↑

separación conceptual:
object.position += normal * penetration
```

La penetración se interpreta junto con la normal; no es una distancia genérica
entre centros.

### Perspectiva inversa

Si la misma geometría se evalúa en sentido inverso sin mutaciones intermedias:

```text
A → B                         B → A

      normal ↑                    normal ↓
        ┌───┐                      ┌───┐
        │ A │                      │ A │
        └─●─┘                      └─●─┘
──────────●──────── B      ──────────●──────── B
        point                       point
```

Deben conservarse:

```text
point
penetration
```

y deben invertirse/intercambiarse:

```text
normal
collider ↔ otherCollider
```

### Multicollider

Una única interacción `RuntimeObject → RuntimeObject` puede contener contactos
procedentes de varias parejas de colliders.

```text
source RuntimeObject A
┌────────────────────────────┐
│                            │
│   [body]      [attack]     │
│      \            \        │
└───────\────────────\───────┘
         \            \
          ● c1         ● c2
       [shield]      [body]
┌────────────────────────────┐
│ target RuntimeObject B     │
└────────────────────────────┘
```

La callback sigue siendo una sola:

```js
function collision(object, other, contacts) {
    for (const contact of contacts) {
        // contact.collider      -> collider de object
        // contact.otherCollider -> collider de other
    }
}
```

No existe una callback por collider. Cada pareja concreta de colliders produce
como máximo un `CollisionContact`, y los contactos válidos se agregan para la
interacción dirigida entre ambos RuntimeObjects.

## Nota de cierre

Estos diagramas son explicativos; no añaden campos ni cambian el contrato
normativo de Collision v0.3.0.
