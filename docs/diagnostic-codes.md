# Códigos de diagnóstico de E-Merge FLX

Formato canónico:

```text
FLX-DOMAIN-NNNNN
```

Cada código tiene un `Identifier` estable y único. El identificador implementado actualmente está cualificado por dominio para evitar ambigüedad.

Ejemplo:

```text
Código: FLX-COMP-00002
Identifier: CompErrorUnclassified
Severity: error
```

## Reservas por dominio

```text
00000 Información no clasificada
00001 Advertencia no clasificada
00002 Error no clasificado
00003 Error interno inesperado
00004 Funcionalidad obsoleta
00005-00009 Reservados
00010+ Diagnósticos específicos
```

## Dominios implementados

```text
CLI
PROJECT
COMP
RESOURCE
MACHINE
INPUT
BINARY
RUNTIME
BUILD
```

## Códigos implementados

### CLI

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-CLI-00000 | CliInformationUnclassified | info | Diagnóstico informativo no clasificado del CLI. |
| FLX-CLI-00001 | CliWarningUnclassified | warning | Advertencia no clasificada del CLI. |
| FLX-CLI-00002 | CliErrorUnclassified | error | Error no clasificado del CLI. |
| FLX-CLI-00003 | CliUnexpectedInternalError | error | Error interno inesperado del CLI. |
| FLX-CLI-00004 | CliDeprecatedFunctionality | warning | Funcionalidad obsoleta del CLI. |

### PROJECT

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-PROJECT-00000 | ProjectInformationUnclassified | info | Diagnóstico informativo no clasificado de resolución de proyecto. |
| FLX-PROJECT-00001 | ProjectWarningUnclassified | warning | Advertencia no clasificada de resolución de proyecto. |
| FLX-PROJECT-00002 | ProjectErrorUnclassified | error | Error no clasificado de resolución de proyecto. |
| FLX-PROJECT-00003 | ProjectUnexpectedInternalError | error | Error interno inesperado de resolución de proyecto. |
| FLX-PROJECT-00004 | ProjectDeprecatedFunctionality | warning | Funcionalidad obsoleta de resolución de proyecto. |
| FLX-PROJECT-00010 | ProjectManifestCouldNotBeOpened | error | No se ha podido abrir el manifiesto del proyecto. |
| FLX-PROJECT-00011 | InvalidProjectManifestSyntax | error | Una línea del manifiesto del proyecto no cumple la sintaxis `key=value`. |
| FLX-PROJECT-00012 | UnknownProjectManifestField | error | El manifiesto del proyecto contiene una clave desconocida. |
| FLX-PROJECT-00013 | DuplicateProjectManifestField | error | El manifiesto del proyecto contiene una clave repetida. |
| FLX-PROJECT-00014 | MissingProjectRoot | error | El manifiesto del proyecto no define un `root` válido. |

### COMP

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-COMP-00000 | CompInformationUnclassified | info | Diagnóstico informativo no clasificado del compilador. |
| FLX-COMP-00001 | CompWarningUnclassified | warning | Advertencia no clasificada del compilador. |
| FLX-COMP-00002 | CompErrorUnclassified | error | Error no clasificado del compilador. |
| FLX-COMP-00003 | CompUnexpectedInternalError | error | Error interno inesperado del compilador. |
| FLX-COMP-00004 | CompDeprecatedFunctionality | warning | Funcionalidad obsoleta del compilador. |
| FLX-COMP-00010 | CompiledProjectMissingRoot | error | Falta el identificador raíz o el recurso raíz del proyecto compilado. |
| FLX-COMP-00011 | CompiledProjectEmbeddedChildren | error | Un objeto compilado conserva hijos embebidos. |
| FLX-COMP-00012 | CompiledProjectMissingChildResource | error | Un childResource del proyecto compilado apunta a un objeto inexistente. |
| FLX-COMP-00013 | CompiledProjectMissingScriptResource | error | Un script resuelto del proyecto compilado apunta a un script inexistente. |
| FLX-COMP-00014 | CompiledProjectIdentityMismatch | error | Una clave de registro compilado no coincide con la identidad declarada del recurso. |
| FLX-COMP-00015 | AutomaticInstantiationCycle | error | Se ha detectado un ciclo de instanciacion automatica. |
| FLX-COMP-00016 | CompiledProjectInvalidStateMachine | error | La maquina de estados compilada no cumple las invariantes del modelo. |

