# Third-Party Notices

Original v1.0+ project code is subject to the source-available terms in
LICENSE.md. The components below are not relicensed by that document.

## SafetyHook — Boost Software License 1.0

The vendored amalgamated SafetyHook sources in external/safetyhook/ are from
cursey/safetyhook and are identified as licensed under the Boost Software
License 1.0.

The following is the complete Boost Software License 1.0 text used for the
distributed SafetyHook component:

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated
by a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Reference: https://www.boost.org/LICENSE_1_0.txt

## Zydis and Zycore-C — MIT License

The bundled Zydis and amalgamated Zycore-C implementations in
external/safetyhook/Zydis.h are distributed under the MIT License. Their
vendored header retains the applicable permissions and copyright notices.

The MIT License applies to Zydis code and is not replaced by the Project
license.

The relevant MIT notice is:

The MIT License (MIT)

Original authors: Florian Bernd; Florian Bernd and Joel Hoener for Zycore-C

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Dear ImGui — MIT License

The overlay vendors a pinned subset of Dear ImGui v1.91.9b from
ocornut/imgui. The complete applicable license text is retained in
`external/imgui/LICENSE.txt`.

## ProggyVector Regular — MIT / Bitstream Vera License

The embedded Cyrillic font resource is ProggyVector Regular, supplied in
`assets/fonts/ProggyVector/ProggyVector-Regular.ttf`. Its complete upstream
license and copyright notices are retained alongside the font and embedded in
the ASI as `IDR_PROGGY_VECTOR_LICENSE`. The notice covers the ProggyVector,
Hack, DejaVu, and Bitstream Vera components as stated in that license file.

## Noto catalog glyph subsets — SIL Open Font License 1.1

The overlay embeds catalog-derived glyph subsets from Noto Sans Mono v2.014,
Noto Sans Arabic v2.013, and Noto Sans CJK v2.004 JP/KR/SC/TC. Their source
attributions and versions are recorded in
`assets/fonts/CatalogSubsets/README.md`; complete OFL text and attribution
notices are embedded as Windows resources in the ASI. The catalog-only
subset binaries, source hashes, generated hashes, exact Unicode contract and
reproduction script are retained under `assets/fonts/CatalogSubsets/` and
`tools/fonts/`.

## spdlog — MIT License

The vendored spdlog headers in external/spdlog/ are from gabime/spdlog and are
licensed under the MIT License. The complete text is retained in
external/spdlog/LICENSE.

The relevant MIT notice is:

The MIT License (MIT)

Copyright (c) 2016 - present, Gabi Melman and spdlog contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## fmt — MIT License

The bundled fmt implementation in
external/spdlog/include/spdlog/fmt/bundled/ is licensed under the MIT License
with the optional object-code exception stated in fmt.license.rst. The
complete text is retained in the repository.

The relevant fmt notice is:

Copyright (c) 2012 - present, Victor Zverovich and fmt contributors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Optional fmt object-code exception:

As an exception, if, as a result of compiling source code, portions of this
Software are embedded into a machine-executable object form of that source
code, those embedded portions may be redistributed in object form without
including the above copyright and permission notices.

## Historical provenance

Early historical versions used Lyall's MIT-licensed STALKER2Tweak as
reference/scaffolding. The current built production path no longer contains
identifiable Lyall-derived implementation. Historical MIT rights and
provenance are not altered by the v1.0+ Project license.

## External research tools

UE4SS, Ghidra, CUE4Parse, Serilog, Dumper-7, Ultimate ASI Loader, Oodle and
other external or research tools are not bundled in the production release
and are not relicensed or distributed by this repository.
