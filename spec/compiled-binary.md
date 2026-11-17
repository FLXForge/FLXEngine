# Formato binario de proyectos compilados (`.flxc`)

## Estado
- Documento: especificación normativa
- Ámbito: persistencia binaria de `CompiledProject`
- Versión inicial: 0.3.0
- Formato vigente: 13
- Estado: consolidado

## 1. Propósito
`.flxc` es la representación binaria persistente de un `CompiledProject`.
Permite guardar, transportar, validar, reconstruir y ejecutar un proyecto compilado sin consultar sus fuentes.

## 2. Flujo
```text
project.flx → ProjectCompiler → CompiledProject → Runtime
.flxc → CompiledProjectReader → CompiledProject → Runtime
```

## 3. Responsabilidades
`CompiledProjectWriter` valida, aplica límites, escribe mediante temporal y emite Diagnostics.
`CompiledProjectReader` valida firma, versión, límites, duplicados, datos sobrantes e invariantes.
`CompiledProjectCodec` conoce la disposición lógica del formato.
`BinaryReader` y `BinaryWriter` conocen primitivas, endianness, límites y errores físicos.
`CompiledProjectValidator` valida únicamente el modelo compilado.

## 4. Cabecera
```text
magic
formatVersion
producerVersion
```
Formato vigente:
```text
FormatVersion = 13
```
Códigos:
- FLX-BINARY-00013 InvalidCompiledProjectMagic
- FLX-BINARY-00014 UnsupportedCompiledProjectFormat

## 5. Disposición lógica
```text
compiled context
rootId
object resources
script resources
```
No contiene fuentes necesarias para ejecutar, referencias pendientes, `rootDefinition` duplicada ni árboles `children`.

## 6. Primitivas
- `u8`
- `u32` little-endian
- `i32` como bits escritos mediante `u32`
- `f32` IEEE-754 binary32 little-endian
- `bool` como `u8`: 0 o 1
- `string`: `u32 length` + bytes UTF-8

Valores booleanos distintos de 0 y 1 producen:
```text
FLX-BINARY-00020 InvalidCompiledProjectValue
```

## 7. Límites
```text
MaxBinaryFileSize             = 64 MiB
MaxStringSize                 = 32 MiB
MaxTotalDecodedElements       = 50.000
MaxTotalDecodedStringBytes    = 96 MiB
MaxResourceCount              = 100.000
MaxCollectionCount            = 100.000
MaxPointCount                 = 100.000
MaxMusicChannelCount          = 64
MaxGridRowCount               = 10.000
```
Los límites globales prevalecen sobre los locales.
Writer y Reader deben aplicar límites compatibles.
El Writer comprueba el tamaño físico del temporal antes de sustituir la salida final.

## 8. Presupuesto global
Reader y Writer controlan:
```text
totalElements
totalStringBytes
```
No se reserva memoria a partir de un contador sin validarlo.

## 9. Determinismo
La serialización debe usar un orden estable.
Dos compilaciones semánticamente idénticas deben producir los mismos bytes.

## 10. Recursos
`.flxc` persiste el `ResourceRegistry`.
Los objetos almacenan `childResources`, no árboles `children`.
Los scripts conservan `ResourceId`, `sourceName` y contenido.
La clave de script debe coincidir con `ScriptResource.id`.
Cuando `ObjectDefinition.id` representa una identidad compilada explícita, debe coincidir con la clave del registro.

## 11. Duplicados
Se rechazan:
- objetos duplicados;
- scripts duplicados;
- entradas duplicadas en music;
- entradas duplicadas en sounds;
- entradas duplicadas en childResources;
- estados duplicados.

Códigos:
```text
FLX-BINARY-00017 DuplicateCompiledResource
FLX-BINARY-00019 DuplicateCompiledEntry
```

## 12. Validación
Antes de escribir y después de leer se ejecuta `CompiledProjectValidator`.
Comprueba:
- rootId;
- raíz existente;
- ausencia de children embebidos;
- childResources válidos;
- scripts existentes;
- coherencia de identidades.

Compiler, Writer, Reader y Runtime deben compartir la misma identidad diagnóstica para cada invariante.

## 13. Escritura transaccional
```text
validar
→ escribir .tmp
→ flush
→ close
→ comprobar tamaño
→ preservar salida anterior
→ renombrar
→ eliminar backup
```
Si falla, se elimina el temporal y se conserva o restaura la salida anterior.
No se soportan dos escrituras simultáneas sobre el mismo destino.

## 14. Lectura
Debe distinguir:
- archivo no accesible;
- firma inválida;
- versión no soportada;
- truncado;
- límite excedido;
- valor inválido;
- recurso duplicado;
- entrada duplicada;
- datos sobrantes;
- proyecto reconstruido inválido.