### RESOURCE

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-RESOURCE-00000 | ResourceInformationUnclassified | info | Diagnóstico informativo no clasificado de recurso. |
| FLX-RESOURCE-00001 | ResourceWarningUnclassified | warning | Advertencia no clasificada de recurso. |
| FLX-RESOURCE-00002 | ResourceErrorUnclassified | error | Error no clasificado de recurso. |
| FLX-RESOURCE-00003 | ResourceUnexpectedInternalError | error | Error interno inesperado de recurso. |
| FLX-RESOURCE-00004 | ResourceDeprecatedFunctionality | warning | Funcionalidad obsoleta de recurso. |
| FLX-RESOURCE-00010 | MissingReferencedResource | error | No se ha podido encontrar o cargar un recurso referenciado. |
| FLX-RESOURCE-00011 | MissingInternalResourceMember | error | No se ha podido encontrar un miembro interno referenciado con `:`. |
| FLX-RESOURCE-00012 | ResourceReferenceCycle | error | Se ha detectado un ciclo de referencias de recurso. |
| FLX-RESOURCE-00013 | ResourceIdCollision | error | Dos recursos distintos han producido la misma identidad lógica. |
| FLX-RESOURCE-00014 | ReferencedScriptNotFound | error | No se ha podido encontrar un script referenciado. |
| FLX-RESOURCE-00015 | InvalidStateMachineDeclaration | error | La declaracion de states no cumple la estructura requerida. |
| FLX-RESOURCE-00016 | MissingStateMachineInitialState | error | La maquina de estados no declara un initial valido. |
| FLX-RESOURCE-00017 | MissingStateMachineState | error | Una maquina de estados referencia un estado no declarado. |
| FLX-RESOURCE-00018 | InvalidStateTransitionTarget | error | Un destino de transicion no es valido. |

### MACHINE

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-MACHINE-00000 | MachineInformationUnclassified | info | Diagnóstico informativo no clasificado de Machine. |
| FLX-MACHINE-00001 | MachineWarningUnclassified | warning | Advertencia no clasificada de Machine. |
| FLX-MACHINE-00002 | MachineErrorUnclassified | error | Error no clasificado de Machine. |
| FLX-MACHINE-00003 | MachineUnexpectedInternalError | error | Error interno inesperado de Machine. |
| FLX-MACHINE-00004 | MachineDeprecatedFunctionality | warning | Funcionalidad obsoleta de Machine. |

### INPUT

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-INPUT-00000 | InputInformationUnclassified | info | Diagnóstico informativo no clasificado de Input. |
| FLX-INPUT-00001 | InputWarningUnclassified | warning | Advertencia no clasificada de Input. |
| FLX-INPUT-00002 | InputErrorUnclassified | error | Error no clasificado de Input. |
| FLX-INPUT-00003 | InputUnexpectedInternalError | error | Error interno inesperado de Input. |
| FLX-INPUT-00004 | InputDeprecatedFunctionality | warning | Funcionalidad obsoleta de Input. |
| FLX-INPUT-00010 | InputMappingCouldNotBeOpened | error | No se ha podido abrir el archivo de input mapping. |
| FLX-INPUT-00011 | InputMappingMalformedLine | error | Una línea del input mapping no cumple la sintaxis `key=value`. |
| FLX-INPUT-00012 | InputMappingUnknownKey | error | El input mapping contiene una clave raíz desconocida. |
| FLX-INPUT-00013 | InputMappingInvalidKey | error | Una clave del input mapping no tiene una estructura válida. |
| FLX-INPUT-00014 | InputMappingPlayerOutOfRange | error | El input mapping referencia un player fuera de la capacidad del Input Chip. |
| FLX-INPUT-00015 | InputMappingButtonOutOfRange | error | El input mapping referencia un botón fuera de la capacidad del Input Chip. |
| FLX-INPUT-00016 | InputMappingDirectionOutOfRange | error | El input mapping referencia una dirección fuera de la capacidad del Input Chip. |
| FLX-INPUT-00017 | InputMappingUnknownDirectionComponent | error | El input mapping referencia un componente direccional desconocido. |
| FLX-INPUT-00018 | InputMappingUnknownPhysicalToken | error | El input mapping referencia un token físico desconocido. |
| FLX-INPUT-00019 | InputMappingEmptyBinding | error | Una asignación del input mapping está vacía o incompleta. |
| FLX-INPUT-00020 | InputMappingDuplicateBinding | error | El input mapping declara la misma clave lógica más de una vez. |
| FLX-INPUT-00021 | InputMappingEmpty | error | El input mapping no contiene declaraciones efectivas. |

