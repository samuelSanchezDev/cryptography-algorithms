# MD5

## 1. Propiedades
- **Nombre:** Message-Digest Algorithm 5.
- **Tipo:** Función hash.
- **Digest:** 128 bits.
- **Estructura:** Esquema de Merkle-Damgård.
    - **Tamaño de bloque:** 512 bits.
    - **Número de iteraciones:** 64 (4 rondas de 16 iteraciones).
- **Creador:** Ronald Rivest.
- **Publicación:** 1992 ([RFC 1321](https://www.rfc-editor.org/rfc/rfc1321)).

## 2. Descripción

**Anotaciones**
- Las palabras se componen de 32 bits (4 bytes).
- Little-endian: en una palabra de 32 bits, el byte menos significativo se almacena primero. Esta convención se aplica a las palabras de los bloques (2.1.2), al campo de longitud del relleno (2.1.1) y a la salida (2.3).
- El símbolo **X <<< n** indica rotar **X** a la izquierda **n** bits.
- Todas las sumas son módulo 2<sup>32</sup>.
- Los operadores `and`, `or`, `not` y `xor` actúan bit a bit sobre palabras de 32 bits.

El algoritmo MD5 se compone de tres etapas:
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
	> len(mensaje) + 1 + nº_ceros ≡ 448 (mod 512), longitudes en bits
3. La longitud del mensaje original en bits, módulo 2<sup>64</sup>, codificada en 64 bits.


El bit `0b1` se añade siempre, incluso si la longitud original ya es congruente con 448 módulo 512 bits.

En los 8 bytes (64 bits) finales se almacena la longitud en bits del mensaje original (antes del relleno). Si es mayor o igual que 2<sup>64</sup> bits, solo se usan los 64 bits de menor peso. Se escribe en little-endian.

Ejemplo con una longitud mayor que 2<sup>64</sup> bits.

```txt
┌────────────────────────────────────────────────────────────────────────────┐
│ Longitud = 0x87EFCDAB8967452301                                            │
│ ┌0─────┬1─────┬2─────┬3─────┬4─────┬5─────┬6─────┬7─────┐                  │
│ │ 0x01 │ 0x23 │ 0x45 │ 0x67 │ 0x89 │ 0xAB │ 0xCD │ 0xEF │ 0x87 se descarta │
│ └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘                  │
└────────────────────────────────────────────────────────────────────────────┘
```

#### 2.1.2 Bloques y palabras
El mensaje con el relleno se divide en bloques consecutivos de 64 bytes. Cada bloque **M** se compone de **16 palabras** M<sub>0</sub>…M<sub>15</sub> de 32 bits. Si los bytes de un bloque son b<sub>0</sub>…b<sub>63</sub>, cada palabra M<sub>j</sub> se forma con 4 bytes consecutivos, el primero como byte menos significativo:

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

Al empezar la compresión, el buffer de entrada se copia en **A**, **B**, **C** y **D** (*buffer de trabajo*). Para el primer bloque, el valor se toma del vector de inicialización (*initialization vector*, **IV**):
- **A0** = `0x67452301`
- **B0** = `0xEFCDAB89`
- **C0** = `0x98BADCFE`
- **D0** = `0x10325476`

La función consiste en 4 rondas (*t* = 1, 2, 3, 4) de 16 iteraciones cada una, 64 iteraciones en total. Las iteraciones se numeran de forma global (*i* = 00…63). Cada iteración mezcla una palabra del **bloque M** con el **buffer de trabajo**, quedando el bloque **M** mezclado por completo al final de cada ronda.

Cada ronda (*t*) tiene asignadas dos funciones, una función **g<sub>t</sub>(i)** que indica qué palabra del bloque **M** se procesa en la iteración *i*, y una función **F<sub>t</sub>** que mezcla las palabras **B**, **C** y **D** del buffer de trabajo.
- **Ronda 1**. Iteraciones 00-15.
	- **g<sub>1</sub>(i)** = `i mod 16`
	- **F<sub>1</sub> = F(B, C, D)** = `(B and C) or ((not B) and D)`
- **Ronda 2**. Iteraciones 16-31.
	- **g<sub>2</sub>(i)** = `(5 × i + 1) mod 16`
	- **F<sub>2</sub> = G(B, C, D)** = `(B and D) or (C and (not D))`
- **Ronda 3**. Iteraciones 32-47.
	- **g<sub>3</sub>(i)** = `(3 × i + 5) mod 16`
	- **F<sub>3</sub> = H(B, C, D)** = `B xor C xor D`
- **Ronda 4**. Iteraciones 48-63.
	- **g<sub>4</sub>(i)** = `(7 × i) mod 16`
	- **F<sub>4</sub> = I(B, C, D)** = `C xor (B or (not D))`

Cada iteración (*i*) tiene asignadas una constante **K<sub>i</sub>** (*tabla 2.2.1*) y una cantidad **s<sub>i</sub>** de bits (*tabla 2.2.2*) para una rotación a la izquierda.

Cada iteración consiste en las siguientes 4 asignaciones simultáneas (todas usan los valores que tenían A, B, C y D antes de la iteración).
- A la palabra **A** se le asigna la palabra **D**.
- A la palabra **B** se le asigna **((A + F<sub>t</sub>(B, C, D) + M[g<sub>t</sub>(i)] + K<sub>i</sub>) <<< s<sub>i</sub>) + B**.
- A la palabra **C** se le asigna la palabra **B**.
- A la palabra **D** se le asigna la palabra **C**.

```txt
┌──────────────────────────────────────────────────────────────┐
│        ┌────────────┬────────────┬────────────┬────────────┐ │
│        │     A      │     B      │     C      │     D      │ │
│        └████████████┴████████████┴████████████┴████████████┘ │
│              ║            ║            ║            ║        │
│            ╔═╩═╗ ╔═══╦════╣            ║            ║        │
│            ║ + ╠═╣ F ╠════║════════════╣            ║        │
│            ╚═╦═╝ ╚══t╩════║════════════║════════════╣        │
│            ╔═╩═╗          ║            ║            ║        │
│ M[g_t(i)] ═╣ + ║          ║            ║            ║        │
│            ╚═╦═╝          ║            ║            ║        │
│            ╔═╩═╗          ║            ║            ║        │
│       K_i ═╣ + ║          ║            ║            ║        │
│            ╚═╦═╝          ║            ║            ║        │
│          ╔═══╩═══╗        ║            ║            ║        │
│          ║  <<<  ║        ║            ║            ║        │
│          ╚═══╦s_i╝        ║            ║            ║        │
│            ╔═╩═╗          ║            ║            ╚═════╗  │
│            ║ + ╠══════════╣            ╚════════════╗     ║  │
│            ╚═╦═╝          ╚════════════╗            ║     ║  │
│              ╚════════════╗            ║            ║     ║  │
│                           ║            ║            ║     ║  │
│              ╔════════════║════════════║════════════║═════╝  │
│              ║            ║            ║            ║        │
│        ┌████████████┬████████████┬████████████┬████████████┐ │
│        │     A      │     B      │     C      │     D      │ │
│        └────────────┴────────────┴────────────┴────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

Al terminar las 64 iteraciones, el buffer de trabajo se suma al de entrada:
- **A0 = A0 + A**
- **B0 = B0 + B**
- **C0 = C0 + C**
- **D0 = D0 + D**

El resultado (**A0**, **B0**, **C0**, **D0**) es el buffer de entrada de la función de compresión del bloque siguiente o, si era el último, el valor que recibe el cálculo de la salida.

#### 2.2.1 Valores de K<sub>i</sub>

| Iteraciones |              |              |              |              |              |              |              |              |
| :---------: | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ |
|   00...07   | `0xD76AA478` | `0xE8C7B756` | `0x242070DB` | `0xC1BDCEEE` | `0xF57C0FAF` | `0x4787C62A` | `0xA8304613` | `0xFD469501` |
|   08...15   | `0x698098D8` | `0x8B44F7AF` | `0xFFFF5BB1` | `0x895CD7BE` | `0x6B901122` | `0xFD987193` | `0xA679438E` | `0x49B40821` |
|   16...23   | `0xF61E2562` | `0xC040B340` | `0x265E5A51` | `0xE9B6C7AA` | `0xD62F105D` | `0x02441453` | `0xD8A1E681` | `0xE7D3FBC8` |
|   24...31   | `0x21E1CDE6` | `0xC33707D6` | `0xF4D50D87` | `0x455A14ED` | `0xA9E3E905` | `0xFCEFA3F8` | `0x676F02D9` | `0x8D2A4C8A` |
|   32...39   | `0xFFFA3942` | `0x8771F681` | `0x6D9D6122` | `0xFDE5380C` | `0xA4BEEA44` | `0x4BDECFA9` | `0xF6BB4B60` | `0xBEBFBC70` |
|   40...47   | `0x289B7EC6` | `0xEAA127FA` | `0xD4EF3085` | `0x04881D05` | `0xD9D4D039` | `0xE6DB99E5` | `0x1FA27CF8` | `0xC4AC5665` |
|   48...55   | `0xF4292244` | `0x432AFF97` | `0xAB9423A7` | `0xFC93A039` | `0x655B59C3` | `0x8F0CCC92` | `0xFFEFF47D` | `0x85845DD1` |
|   56...63   | `0x6FA87E4F` | `0xFE2CE6E0` | `0xA3014314` | `0x4E0811A1` | `0xF7537E82` | `0xBD3AF235` | `0x2AD7D2BB` | `0xEB86D391` |

#### 2.2.2 Valores de s<sub>i</sub> (rotación de bits)

| Ronda \ i mod 16 | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
| :--------------- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| 1                | 07 | 12 | 17 | 22 | 07 | 12 | 17 | 22 | 07 | 12 | 17 | 22 | 07 | 12 | 17 | 22 |
| 2                | 05 | 09 | 14 | 20 | 05 | 09 | 14 | 20 | 05 | 09 | 14 | 20 | 05 | 09 | 14 | 20 |
| 3                | 04 | 11 | 16 | 23 | 04 | 11 | 16 | 23 | 04 | 11 | 16 | 23 | 04 | 11 | 16 | 23 |
| 4                | 06 | 10 | 15 | 21 | 06 | 10 | 15 | 21 | 06 | 10 | 15 | 21 | 06 | 10 | 15 | 21 |


### 2.3 Cálculo de la salida (Finalization)
La salida se calcula concatenando las palabras **A0**, **B0**, **C0** y **D0** resultantes de la última compresión, empezando por el byte de menor peso de **A0** y acabando por el byte de mayor peso de **D0**. El siguiente es un ejemplo de MD5("").

```txt
┌───────────────┬───────────────────────────────────┐
│ A0 0xD98C1DD4 │ digest = {0xD4, 0x1D, 0x8C, 0xD9, │
│ B0 0x04B2008F │           0x8F, 0x00, 0xB2, 0x04, │
│ C0 0x980980E9 │           0xE9, 0x80, 0x09, 0x98, │
│ D0 0x7E42F8EC │           0xEC, 0xF8, 0x42, 0x7E} │
└───────────────┴───────────────────────────────────┘
```