## 15. Diagnósticos binarios
- FLX-BINARY-00010 CompiledProjectCouldNotBeOpened
- FLX-BINARY-00011 CompiledProjectCouldNotBeCreated
- FLX-BINARY-00012 CompiledProjectWriteFailed
- FLX-BINARY-00013 InvalidCompiledProjectMagic
- FLX-BINARY-00014 UnsupportedCompiledProjectFormat
- FLX-BINARY-00015 TruncatedCompiledProject
- FLX-BINARY-00016 CompiledProjectLimitExceeded
- FLX-BINARY-00017 DuplicateCompiledResource
- FLX-BINARY-00018 TrailingCompiledProjectData
- FLX-BINARY-00019 DuplicateCompiledEntry
- FLX-BINARY-00020 InvalidCompiledProjectValue

Las invariantes del modelo usan diagnósticos COMP o RESOURCE, no BINARY.

## 16. Metadata
`CompiledProjectBinaryResult` expone:
```text
success
project
metadata
diagnostics
```
Metadata:
```text
formatVersion
producerVersion
```

## 17. Compatibilidad
La v0.3.0 no garantiza compatibilidad con formatos anteriores.
El lector v3 rechaza v2 explícitamente.
Antes de v1 deben definirse política de compatibilidad, migración y estabilidad de `ResourceId`.

## 18. Seguridad
Todo `.flxc` se trata como entrada no confiable.
El Reader debe protegerse frente a tamaños falsos, expansión acumulada, valores inválidos, duplicados, truncado, datos sobrantes e identidades incoherentes.

## 19. Tests mínimos
- roundtrip;
- metadata;
- determinismo byte a byte;
- rechazo de v2;
- magic inválido;
- truncado;
- archivo inexistente;
- límite físico del Reader;
- límite físico del Writer;
- presupuestos globales;
- bool inválido;
- duplicados;
- identidades incoherentes;
- trailing bytes;
- ausencia de archivo parcial;
- conservación de salida anterior;
- validación común;
- ejecución sin fuentes;
- smoke desde `.flxc`.

## 20. Reglas normativas
- BIN-001 Todo `.flxc` comienza con firma válida.
- BIN-002 Todo `.flxc` declara su versión.
- BIN-003 v3 utiliza little-endian.
- BIN-004 bool solo admite 0 y 1.
- BIN-005 `f32` es IEEE-754 binary32.
- BIN-006 Writer y Reader aplican límites compatibles.
- BIN-007 La lectura mantiene presupuestos globales.
- BIN-008 La serialización es determinista.
- BIN-009 Los duplicados se rechazan.
- BIN-010 No se aceptan bytes sobrantes.
- BIN-011 El Writer no deja una salida final parcial.
- BIN-012 El proyecto se valida antes de escribir y después de leer.
- BIN-013 Los fallos de filesystem producen Diagnostics.
- BIN-014 Las versiones no soportadas se rechazan explícitamente.
- BIN-015 No se conservan fuentes necesarias para ejecutar.
- BIN-016 No se soportan escrituras concurrentes al mismo destino.

## 21. Lo que no es `.flxc`
No es todavía:
- paquete de distribución;
- ejecutable;
- archivo comprimido;
- sistema de firma;
- atlas;
- bytecode;
- `.flxpack`.

Es la persistencia de `CompiledProject`.

## 22. Principio final
```text
documentos
→ significado
→ representación
→ bytes
→ representación
→ ejecución
```

## 23. Bitmap y compatibilidad del formato 13 (v0.4.0)

El writer escribe formato 13. El reader admite 12 y 13; el resto se rechaza.
El formato 12 se lee sin añadir campos ni reinterpretar etiquetas. Su fixture
Windows permanece intacto. Un reader antiguo no puede ejecutar un bitmap 13.

Las etiquetas existentes conservan 0=primitive, 1=geometry y 2=text.
La etiqueta 3=bitmap solo es válida en formato 13 y añade, después de los
campos comunes del elemento:

```text
u8 width (1..255)
u8 height (1..255)
u32 byteLength = width * height * 4
RGBA bytes, orden por filas; un byte por canal R, G, B, A
```

No se persisten texturas, alias ni paletas declarativas. Los colores ya han
pasado por Machine. El reader valida dimensiones y longitud exacta antes de
reservar el payload. Writer y validator comprueban el número de píxeles.

Los bytes del payload comparten con los strings el presupuesto agregado de
96 MiB; el límite físico de 64 MiB sigue vigente. Un bitmap es un registro con
payload acotado, no 65.025 entradas del presupuesto de elementos. Esto permite
255x255 sin relajar MaxTotalDecodedElements. El payload es binario, no UTF-8.
