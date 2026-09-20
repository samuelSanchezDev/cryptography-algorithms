# MD4

## 1. Propiedades
- **Nombre:** Message-Digest Algorithm 4.
- **Tipo:** Función hash.
- **Digest:** 128 bits.
- **Estructura:** Esquema de Merkle-Damgård.
    - **Tamaño de bloque:** 512 bits.
    - **Número de iteraciones:** 48 (3 rondas de 16 iteraciones).
- **Creador:** Ronald Rivest.
- **Publicación original:** 1990 ([RFC 1186](https://www.rfc-editor.org/rfc/rfc1186)).

## 2. Descripción

**Anotaciones**
- Las palabras se componen de **32 bits** (4 bytes).
- Little-endian: en una palabra de 32 bits, el byte menos significativo se almacena primero. Esta convención se aplica a las palabras de los bloques (2.1.2), al campo de longitud del relleno (2.1.1) y a la salida (2.3).
- El símbolo **X <<< i** indica rotar **X** a la izquierda **i** bits.
- Todas las sumas son módulo 2<sup>32</sup>.
- Los operadores `and`, `or`, `not` y `xor` actúan bit a bit sobre palabras de 32 bits.

El algoritmo MD4 se compone de tres etapas:
1. **Preprocesamiento del mensaje.** El mensaje se rellena y se divide en bloques de 64 bytes.
2. **Función de compresión (CF).** Los bloques son procesados secuencialmente. Cada bloque se procesa con el resultado de la compresión anterior.
3. **Cálculo de la salida (Finalization).** Al resultado de la última función de compresión se le da el formato adecuado.

El siguiente es un esquema del proceso.
```txt
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│        ┌─────────────────────────── ─ ─ ─ ─ ───┬─────────┐                              │
│        │ MESSAGE                               │ PADDING │                              │
│        ├███████████████████████████ █ █ █ █ ███┴█████████┤                              │
│        .                                                 .                              │
│        .                                                 .                              │
│        ┌─────────────┬─────────────┬─ ─ ─ ─┬─────────────┐                              │
│        │  Block #1   │  Block #2   │       │  Block  #N  │                              │
│        └█████████████┴█████████████┴─ ─ ─ ─┴█████████████┘                              │
│          ╚══╦════╗      ╚══╦════╗              ╚═════╦════╗  ╔══════════════╗   Digest  │
│ (IV)        ║ CF ╠═╗       ║ CF ╠═╗                  ║ CF ╠══╣ Finalization ╠═ ■■■■■■■■ │
│ ■■■■■■■■ ═══╩════╝ ╚═══════╩════╝ ╚═ ═ ═ ═ ═ ═ ═ ═ ══╩════╝  ╚══════════════╝           │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Preprocesamiento del mensaje

#### 2.1.1 Relleno (padding)
El relleno hace que la longitud total del mensaje sea múltiplo de 64 bytes (512 bits). Consta de tres partes, que se concatenan al mensaje original en este orden:
1. Un bit `0b1`.
2. Tantos bits `0b0` como hagan falta para que la longitud hasta este punto sea congruente con 448 módulo 512 bits (56 módulo 64 bytes).
3. La longitud del mensaje original, codificada en 64 bits.

> len(mensaje) + 1 + nº_ceros ≡ 448 (mod 512), longitudes en bits

El bit `0b1` se añade siempre, incluso si la longitud original ya es congruente con 448 módulo 512 bits.

En los 8 bytes (64 bits) finales se almacena la longitud en bits del mensaje original (antes del relleno), módulo 2<sup>64</sup>: si es mayor o igual que 2<sup>64</sup> bits, solo se usan los 64 bits de menor peso. Se escribe en little-endian, por lo que ocupa las palabras M<sub>14</sub> (32 bits de menor peso) y M<sub>15</sub> (32 bits de mayor peso) del último bloque (ver 2.1.2).

Ejemplo con una longitud mayor que 2<sup>64</sup> bits.

```txt
┌────────────────────────────────────────────────────────────────────────────┐
│ Longitud = 0x87EFCDAB8967452301                                            │
│ ┌0─────┬1─────┬2─────┬3─────┬4─────┬5─────┬6─────┬7─────┐                  │
│ │ 0x01 │ 0x23 │ 0x45 │ 0x67 │ 0x89 │ 0xAB │ 0xCD │ 0xEF │ 0x87 se descarta │
│ └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘                  │
│ M14 = 0x67452301   M15 = 0xEFCDAB89                                        │
└────────────────────────────────────────────────────────────────────────────┘
```

#### 2.1.2 Bloques y palabras
El mensaje con el relleno se divide en bloques consecutivos de 64 bytes. Cada bloque **M** se compone de **16 palabras** M<sub>0</sub>…M<sub>15</sub>. Si los bytes de un bloque son b<sub>0</sub>…b<sub>63</sub>, cada palabra M<sub>j</sub> se forma con 4 bytes consecutivos, el primero como byte menos significativo:

```txt
┌─────────────────────────────────────────────────────────────────┐
│ ┌0─────┬1─────┬2─────┬3─────┬4─────┬5─────┬6─────┬7─────┬8── ─  │
│ │ 0x01 │ 0x23 │ 0x45 │ 0x67 │ 0x89 │ 0xAB │ 0xCD │ 0xEF │ ...   │
│ └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴─── ─  │
│  M0 = 0x67452301   M1 = 0xEFCDAB89   ...                        │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Función de compresión (CF)
Los argumentos de la función son:
- Un **bloque M** del mensaje. Tiene un tamaño de **16 palabras** (**512 bits**).
- Un **buffer de entrada** con el resultado de la función de compresión anterior. Tiene un tamaño de **4 palabras** (**128 bits**), que se identifican con **A0**, **B0**, **C0** y **D0**.

Al empezar la compresión, el buffer de entrada se copia en **A**, **B**, **C** y **D** (*buffer de trabajo*), que son las palabras que modifican las iteraciones. Para el primer bloque, (A0, B0, C0, D0) es el vector de inicialización (*initialization vector*, **IV**):
- **A0** = `0x67452301`
- **B0** = `0xEFCDAB89`
- **C0** = `0x98BADCFE`
- **D0** = `0x10325476`

Todas las operaciones son módulo 2<sup>32</sup>.

La función consiste en 3 rondas (*t* = 1, 2, 3) de 16 iteraciones cada una, 48 iteraciones en total. Las iteraciones se numeran de forma global (*i* = 00…47). Cada iteración mezcla una palabra del **bloque M** con el **buffer de trabajo**, quedando el bloque **M** mezclado por completo al final de cada ronda.

Cada ronda (*t*) tiene asignada una constante **K<sub>t</sub>** y una función **F<sub>t</sub>** que mezcla las palabras **B**, **C** y **D** del buffer de trabajo.
- **Ronda 1**. Iteraciones 00-15.
	- **K<sub>1</sub>** = `0x00000000`
	- **F<sub>1</sub> = F(B, C, D)** = (B and C) or ((not B) and D)
- **Ronda 2**. Iteraciones 16-31.
	- **K<sub>2</sub>** = `0x5A827999`
	- **F<sub>2</sub> = G(B, C, D)** = (B and C) or (B and D) or (C and D)
- **Ronda 3**. Iteraciones 32-47.
	- **K<sub>3</sub>** = `0x6ED9EBA1`
	- **F<sub>3</sub> = H(B, C, D)** = B xor C xor D

Cada iteración (*i*) tiene asignado un índice **g<sub>i</sub>** que indica qué palabra del bloque (**M<sub>g<sub>i</sub></sub>**) se procesa y una cantidad **s<sub>i</sub>** de bits para una rotación a la izquierda. Los valores se dan en las tablas de más abajo.

Cada iteración consiste en las siguientes 4 asignaciones simultáneas (todas usan los valores que tenían A, B, C y D antes de la iteración).
- A la palabra **A** se le asigna la palabra **D**.
- A la palabra **B** se le asigna **(A + F<sub>t</sub>(B, C, D) + M<sub>g<sub>i</sub></sub> + K<sub>t</sub>) <<< s<sub>i</sub>**.
- A la palabra **C** se le asigna la palabra **B**.
- A la palabra **D** se le asigna la palabra **C**.

```txt
┌───────────────────────────────────────────────────────────┐
│     ┌────────────┬────────────┬────────────┬────────────┐ │
│     │     A      │     B      │     C      │     D      │ │
│     └████████████┴████████████┴████████████┴████████████┘ │
│           ║            ║            ║            ║        │
│         ╔═╩═╗ ╔═══╦════╣            ║            ║        │
│         ║ + ╠═╣ F ╠════║════════════╣            ║        │
│         ╚═╦═╝ ╚══t╩════║════════════║════════════╣        │
│         ╔═╩═╗          ║            ║            ║        │
│ M[g_i] ═╣ + ║          ║            ║            ║        │
│         ╚═╦═╝          ║            ║            ║        │
│         ╔═╩═╗          ║            ║            ║        │
│    K_t ═╣ + ║          ║            ║            ║        │
│         ╚═╦═╝          ║            ║            ║        │
│       ╔═══╩═══╗        ║            ║            ╚═════╗  │
│       ║  <<<  ║        ║            ╚════════════╗     ║  │
│       ╚═══╦s_i╝        ╚════════════╗            ║     ║  │
│           ╚════════════╗            ║            ║     ║  │
│           ╔════════════║════════════║════════════║═════╝  │
│           ║            ║            ║            ║        │
│     ┌████████████┬████████████┬████████████┬████████████┐ │
│     │     A      │     B      │     C      │     D      │ │
│     └────────────┴────────────┴────────────┴────────────┘ │
└───────────────────────────────────────────────────────────┘
```

Al terminar las 48 iteraciones, el buffer de trabajo se suma al de entrada:
- **A0 = A0 + A**
- **B0 = B0 + B**
- **C0 = C0 + C**
- **D0 = D0 + D**

El resultado (A0, B0, C0, D0) es el buffer de entrada de la función de compresión del bloque siguiente o, si era el último, el valor que recibe el cálculo de la salida.

#### 2.2.1 Valores de g<sub>i</sub>

| Ronda \ i mod 16 | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
| :--------------- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| 1                | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
| 2                | 00 | 04 | 08 | 12 | 01 | 05 | 09 | 13 | 02 | 06 | 10 | 14 | 03 | 07 | 11 | 15 |
| 3                | 00 | 08 | 04 | 12 | 02 | 10 | 06 | 14 | 01 | 09 | 05 | 13 | 03 | 11 | 07 | 15 |

#### 2.2.2 Valores de s<sub>i</sub> (rotación de bits)

| Ronda \ i mod 16 | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
| :--------------- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| 1                | 03 | 07 | 11 | 19 | 03 | 07 | 11 | 19 | 03 | 07 | 11 | 19 | 03 | 07 | 11 | 19 |
| 2                | 03 | 05 | 09 | 13 | 03 | 05 | 09 | 13 | 03 | 05 | 09 | 13 | 03 | 05 | 09 | 13 |
| 3                | 03 | 09 | 11 | 15 | 03 | 09 | 11 | 15 | 03 | 09 | 11 | 15 | 03 | 09 | 11 | 15 |

### 2.3 Cálculo de la salida (Finalization)
La salida se calcula concatenando las palabras A0, B0, C0 y D0 resultantes de la última compresión, empezando por el byte de menor peso de A0 y acabando por el byte de mayor peso de D0. El siguiente es un ejemplo del formato, que usa los valores del IV y no es el digest de ningún mensaje real.

```txt
┌───────────────┬───────────────────────────────────┐
│ A0 0x67452301 │ digest = {0x01, 0x23, 0x45, 0x67, │
│ B0 0xEFCDAB89 │           0x89, 0xAB, 0xCD, 0xEF, │
│ C0 0x98BADCFE │           0xFE, 0xDC, 0xBA, 0x98, │
│ D0 0x10325476 │           0x76, 0x54, 0x32, 0x10} │
└───────────────┴───────────────────────────────────┘
```
