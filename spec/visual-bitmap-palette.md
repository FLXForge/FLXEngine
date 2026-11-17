# Paletas declarativas y Bitmap

Especificación normativa, FLX Engine v0.4.0. Complementa `visual-representation.md`.
JSON expresa intención; Machine determina el color efectivo. No hay assets gráficos
públicos, paletas globales, cambios de paleta en runtime ni referencias pendientes.

## Paletas

`visual.palette` es opcional. Su gramática es:

```text
palette       := map | [ (map | reference)* ]
map           := { string: nativeColor, ... }
nativeColor   := nombre nativo FLX | #RGB | #RRGGBB
reference     := archivo relativo | /ruta lógica | archivo:miembro | :miembro
```

Una referencia debe seleccionar un mapa de entradas, no otro objeto de juego ni
una composición recursiva. Se reutilizan caché, raíces, miembros y diagnósticos
del resolver de FLX. Las rutas relativas se interpretan desde el archivo que
declara la referencia, incluso dentro de un `visual` externo o heredado con `like`.

Los mapas se combinan de izquierda a derecha: la última definición de cada clave
prevalece. Cada visual obtiene un resultado independiente. No se mutan mapas
compartidos ni archivos fuente. La paleta se normaliza antes de mezclar overrides
de herencia, conservando la procedencia de cada declaración.

La búsqueda de un color usado en `visual.color`, en un elemento o en un píxel
explícito consulta primero la clave exacta de la paleta y después el parser nativo.
Las claves distinguen mayúsculas; los nombres nativos no. Las claves pueden
sobrescribir nombres nativos, incluido `blank`.

Los valores de las entradas se interpretan directamente mediante el parser nativo,
sin buscar otras claves. No existe resolución recursiva de alias.

```json
{"visual":{"palette":["./commons/colors.json:arcade",{"red":"blue","0":"blank"}],"color":"red"}}
```

Aquí `red` usado como color resuelve a azul. El valor nativo `blank` de `0`
sigue solicitando transparencia, aunque otra clave redefina `blank`.

## Gramática de Bitmap

Cada elemento de `visual.representation` declara exactamente una familia:
`primitive`, `geometry`, `text` o `bitmap`.

```text
bitmapElement := { bitmap: { pixels: [ rowDeclaration+ ] }, size?: representationSize, color?: string }
rowDeclaration := row | { row: row, repeat?: positiveInteger }
row            := compactString | [ (colorToken | horizontalPattern)+ ]
horizontalPattern := { pattern: compactString | [colorToken+], repeat?: positiveInteger }
```

`repeat` tiene valor predeterminado 1. Solo admite enteros de 1 a 255; un valor
mayor necesariamente excedería una dimensión. No se admiten patrones anidados,
filas vacías, matrices vacías ni propiedades adicionales en filas/patrones/bitmap.

Una cadena compacta se separa en caracteres Unicode (valores escalares, no bytes
UTF-8 ni grafemas). Cada carácter debe ser una clave de la paleta. Un array
explícito admite claves completas, nombres nativos o hexadecimales. No hay importación
de bloques dentro de matrices ni composición arbitraria de bitmaps completos.

```json
{"palette":{"0":"blank","1":"red"},"representation":[
  {"bitmap":{"pixels":["0110",{"row":[{"pattern":"10","repeat":2}],"repeat":2},"0110"]}}
]}
```

Primero se expanden las repeticiones horizontal y vertical, con comprobaciones
previas a cada copia para impedir desbordamientos y expansiones ilimitadas.
La matriz expandida debe ser rectangular, de 1 a 255 columnas y de 1 a 255 filas.
Después se resuelven todos los colores y se construye la representación normalizada.

## Color y Machine

Cada píxel contiene su propio RGBA; ni `visual.color`, ni `apply_color`, ni el
color del elemento tiñen o sustituyen esos colores. Si el elemento declara `color`,
se valida igualmente para no ocultar errores de declaración.

Todos los píxeles pasan por `VideoColorProcessor` durante compilación, incluida
la transparencia. No hay un píxel especial para omitir dibujo. `blank` es el
color nativo transparente de FLX: con `color.alpha: false`, Machine lo transforma
en opaco y aplica después su paleta, tono o cuantización actuales.

## Transformaciones, orden y rendimiento

Las dimensiones de la matriz son intrínsecas. No se declara width/height dentro
de `bitmap`. El `size` opcional del elemento tiene las mismas dimensiones numéricas
o porcentuales que una primitiva. Los ejes omitidos heredan el tamaño del objeto;
si este no tiene `size`, se usa el tamaño natural de la matriz. Los porcentajes
usan esa misma base. El bitmap no crea tamaño espacial, colisiones ni delimitación.

Se centra en el pivot, sigue posición y ángulo vivos, y conserva el snapshot y el
orden de composición existentes. La rotación se aplica en el mundo antes de
proyectar a la resolución raster, también con escalas distintas en X e Y.

La implementación crea una textura interna con filtro puntual y la reutiliza
por mundo. Se dibuja un quad por instancia/presentación, no miles de rectángulos
por fotograma. La textura no se serializa, no exige archivos externos y se libera
con el mundo mientras su contexto gráfico siga disponible. El host debe conservar
ese contexto durante la vida de la sesión, como exige el contrato de sesión.

## Diagnósticos

- FLX-RESOURCE-00019 `UnknownVisualColor`: color usado o clave compacta inexistente.
- FLX-RESOURCE-00020 `InvalidVisualPalette`: declaración o valor nativo inválido.
- FLX-RESOURCE-00021 `InvalidBitmapDeclaration`: matriz, repetición o modelo inválido.
- Las referencias conservan los códigos existentes de recurso ausente y miembro ausente,
  con archivo de declaración, campo y ruta resuelta.

Ejemplos inválidos:

```json
{"palette":{"primary":"red","secondary":"primary"}}
```

`primary` no es un valor nativo válido dentro de la paleta. Su existencia como
clave no lo convierte en alias encadenable.

```json
{"bitmap":{"pixels":["11","1"]}}
```

La matriz no es rectangular. También se rechazan `repeat: 0`, `repeat: true`,
fracciones, dimensiones expandidas mayores de 255 y colores como `none`.

## Persistencia y límites de v0.4.0

El writer produce formato 13; el reader conserva lectura del 12. La nueva familia
usa etiqueta 3 y datos RGBA normalizados, sin alias ni referencias. Los bitmaps
son inmutables; cambiar colores requerirá futuras decisiones de contrato.

No se añaden paletas dinámicas, animaciones diferenciales, normalización de
grafemas, importación de imágenes ni API pública de assets. Los formatos hexadecimales
se limitan a los ya nativos: #RGB y #RRGGBB; la transparencia declarativa usa `blank`.
