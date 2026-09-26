#!/usr/bin/env python3
"""Rebuild the checked-in catalog-only Noto font subsets.

Requires fonttools==4.66.0. Source files are downloaded separately from the
version-pinned upstream URLs in assets/fonts/CatalogSubsets/README.md. No
production build invokes this script: the ASI consumes the checked-in subset
TTFs directly.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
import zipfile

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont


ROOT = pathlib.Path(__file__).resolve().parents[2]
ASSET_DIR = ROOT / "assets" / "fonts" / "CatalogSubsets"
CONTRACT_PATH = ASSET_DIR / "glyph-contract.json"
ARABIC_MEMBER = "NotoSansArabic/unhinted/ttf/NotoSansArabic-Regular.ttf"

SOURCES = {
    "common": ("NotoSansMono-Regular.ttf",
        "74fd536351d0f30a73410e1bd223a0ebf763dd4808eb844aa2aed18c0d6e3c84"),
    "arabic": ("NotoSansArabic-v2.013.zip",
        "1301aceaea84c501cf2e6dcfb3182e2328c8eae5725817fcb239672bda7154f1"),
    "japanese": ("NotoSansJP-VF.ttf",
        "f4b373b226668ee33a6e54b02823dcd2d1209f17159f777421ae8c2275160369"),
    "korean": ("NotoSansKR-VF.ttf",
        "9e1d729e7e2b36f9ef439da102f8c134c10aabe46f1c843bf0aca5c043b86f76"),
    "chinese-simplified": ("NotoSansSC-VF.ttf",
        "d68bafcb48a2707749396aa12bbbd833cb70401f3a9a689fd2902c7e0d295964"),
    "chinese-traditional": ("NotoSansTC-VF.ttf",
        "ac091cc8cd19e848202afc8fe6d3809b4526c8fdbdb4be82da20c4f785949591"),
}

OUTPUTS = {
    "common": "NotoSansMono-Catalog.ttf",
    "arabic": "NotoSansArabic-Catalog.ttf",
    "japanese": "NotoSansJP-Catalog.ttf",
    "korean": "NotoSansKR-Catalog.ttf",
    "chinese-simplified": "NotoSansSC-Catalog.ttf",
    "chinese-traditional": "NotoSansTC-Catalog.ttf",
}


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_font(source_dir: pathlib.Path, profile: str) -> TTFont:
    filename, expected_hash = SOURCES[profile]
    source = source_dir / filename
    if not source.is_file() or sha256(source) != expected_hash:
        raise RuntimeError(f"missing or hash-mismatched source: {source}")
    if profile == "arabic":
        with zipfile.ZipFile(source) as archive:
            font_bytes = archive.read(ARABIC_MEMBER)
        if hashlib.sha256(font_bytes).hexdigest() != (
                "bd86ca02f087d7f3c3788ba458fb6b73744c7639ed276b8d870dba6def6c40d0"):
            raise RuntimeError("Arabic Regular TTF member hash mismatch")
        import io
        font = TTFont(io.BytesIO(font_bytes))
        font.recalcTimestamp = False
        return font
    font = TTFont(source)
    if profile in {"japanese", "korean", "chinese-simplified",
                   "chinese-traditional"}:
        font = instantiateVariableFont(font, {"wght": 400}, inplace=False)
    font.recalcTimestamp = False
    return font


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", type=pathlib.Path, default=ASSET_DIR)
    parser.add_argument("--update-contract", action="store_true",
        help="refresh registry display names and combined requirements before subsetting")
    parser.add_argument("--profiles", nargs="+", choices=tuple(SOURCES),
        help="regenerate only the named profiles; verify other checked-in subset cmaps")
    args = parser.parse_args()

    if getattr(__import__("fontTools"), "version", None) != "4.66.0":
        raise RuntimeError("use the pinned fontTools 4.66.0 environment")
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    if contract.get("format") != 1 or set(contract.get("profiles", {})) != set(SOURCES):
        raise RuntimeError("glyph contract profile set is invalid")

    registry_source = (ROOT / "src/localization/locale_registry.hpp").read_text(
        encoding="utf-8")
    descriptor_pattern = re.compile(
        r'LocaleDescriptor\s*\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*'
        r'"([^"]+)"\s*,\s*"([^"]+)"')
    descriptors = {code: (display_name, filename, profile)
        for display_name, code, filename, profile
        in descriptor_pattern.findall(registry_source)}
    registry_locale_profiles = {code: profile for code,
        (_, _, profile) in descriptors.items()}
    if args.update_contract:
        contract["localeProfiles"] = registry_locale_profiles
    if {code: (filename, profile) for code, (_, filename, profile)
            in descriptors.items()} != {code: (f"{code}.json", profile)
            for code, profile in contract["localeProfiles"].items()}:
        raise RuntimeError("locale registry profile assignments differ from glyph contract")

    required_by_locale = {}
    catalog_name_points = {}
    display_name_points = {}
    for code, (display_name, filename, profile) in descriptors.items():
        catalog = json.loads((ROOT / "locales" / filename).read_text(encoding="utf-8"))
        def values(node):
            if isinstance(node, str):
                yield node
            elif isinstance(node, dict):
                for value in node.values():
                    yield from values(value)
            elif isinstance(node, list):
                for value in node:
                    yield from values(value)
        catalog_required = {ord(char) for value in values(catalog) for char in value
                    if ord(char) >= 0x20}
        locked = {int(value, 16)
                  for value in contract["catalogRequiredCodepoints"].get(code, [])}
        if catalog_required != locked and not args.update_contract:
            raise RuntimeError(f"catalog codepoint inventory changed: {code}")
        display_required = {ord(char) for char in display_name if ord(char) >= 0x20}
        locked_display = {int(value, 16) for value in
            contract.get("displayNameRequiredCodepoints", {}).get(code, [])}
        if display_required != locked_display and not args.update_contract:
            raise RuntimeError(f"registry display-name inventory changed: {code}; "
                "rerun with --update-contract")
        required = catalog_required | display_required
        locked_full = {int(value, 16) for value in
            contract.get("requiredCodepoints", {}).get(code, [])}
        if required != locked_full and not args.update_contract:
            raise RuntimeError(f"full catalog-plus-metadata inventory changed: {code}")
        required_by_locale[code] = required
        catalog_name_points[code] = catalog_required
        display_name_points[code] = display_required

    if args.update_contract:
        contract["catalogRequiredCodepoints"] = {
            code: [f"{point:04X}" for point in sorted(points)]
            for code, points in catalog_name_points.items()}
        contract["displayNameRequiredCodepoints"] = {
            code: [f"{point:04X}" for point in sorted(points)]
            for code, points in display_name_points.items()}
        contract["requiredCodepoints"] = {
            code: [f"{point:04X}" for point in sorted(points)]
            for code, points in required_by_locale.items()}
        baseline = {code: {int(value, 16) for value in values}
            for code, values in contract["baselineCoveredCodepoints"].items()}
        profile_points = {profile: {int(value, 16)
            for value in data["codepoints"]}
            for profile, data in contract["profiles"].items()}
        for code, points in display_name_points.items():
            profile = contract["localeProfiles"][code]
            target = "common" if profile == "base" else profile
            already_covered = baseline.get(code, set()) if profile == "base" else set()
            profile_points[target].update(points - already_covered)
        for profile, points in profile_points.items():
            contract["profiles"][profile]["codepoints"] = [
                f"{point:04X}" for point in sorted(points)]
        for code, points in required_by_locale.items():
            profile = contract["localeProfiles"][code]
            subset_points = profile_points["common"] | profile_points.get(profile, set())
            contract["baselineCoveredCodepoints"][code] = [
                f"{point:04X}" for point in sorted(points - subset_points)]
        CONTRACT_PATH.write_text(json.dumps(contract, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8")

    args.output_dir.mkdir(parents=True, exist_ok=True)

    font_cmaps = {}

    regenerate_profiles = set(args.profiles or SOURCES)
    for profile in SOURCES:
        points = sorted(int(value, 16)
            for value in contract["profiles"][profile]["codepoints"])
        if not points or len(points) != len(set(points)):
            raise RuntimeError(f"empty or duplicate glyph contract: {profile}")
        output = args.output_dir / OUTPUTS[profile]
        if profile not in regenerate_profiles:
            if not output.is_file():
                raise RuntimeError(f"unchanged subset is missing: {output}")
            built = TTFont(output)
            built_cmap = set(built.getBestCmap())
            if built_cmap != set(points):
                raise RuntimeError(f"unchanged subset cmap differs from contract: {profile}")
            font_cmaps[profile] = built_cmap
            continue
        font = load_font(args.source_dir, profile)
        source_cmap = set(font.getBestCmap())
        missing = set(points) - source_cmap
        if missing:
            raise RuntimeError(f"{profile} source missing: " + ",".join(
                f"U+{point:04X}" for point in sorted(missing)))

        options = subset.Options()
        options.layout_features = []
        options.name_IDs = []
        options.name_legacy = False
        options.name_languages = []
        options.hinting = False
        options.recalc_timestamp = False
        options.canonical_order = True
        subsetter = subset.Subsetter(options=options)
        subsetter.populate(unicodes=points)
        subsetter.subset(font)
        font.save(output)

        built = TTFont(output)
        built_cmap = set(built.getBestCmap())
        if built_cmap != set(points):
            raise RuntimeError(f"subset cmap differs from locked contract: {profile}")
        font_cmaps[profile] = built_cmap
        print(f"{profile}: {output.stat().st_size} bytes, {len(built_cmap)} cmap, "
              f"sha256={sha256(output)}")

    common = {int(value, 16)
              for value in contract["profiles"]["common"]["codepoints"]}
    for code, required in required_by_locale.items():
        profile = contract["localeProfiles"][code]
        covered_by_subsets = common | font_cmaps.get(profile, set())
        baseline_remainder = {int(value, 16) for value in
            contract["baselineCoveredCodepoints"][code]}
        if required - (covered_by_subsets | baseline_remainder):
            raise RuntimeError(f"unaccounted catalog glyphs: {code}")
        if baseline_remainder != required - covered_by_subsets:
            raise RuntimeError(f"base-font remainder contract changed: {code}")
        if display_name_points[code] - (covered_by_subsets | baseline_remainder):
            raise RuntimeError(f"unaccounted locale display-name glyphs: {code}")
    all_required = set().union(*required_by_locale.values())
    print(f"catalog_plus_registry_metadata_coverage=100% locales={len(required_by_locale)} "
        f"unique_codepoints={len(all_required)} missing=0")
    if args.output_dir.resolve() == ASSET_DIR.resolve():
        for profile in regenerate_profiles:
            filename = OUTPUTS[profile]
            output = args.output_dir / filename
            contract["profiles"][profile]["bytes"] = output.stat().st_size
            contract["profiles"][profile]["sha256"] = sha256(output)
        CONTRACT_PATH.write_text(json.dumps(contract, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"subset generation failed: {error}", file=sys.stderr)
        raise
