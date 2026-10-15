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

## 6. Estado lógico y materialización Machine
El estado observable de Music expresa la intención lógica del juego, no el estado físico instantáneo del backend de audio.

Arquitectura:

```text
Declarative/logical state
        ↓
Machine capabilities/policies
        ↓
Effective materialization
```

La materialización efectiva no redefine hacia atrás el estado lógico.

`music_active()` observa si existe una Music activa solicitada por el juego. Una Music lógicamente pausada o temporalmente suspendida por Machine sigue activa hasta que termina o se llama a `stop_music()`.

`music_paused()` observa exclusivamente la pausa lógica solicitada por el juego mediante `pause_music()`. Una suspensión técnica de Machine, por ejemplo por `steal_from_music`, no convierte Music en pausada desde la API de scripting.

Conceptualmente, el juego dice que Music debería reproducirse; Machine determina si puede materializar esa intención en cada momento.

Por tanto, una Music puede estar:

- activa y lógicamente reproduciéndose;
- activa y lógicamente pausada;
- activa y lógicamente reproduciéndose, pero temporalmente suspendida por Machine;
- activa y lógicamente pausada, además de temporalmente suspendida por Machine.

La reanudación física solo puede ocurrir cuando no exista pausa lógica y no exista suspensión técnica de Machine.

## 7. Modes
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

## 8. Overflow
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

## 9. Frontera Sound / Music
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

sí puede cruzar la frontera Sound -> Music. En v0.3.0 puede suspender la materialización efectiva de la Music completa mientras el Sound utiliza esa capacidad y retomarla cuando la capacidad vuelve a estar disponible.

La suspensión producida por `steal_from_music` pertenece a Machine. No cambia el estado lógico de Music, no hace que `music_paused()` devuelva `true` y no equivale a una llamada del juego a `pause_music()`.

Si el juego llama a `pause_music()` mientras Music está suspendida por `steal_from_music`, la pausa lógica debe sobrevivir a la liberación de la voz. Al terminar el Sound, Music solo se materializa de nuevo si ya no está lógicamente pausada.

## 10. Machine synthesis y fidelity
`audio.synthesis` y `audio.fidelity` describen capacidades de la Machine.

El contrato actual conserva efectos audibles básicos para:

- `synthesis.texture`;
- `synthesis.movement`;
- `synthesis.noise`;
- `fidelity.resolution`;
- `fidelity.dynamics`;
- audio generado.

Los modelos de síntesis declarados que todavía no tienen materialización diferenciada quedan preservados como intención de arquitectura. No deben eliminarse por estar parcialmente implementados.

## 11. Resources y fileAudio
La Machine puede declarar capacidades relacionadas con recursos de audio, como audio generado, samples, streams y `fileAudio.mode`.

En v0.3.0 el contrato funcional se centra en audio generado. Samples, streams y modos de archivo quedan preservados para iteraciones posteriores.

## 12. Capacidades preservadas / futuras
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

## 13. Rationale
Las limitaciones de audio pueden producir comportamiento audible observable, como pérdida temporal de música cuando un efecto roba capacidad sonora.

Este comportamiento representa máquinas con capacidad sonora limitada. Puede inspirarse en hardware histórico, pero FLX no debe convertir esa referencia en un preset nominal ni en una emulación concreta.

## 14. Síntesis musical incremental (v0.3.1)

La partitura sigue siendo la fuente de verdad. `MusicSequencer` prepara solo los
canales admitidos por Machine, copia instrumentos, resuelve frecuencias y calcula
la duración. La preparación cuesta O(notas + canales), con almacenamiento
O(notas + canales). No genera ni almacena PCM completo. Conserva el cursor absoluto
y el orden de mezcla, material, ADSR, vibrato, eco y fidelidad del algoritmo anterior.
El historial circular de eco contiene 1984 floats (7936 bytes, aproximadamente
45 ms a 44100 Hz), guardando la mezcla después de fidelidad.

El formato sigue siendo mono, 44100 Hz, PCM de 16 bits. El bloque interno fijo
es de 4096 frames (8192 bytes, aproximadamente 92,9 ms). raylib almacena dos mitades
de 4096 frames (16384 bytes de PCM), además de sus buffers de conversión y del
dispositivo. No hay caché persistente ni nuevos parámetros de Machine.

`playMusic()` libera la reproducción anterior, prepara la partitura, crea el
stream y sintetiza como máximo dos bloques antes de retornar. Ese trabajo sigue
siendo síncrono, pero su coste de síntesis no depende de la duración musical.
`update()` entrega como máximo dos bloques por frame; no se sintetiza en el
callback del dispositivo.

