# Persistencia de partidas

## Estado
- Documento: especificación normativa
- Ámbito: save(), load() y ubicación de archivos .flxsave

## Responsabilidades
El juego JavaScript DEBE decidir los datos mediante las firmas existentes de
save(name, key, value) y load(name, key, defaultValue). FLX Core DEBE gestionar
el formato, lectura y escritura. El anfitrión DEBE decidir la ubicación física.
Machine, JSON, YAML y CompiledProject NO DEBEN almacenar esta configuración.

## Directorio por sesión
FlxSessionOptions::savePath y RunOptions::savePath son rutas opcionales.
Al cargar la sesión, el motor DEBE resolver la ruta una sola vez a una ruta
absoluta normalizada léxicamente. Si se omite, DEBE utilizar cwd/saves.
Una ruta relativa DEBE interpretarse respecto al directorio de trabajo en ese
instante. El motor NO DEBE cambiar el directorio de trabajo del proceso.

La configuración resuelta pertenece a PersistenceSystem de esa sesión y DEBE
ser idéntica para lectura y escritura. Descargar y recargar DEBE resolver la
nueva configuración. No se añaden subdirectorios por proyecto, juego o sesión.
Dos sesiones con directorios diferentes NO DEBEN compartir archivos por estado
interno del motor. Si comparten directorio y nombre, PUEDEN compartir datos.
No se garantiza escritura concurrente ni aislamiento frente a alias/symlinks.

## Errores y compatibilidad
Una ruta explícita vacía o con NUL DEBE rechazar la carga con diagnóstico.
Los restantes errores de ruta o permisos del sistema de archivos DEBEN
diagnosticarse al acceder al archivo. El directorio PUEDE no existir al cargar;
DEBE intentarse su creación al escribir. No se verifica su escritura anticipadamente.
Nunca DEBE haber fallback silencioso a otro directorio.
La ruta efectiva DEBE registrarse mediante el logging de depuración existente.

save() mantiene su retorno JavaScript undefined; un fallo no introduce excepciones
JS nuevas. load() DEBE mantener el valor predeterminado si el dato no se recupera.
Los nombres y claves mantienen su validación actual.
La extensión .flxsave, magic FLXSAVE1, versión binaria 1, checksum, ofuscación y
tipos boolean/number/string NO DEBEN cambiar. Los archivos existentes DEBEN
seguir legibles al seleccionar su directorio. Descargar NO DEBE borrar guardados.

## CLI
run y run-compiled DEBEN aceptar --save-path=<directory> una sola vez, con valor
no vacío; los argumentos con espacios requieren las comillas normales del shell.
La omisión selecciona el modo portable. compile y build NO DEBEN aceptar esta
opción. La opción NO DEBE modificar el proyecto ni el binario compilado.

## Invariantes verificables
- Guardar después de cambiar cwd utiliza la ruta fijada en la carga.
- Leer tras guardar utiliza el mismo archivo.
- Un fallo de creación/escritura no produce un guardado en otra ruta.
- Dos rutas distintas aíslan datos; la misma ruta permite compartirlos.
- Un archivo del formato anterior conserva los mismos valores.
