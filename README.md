Roland SR-JV80 / SRX Scrambler
==============================

Roland's old ROM modules like SR-JV80 and SRX boards use a scrambling scheme to
make the data unreadable. This is implemented by simply swapping address and
data lines on the ASIC.

The tools in this repository allow scrambling and descrambling of ROM data, so
you can either look at the contents of a dumped ROM or scramble your own ROM
for use in a physical sound module.

Descrambling
------------

Address and data wires are swapped, this is the relation:

| Address line (internal) | Address line (ROM, SRX) | Address line (ROM, SR-JV80) |
|:-----------------------:|:-----------------------:|:---------------------------:|
|            0            |             0           |               2             |
|            1            |             4           |               0             |
|            2            |             2           |               3             |
|            3            |             3           |               4             |
|            4            |             1           |               1             |
|            5            |            13           |               9             |
|            6            |             7           |              13             |
|            7            |            12           |              10             |
|            8            |             5           |              18             |
|            9            |            10           |              17             |
|           10            |            16           |               6             |
|           11            |             9           |              15             |
|           12            |             6           |              11             |
|           13            |             8           |              16             |
|           14            |            14           |               8             |
|           15            |            17           |               5             |
|           16            |            11           |              12             |
|           17            |            15           |               7             |
|           18            |            18           |              14             |
|           19            |            19           |              19             |
|           20            |            20           |              20             |
|           21            |            21           |              21             |
|           22            |            22           |              22             |
|           23            |            23           |              23             |

| Data line (internal) | Data line (ROM) |
|:--------------------:|:---------------:|
|            0         |         1       |
|            1         |         7       |
|            2         |         0       |
|            3         |         6       |
|            4         |         2       |
|            5         |         3       |
|            6         |         5       |
|            7         |         4       |
|            8         |         9       |
|            9         |        15       |
|           10         |         8       |
|           11         |        14       |
|           12         |        10       |
|           13         |        11       |
|           14         |        13       |
|           15         |        12       |

ROM Identifiers (scrambled)
---------------------------

Every scrambled ROM starts with 32 bytes of identifying information.

| Bytes 0x00 – 0x0F  | ROM Type |
|--------------------|----------|
| `Roland JV80 O°S¬` | SR-JV80  |
| `Roland SRX  O°X§` | SRX      |
| `JP-800          ` | JD-800   |

Bytes 0x10 to 0x1F contain an ASCII identifier of the ROM sound set.