En raylib 6.0, `IsAudioStreamProcessed()` indica si alguna mitad está disponible.
`UpdateAudioStream()` escribe una sola mitad; si ambas están libres selecciona
la primera y reposiciona el cursor. `PlayAudioStream()` reinicia cursor y marcas
de ambas mitades: el arranque hace Play, Pause, prepara ambas mitades y Resume.
Las reanudaciones posteriores usan exclusivamente Resume.

Dos marcas registran bloques musicales pendientes en orden de entrega. Cada mitad
libre permite retirar una marca y entregar otro bloque. Tras terminar la síntesis
se entregan marcadores de silencio hasta retirar los bloques musicales pendientes;
solo entonces se libera el stream. `IsAudioStreamPlaying()` no indica fin natural,
porque raylib mantiene los streams en bucle. La última mitad se rellena con ceros:
el estado activo puede durar hasta 92,9 ms adicionales más el intervalo hasta
el siguiente update. Consumo significa aquí consumo del mezclador de raylib;
la latencia física adicional depende del dispositivo.

Los bucles cruzan el final de la partitura dentro del mismo bloque, reinician
cursor e historial de eco y conservan el stream, sin padding entre iteraciones.
Una discontinuidad propia de la partitura al unir final y principio sigue siendo
posible, como antes. La copia preparada es independiente del objeto fuente.

Pausa lógica y suspensión por `steal_from_music` pausan físicamente el stream,
sin generar más bloques ni perder posición. La suspensión sigue afectando a toda
la música. Liberar voces solo reanuda si no existe pausa lógica. Parada,
sustitución, fin natural y shutdown liberan stream, partitura e historial.

El ruido conserva `GetRandomValue()`. Con semilla e idéntico orden de llamadas,
dividir en bloques conserva el PCM. Llamadas aleatorias del juego o efectos
intercaladas pueden cambiar la realización respecto a la síntesis anticipada;
no se promete reproducibilidad del ruido durante el juego. Legato, glide y otras
capacidades futuras conservan sus limitaciones anteriores.

El motor debe actualizar audio regularmente. Bloqueos superiores al audio
pendiente pueden producir underrun y silencio; el siguiente update recupera
la alimentación sin reiniciar la composición. raylib puede ampliar una mitad si
el período interno del dispositivo supera 4096 frames: esos dispositivos
excepcionales pueden introducir silencio adicional y requieren validación
específica. La parametrización de buffers queda reservada para v0.4.0.

### Verificación

`flx-audio-system-tests` compara PCM exacto con el algoritmo anterior, compilado
solo para pruebas: varios tempos, formas de onda, mezcla, ADSR, materiales,
vibrato, eco y bloques de 1, 127, 1984, 4096 y 8191 frames. Incluye ruido con semilla
fija y bucles deterministas. `flx-audio-stream-tests` prueba el AudioSystem real
contra un modelo del doble buffer sin dispositivo: inicio, drenaje, pausa,
sustitución, bucles, underrun, voces, fallos de carga y liberación.

`flx-audio-device-tests` es una prueba optativa con raylib y dispositivo real;
no forma parte de CTest porque reproduce tonos. Devuelve 77 sin dispositivo.
Muestra tiempos de inicio para partituras de 50 y 750 segundos y el coste del
algoritmo anterior para 50 segundos. Son tiempos locales, no latencia acústica.

Para comprobar manualmente, ejecutar `flx-audio-device-tests` desde la carpeta
de build y escuchar inicio, pausa y bucle. En un juego con `steal_from_music`,
lanzar un efecto y alternar `pause_music()` durante la suspensión: al terminar
el efecto solo debe reanudarse si `music_paused()` es falso. Destruir el objeto
fuente no debe detener la música. Para medir latencia acústica, registrar
conjuntamente la llamada y la salida de audio con un dispositivo de captura.

Medición orientativa en este checkout, compilación Debug con MSVC y WASAPI
(48000 Hz, período de 1440 frames): `playMusic()` tardó 11,30 ms para 50 segundos
y 11,65 ms para 750 segundos, ambos con 1000 notas. Sintetizar los 50 segundos
completos con el algoritmo anterior tardó 2057,69 ms. Una ejecución, sin medición
acústica; no constituye una garantía de rendimiento en otras máquinas.

Para ejecutar CTest manteniendo sus archivos temporales dentro del checkout,
crear `out/audio-test-temp`, asignar `TMP` y `TEMP` a su ruta absoluta y ejecutar
`ctest --test-dir out/build/x64-debug --output-on-failure`. Las pruebas gráficas
pueden ejecutarse sin ventana con `FLX_TEST_HEADLESS=1`.