### BINARY

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-BINARY-00000 | BinaryInformationUnclassified | info | Diagnóstico informativo no clasificado de binario compilado. |
| FLX-BINARY-00001 | BinaryWarningUnclassified | warning | Advertencia no clasificada de binario compilado. |
| FLX-BINARY-00002 | BinaryErrorUnclassified | error | Error no clasificado de binario compilado. |
| FLX-BINARY-00003 | BinaryUnexpectedInternalError | error | Error interno inesperado de binario compilado. |
| FLX-BINARY-00004 | BinaryDeprecatedFunctionality | warning | Funcionalidad obsoleta de binario compilado. |
| FLX-BINARY-00010 | CompiledProjectCouldNotBeOpened | error | No se ha podido abrir el archivo de proyecto compilado. |
| FLX-BINARY-00011 | CompiledProjectCouldNotBeCreated | error | No se ha podido crear el archivo de proyecto compilado. |
| FLX-BINARY-00012 | CompiledProjectWriteFailed | error | No se ha podido escribir o finalizar el archivo de proyecto compilado. |
| FLX-BINARY-00013 | InvalidCompiledProjectMagic | error | El archivo de proyecto compilado no contiene la firma esperada. |
| FLX-BINARY-00014 | UnsupportedCompiledProjectFormat | error | La version del formato de proyecto compilado no esta soportada. |
| FLX-BINARY-00015 | TruncatedCompiledProject | error | El archivo de proyecto compilado esta truncado. |
| FLX-BINARY-00016 | CompiledProjectLimitExceeded | error | El archivo de proyecto compilado supera un limite tecnico del formato. |
| FLX-BINARY-00017 | DuplicateCompiledResource | error | El archivo de proyecto compilado contiene un recurso duplicado. |
| FLX-BINARY-00018 | TrailingCompiledProjectData | error | El archivo de proyecto compilado contiene datos sobrantes al final. |
| FLX-BINARY-00019 | DuplicateCompiledEntry | error | El archivo de proyecto compilado contiene una entrada duplicada dentro de un recurso. |
| FLX-BINARY-00020 | InvalidCompiledProjectValue | error | El archivo de proyecto compilado contiene un valor binario inválido. |

### RUNTIME

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-RUNTIME-00000 | RuntimeInformationUnclassified | info | Diagnóstico informativo no clasificado de runtime. |
| FLX-RUNTIME-00001 | RuntimeWarningUnclassified | warning | Advertencia no clasificada de runtime. |
| FLX-RUNTIME-00002 | RuntimeErrorUnclassified | error | Error no clasificado de runtime. |
| FLX-RUNTIME-00003 | RuntimeUnexpectedInternalError | error | Error interno inesperado de runtime. |
| FLX-RUNTIME-00004 | RuntimeDeprecatedFunctionality | warning | Funcionalidad obsoleta de runtime. |
| FLX-RUNTIME-00010 | EngineAlreadyRun | error | Una instancia de Engine ha recibido más de una ejecución. |
| FLX-RUNTIME-00011 | InvalidVideoOutputConfiguration | error | La configuración de salida de vídeo no es válida. |
| FLX-RUNTIME-00012 | WindowInitializationFailed | error | No se ha podido inicializar la ventana. |
| FLX-RUNTIME-00013 | RenderTargetInitializationFailed | error | No se ha podido inicializar el render target. |
| FLX-RUNTIME-00014 | RuntimeWorldLoadFailed | error | No se ha podido cargar el mundo runtime. |
| FLX-RUNTIME-00015 | RuntimeLoadSpawnLimitExceeded | error | La carga del runtime supero el limite de instanciacion. |

### BUILD

| Código | Identifier | Severity | Descripción |
| --- | --- | --- | --- |
| FLX-BUILD-00000 | BuildInformationUnclassified | info | Diagnóstico informativo no clasificado de build. |
| FLX-BUILD-00001 | BuildWarningUnclassified | warning | Advertencia no clasificada de build. |
| FLX-BUILD-00002 | BuildErrorUnclassified | error | Error no clasificado de build. |
| FLX-BUILD-00003 | BuildUnexpectedInternalError | error | Error interno inesperado de build. |
| FLX-BUILD-00004 | BuildDeprecatedFunctionality | warning | Funcionalidad obsoleta de build. |

## Nota de estado

Los códigos genéricos reservados por dominio conviven con códigos concretos
`00010+` cuando cada productor ya tiene una clasificación estable.