# FSBL

FSBL is First Stage Boot Loader.
Act as ATF BL2.

# License

- Project license: BSD-3-Clause. Unless stated otherwise at file level, original code authored by Sophgo Technologies in this repository is licensed under BSD-3-Clause (see LICENSE at repository root).

- Upstream attributions and Third-party licenses:

    - Portions derived from ARM Trusted Firmware and related sources
        Copyright (c) 2013–2017, ARM Limited and Contributors
        License: BSD-3-Clause

    - Additional components from Arm Limited
        2012–2020: MIT-licensed files (SPDX: MIT)
        2006–2015: Apache-2.0-licensed files (SPDX: Apache-2.0)
 
    - .clang-format
       SPDX: GPL-2.0

    - lib/cli_simple/; include/drivers/usb/
        SPDX: GPL-2.0+

    - lib/crc/crc16.c
        SPDX: eCos-2.0

    - lib/libtommath/
        SPDX: Unlicense

- Compliance notes

    - File-level SPDX headers are authoritative for each file’s license.
    - Source distributions: keep this repository’s LICENSE, the third-party license texts, and all file headers.
    - No binary releases are provided by this project. If you redistribute binaries built from this source, ensure you comply with the applicable licenses. In particular, for any GPL-2.0/GPL-2.0+ files you distribute or link, provide the corresponding source and license texts as required by the GPL. Apache-2.0-licensed files are present but we do not include separate Apache NOTICE content (no upstream NOTICE required in this tree per current inventory).
