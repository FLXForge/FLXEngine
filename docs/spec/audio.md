# Audio

## Estado
- Documento: especificación normativa
- Ámbito: audio declarado, voces lógicas y arbitraje de capacidad
- Versión inicial: 0.3.0
- Estado: consolidado para el contrato actual; preserva capacidades futuras

## 1. Principio
El juego declara lo que quiere que suene. La Machine declara lo que puede sonar. El Runtime adapta la intención del juego a las capacidades de la Machine.

Los scripts solicitan reproducciones; no administran voces. La asignación, sustitución, pérdida, préstamo o robo de capacidad sonora pertenece a Machine y `AudioSystem`.

Las limitaciones de una Machine son comportamiento efectivo, no filtros estéticos. Una Machine limitada puede degradar, omitir, sustituir o interrumpir audio como consecuencia de sus capacidades.

## 2. Voces FLX
Una FLX voice es capacidad lógica simultánea de la Machine. No tiene que corresponder uno a uno con un canal físico del backend de audio.

La Machine declara:

```text
voices.music
voices.sound
voices.mode
voices.overflow
```

`voices.music` limita la capacidad musical. `voices.sound` limita la capacidad de efectos de sonido. La música no consume voces de sound en modo `reserved`.

## 3. Sound
`play_sound(object, id)` resuelve una `SoundDefinition` declarada en el objeto usado como contexto.

Un Sound iniciado es un evento independiente:

- no crea `RuntimeObject`;
- no pertenece al ciclo de vida del `RuntimeObject` que lo inició;
- matar el objeto fuente no detiene sonidos ya iniciados;
- no implica fuente acústica espacial en v0.3.0.

El contrato funcional actual conserva:

- `oscillator` y `noise`;
- `sine`, `square`, `triangle`, `saw`, `pulse` y `noise`;
- `duty`;
- `note` y `frequency`;
- `duration`;
- `volume`;
- `slide`;
- `movement` con comportamiento audible ya implementado;
- material, ADSR y eco básico.

## 4. Priority
Un Sound puede declarar:

```text
priority: integer
```

El valor por defecto es `0`.

`priority` solo participa en políticas que necesitan prioridad. En v0.3.0 se usa en `replace_lowest_priority`.

Si varios Sounds activos tienen la misma prioridad, el desempate debe ser determinista. La política actual usa antigüedad: ante empate se selecciona el Sound activo más antiguo entre los de menor prioridad.

## 5. Music
`play_music(object, id)` resuelve una `MusicDefinition` declarada en el objeto usado como contexto.

La música pertenece al estado global de `AudioSystem`:

- solo existe una música activa;
- iniciar otra música sustituye la actual;
- matar el objeto que resolvió la música no la detiene;
- `stop_music()` detiene la música actual;
- `pause_music()` es un toggle deliberado.

`pause_music()` se comporta como un botón de pausa de reproductor:

```text
playing -> pause_music() -> paused
paused  -> pause_music() -> playing
```

Debe conservar la posición musical.

Los canales musicales son semánticos y ordenados. Si la Machine solo puede materializar `N` voces musicales, se usan los primeros `N` canales declarados y los restantes se omiten con diagnóstico o warning claro según la capa de ejecución.

## 6. Modes
### reserved
En `reserved`, music y sound usan pools separados:

```text
music -> voices.music
sound -> voices.sound
```

Un Sound no puede usar capacidad de Music, salvo que el contrato de la política aplicable lo permita explícitamente. En v0.3.0 `steal_from_music` no cruza pools en `reserved`.

### shared
En `shared`, music y sound comparten una capacidad lógica total:

```text
voices.music + voices.sound
```

Compartir capacidad no autoriza a todas las políticas a expulsar Music. Las políticas `replace_*` arbitran entre Sounds activos elegibles. Solo `steal_from_music` puede cruzar deliberadamente la frontera Sound -> Music.

### preferred
`preferred` queda preservado como capacidad futura. Representa una intención de pools preferentes o prestables entre Music y Sound, situada conceptualmente entre `reserved` y `shared`.

Las reglas exactas de préstamo y recuperación de `preferred` no forman parte del contrato funcional v0.3.0.

## 7. Overflow
`overflow` se aplica cuando llega un nuevo Sound y no existe capacidad Sound elegible disponible.

El Sound entrante no forma parte de la selección de víctima. Primero se elige la víctima entre Sounds activos elegibles; después se admite el entrante.

Políticas:

```text
ignore
```

Rechaza el Sound entrante. No modifica reproducciones existentes.

```text
replace_oldest
```

Termina el Sound activo elegible iniciado más antiguamente. Después reproduce el entrante.

```text
replace_newest
```

Termina el Sound activo elegible iniciado más recientemente. Después reproduce el entrante.

```text
replace_lowest_priority
```

Termina el Sound activo elegible con menor `priority`. En empate usa un criterio determinista basado en antigüedad. Después reproduce el entrante.

```text
steal_from_music
```

Es la única política overflow que permite que Sound obtenga capacidad destinada u ocupada por Music.

## 8. Frontera Sound / Music
Las políticas:

```text
replace_oldest
replace_newest
replace_lowest_priority
```

son Sound <-> Sound. No pueden detener ni reemplazar Music, tampoco en modo `shared`.

La política:

```text
steal_from_music
```

sí puede cruzar la frontera Sound -> Music. En v0.3.0 puede suspender la Music completa mientras el Sound utiliza esa capacidad y retomarla cuando la capacidad vuelve a estar disponible.

## 9. Machine synthesis y fidelity
`audio.synthesis` y `audio.fidelity` describen capacidades de la Machine.

El contrato actual conserva efectos audibles básicos para:

- `synthesis.texture`;
- `synthesis.movement`;
- `synthesis.noise`;
- `fidelity.resolution`;
- `fidelity.dynamics`;
- audio generado.

Los modelos de síntesis declarados que todavía no tienen materialización diferenciada quedan preservados como intención de arquitectura. No deben eliminarse por estar parcialmente implementados.

## 10. Resources y fileAudio
La Machine puede declarar capacidades relacionadas con recursos de audio, como audio generado, samples, streams y `fileAudio.mode`.

En v0.3.0 el contrato funcional se centra en audio generado. Samples, streams y modos de archivo quedan preservados para iteraciones posteriores.

## 11. Capacidades preservadas / futuras
Las siguientes capacidades forman parte de la arquitectura y no se consideran abandonadas, aunque v0.3.0 no materialice todavía toda su semántica:

- Sound source `impact`;
- Sound source `pulse` donde su materialización siga siendo parcial;
- movement `scatter`;
- movement `random`;
- Instrument play `legato`;
- Instrument play `glide`;
- `voices.mode = preferred`;
- modelos de síntesis no diferenciados aún;
- spatial audio: `tone.space.mode`, `tone.space.width` y Machine `fidelity.space`;
- resources `samples`;
- resources `streams`;
- `fileAudio.mode`.

No deben documentarse como promesas funcionales completas de v0.3.0.

## 12. Rationale
Las limitaciones de audio pueden producir comportamiento audible observable, como pérdida temporal de música cuando un efecto roba capacidad sonora.

Este comportamiento representa máquinas con capacidad sonora limitada. Puede inspirarse en hardware histórico, pero FLX no debe convertir esa referencia en un preset nominal ni en una emulación concreta.
