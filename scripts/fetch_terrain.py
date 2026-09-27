"""Download a small real USGS elevation sample and encode an Unreal heightmap.

No imagery is included: hillshade is only a topographic inspection preview.
"""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import urlopen

import numpy as np
import rasterio
from rasterio.warp import transform
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data" / "terrain"
SERVICE = "https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer"


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    # Horseshoe Bend, Arizona. UTM coordinates keep distances in metres.
    lon, lat = -111.510, 36.880
    east, north = transform("EPSG:4326", "EPSG:32612", [lon], [lat])
    size, spacing = 1009, 4.0
    # Pixel centres become Landscape vertices: 1008 intervals = 4032 metres.
    half = size * spacing / 2
    bbox = [east[0] - half, north[0] - half, east[0] + half, north[0] + half]
    params = dict(f="json", bbox=",".join(map(str, bbox)), bboxSR=32612,
                  imageSR=32612, size=f"{size},{size}", format="tiff",
                  pixelType="F32", interpolation="RSP_BilinearInterpolation",
                  renderingRule=json.dumps({"rasterFunction": "None"}))
    request_url = SERVICE + "/exportImage?" + urlencode(params)
    with urlopen(request_url, timeout=120) as response:
        export = json.load(response)
    if "error" in export or "href" not in export:
        raise RuntimeError(export)
    raw = OUT / "horseshoe_bend_dem.tif"
    with urlopen(export["href"], timeout=120) as response:
        raw.write_bytes(response.read())
    with rasterio.open(raw) as source:
        heights = source.read(1, masked=True)
        if source.count != 1 or heights.shape != (size, size):
            raise ValueError("Unexpected raster dimensions/bands")
        if np.ma.getmaskarray(heights).any() or not np.isfinite(heights).all():
            raise ValueError("Missing elevation samples; do not interpolate silently")
        if source.crs.to_epsg() != 32612:
            raise ValueError("Unexpected coordinate reference system")
        dx, dy = abs(source.transform.a), abs(source.transform.e)
        if not np.allclose([dx, dy], spacing, atol=0.001):
            raise ValueError("Unexpected spatial resolution")
        height = np.asarray(heights, dtype=np.float64)
        bounds = list(source.bounds)
    low, high = float(height.min()), float(height.max())
    if not (0 < high - low < 3000 and -500 < low < high < 5000):
        raise ValueError("Implausible Arizona elevation range")
    encoded = np.rint((height - low) / (high - low) * 65535).astype(np.uint16)
    Image.fromarray(encoded).save(OUT / "horseshoe_bend_height16.png")
    encoded.astype("<u2").tofile(OUT / "horseshoe_bend_height.r16")
    # UE Landscape height = (sample - 32768) / 128 * ZScale cm.
    z_scale = (high - low) * 100 * 128 / 65535
    origin_z = low * 100 + 256 * z_scale
    reconstructed = (encoded.astype(float) - 32768) / 128 * z_scale / 100 + origin_z / 100
    error = float(np.max(np.abs(reconstructed - height)))
    if error > (high - low) / 65535 / 2 + 1e-6:
        raise ValueError("Height encoding round-trip failed")
    gy, gx = np.gradient(height, dy, dx)
    light = np.array([-0.5, -0.5, 0.70710678])
    shade = (-gx * light[0] - gy * light[1] + light[2]) / np.sqrt(gx*gx + gy*gy + 1)
    shade = np.clip(0.25 + 0.75 * shade, 0, 1)
    Image.fromarray((shade * 255).astype(np.uint8)).save(OUT / "horseshoe_bend_hillshade.png")
    metadata = dict(
        name="Horseshoe Bend terrain feasibility sample", retrieved_utc=datetime.now(timezone.utc).isoformat(),
        source=SERVICE, request_url=request_url, attribution="USGS National Map 3D Elevation Program (3DEP)",
        usage_source="https://www.usgs.gov/3d-elevation-program/about-3dep-products-services",
        license_note="USGS states 3DEP products are free of charge and without use restrictions. Preserve attribution.",
        sha256=hashlib.sha256(raw.read_bytes()).hexdigest(), center_wgs84=[lon,lat],
        crs="EPSG:32612", raster_bounds_m=bounds, size=[size,size], sample_spacing_m=[dx,dy],
        landscape_extent_m=[(size-1)*dx,(size-1)*dy], elevation_min_m=low, elevation_max_m=high,
        relief_m=high-low, max_quantization_error_m=error,
        unreal=dict(section_size_quads=63, sections_per_component=1, components=[16,16],
                    scale_xyz=[dx*100,dy*100,z_scale], location_z_cm=origin_z,
                    orientation="Rows run north to south; columns west to east. Local +X=east, +Y=south.",
                    note="Z location retains absolute elevation; subtract a local datum for gameplay if desired."),
        limitations=["4 m is output sampling, not a claim of native source accuracy/resolution.",
                      "Elevation mosaic only: no aerial photo, material, vegetation, or overhangs.",
                      "Candidate area only; racing route and playability are not yet validated."])
    (OUT / "horseshoe_bend.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(json.dumps(metadata, indent=2))


if __name__ == "__main__":
    main()
