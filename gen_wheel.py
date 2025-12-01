#!/usr/bin/env python3
"""
gen_wheel.py  —  Parametric wheel generator for Chrono/DEM runs
- Reads a JSON config (radius, width, grouser + inner-geometry params, material rho).
- Builds a surface mesh (OBJ/STL/PLY) using trimesh.
- Outer geometry: ring shell + plate grousers.
- Inner geometry: hub ring + radial spokes (trimesh boxes).
- Runs mesh QA/repair.
- Emits a wheel manifest (JSON) with derived fields (effective radius, est. mass).

Minimal usage:
  python gen_wheel.py --config wheel.json --outdir meshes/ --tag MR

Dependencies (suggested):
  pip install trimesh shapely
"""

import argparse, json, math, sys
from pathlib import Path

import trimesh
from trimesh.transformations import rotation_matrix, translation_matrix

# ---------------- Geometry helpers ----------------

def _cylinder(radius: float, height: float, sections: int = 128) -> trimesh.Trimesh:
    """Closed solid cylinder centered on origin, z axis is height/width."""
    return trimesh.creation.cylinder(radius=radius, height=height, sections=sections)

def _ring(r_out: float, thickness: float, height: float, sections: int = 128) -> trimesh.Trimesh:
    """
    Thin ring shell (annular cylinder) centered on origin, z axis is height.
      - r_out: outer radius
      - thickness: radial thickness of shell (r_out - r_in)
    """
    r_in = max(0.0, r_out - thickness)
    if r_in <= 0.0:
        return _cylinder(radius=r_out, height=height, sections=sections)
    return trimesh.creation.annulus(r_min=r_in, r_max=r_out, height=height, sections=sections)

def _box(size_xyz) -> trimesh.Trimesh:
    return trimesh.creation.box(extents=size_xyz)

def place_grouser_plate(
    R_out: float,
    g_tan_thick: float,
    g_height: float,
    g_axial_width: float,
    theta: float,
) -> trimesh.Trimesh:
    """
    Create one thin plate grouser and place it on wheel circumference at angle theta.

    Local extents:
      - x: radial height (0 → g_height, aligned with wheel diameter)
      - y: tangential thickness (small)
      - z: axial width (almost full wheel width)

    After transforms:
      - inner face of grouser is on radius R_out
      - outer tip is at radius R_out + g_height
    """

    # Plate extents: [radial (x), tangential (y), axial (z)]
    plate_local = _box([g_height, g_tan_thick, g_axial_width])

    # Shift so that the inner face is at x = 0 and the plate extends 0 → g_height
    # and is centered across z
    T0 = translation_matrix([g_height / 2.0, 0.0, 0.0])
    plate_local.apply_transform(T0)

    # Rotate so x-axis becomes radial at angle theta
    # (after this, x is radial, y is tangential)
    Rz = rotation_matrix(theta, [0, 0, 1])
    plate_local.apply_transform(Rz)

    # Translate so the inner face sits on the shell at radius R_out.
    # With the above setup, the inner face is at local x = -g_height/2,
    # but after T0 it moved to x in [0, g_height].
    # So translating by R_out puts that inner face at radius R_out.
    T1 = translation_matrix([R_out * math.cos(theta), R_out * math.sin(theta), 0.0])
    plate_local.apply_transform(T1)

    return plate_local


def create_spoke(
    hub_radius: float,
    R_out: float,
    tread_thickness: float,
    spoke_tangential_width: float,
    spoke_thickness: float,
    theta: float,
    z_offset: float,
) -> trimesh.Trimesh:
    """
    Create a single radial spoke between hub and inner shell, rotated to angle theta.
      - hub_radius: hub outer radius
      - R_out: wheel outer radius
      - tread_thickness: radial shell thickness
      - spoke_tangential_width: width in tangential dir (x)
      - spoke_thickness: thickness in axial dir (z)
      - z_offset: axial position of spoke center
    """
    r_inner = hub_radius
    r_outer = R_out - tread_thickness
    if r_outer <= r_inner:
        return None

    radial_length = r_outer - r_inner

    # Box extents: [tangent (x), radial (y), axial (z)]
    box_local = _box([spoke_tangential_width, radial_length, spoke_thickness])

    # Place radial extent from r_inner to r_outer (along +y), centered at z = z_offset
    center_y = r_inner + radial_length / 2.0
    T0 = translation_matrix([0.0, center_y, z_offset])
    box_local.apply_transform(T0)

    # Rotate around z to the spoke angle
    Rz = rotation_matrix(theta, [0, 0, 1])
    box_local.apply_transform(Rz)

    return box_local

# ---------------- Wheel builder ----------------

def build_wheel_mesh(cfg: dict) -> trimesh.Trimesh:
    """
    Build the wheel mesh:
      - outer tread ring (shell at outer_radius with thickness t_tread_m)
      - optional inner hub ring (hub_radius_m, hub_wall_thickness_m, hub_width_m, hub_z_offset_m)
      - optional radial spokes (spoke_count, widths, z_offset)
      - thin-plate grousers standing on the outer shell
    """
    R_out   = float(cfg["outer_radius"])
    width   = float(cfg["width"])
    segs    = int(cfg.get("circle_sections", 128))

    tread_thickness = float(cfg.get("t_tread_m", 0.006))

    # --- Outer tread as a ring shell, centered at z = 0 ---
    tread = _ring(r_out=R_out, thickness=tread_thickness, height=width, sections=segs)
    parts = [tread]

    # --- Inner hub ring: width + axial offset are configurable ---
    hub_radius = float(cfg.get("hub_radius_m", 0.0))
    hub_wall   = float(cfg.get("hub_wall_thickness_m", tread_thickness))
    hub_width  = float(cfg.get("hub_width_m", 0.5 * width))  # narrower than wheel by default
    hub_z_off  = float(cfg.get("hub_z_offset_m", 0.0))       # axial center position

    hub = None
    if hub_radius > 0.0 and hub_wall > 0.0:
        hub = _ring(r_out=hub_radius, thickness=hub_wall, height=hub_width, sections=segs)
        # Move hub so that its center is at z = hub_z_off
        T_hub = translation_matrix([0.0, 0.0, hub_z_off])
        hub.apply_transform(T_hub)
        parts.append(hub)

    # --- Spokes between hub and inner shell ---
    spoke_count = int(cfg.get("spoke_count", 0))
    spoke_root_width = float(cfg.get("spoke_root_width_m", 0.5 * hub_radius if hub_radius > 0 else 0.0))
    spoke_thickness  = float(cfg.get("spoke_thickness_m", 0.3 * width))
    spoke_z_off      = float(cfg.get("spoke_z_offset_m", hub_z_off))  # follows hub by default

    if hub is not None and spoke_count > 0 and spoke_root_width > 0.0:
        for k in range(spoke_count):
            theta = 2.0 * math.pi * k / spoke_count
            spoke = create_spoke(
                hub_radius=hub_radius,
                R_out=R_out,
                tread_thickness=tread_thickness,
                spoke_tangential_width=spoke_root_width,
                spoke_thickness=spoke_thickness,
                theta=theta,
                z_offset=spoke_z_off,
            )
            if spoke is not None:
                parts.append(spoke)

    # --- Plate grousers, standing off the ring ---
    gcount    = int(cfg["grouser_number"])
    g_height  = float(cfg["grouser_height"])

    pitch     = 2.0 * math.pi * R_out / max(gcount, 1)
    coverage  = float(cfg.get("grouser_coverage_ratio", 0.25))

    # Plate thickness in tangential direction
    g_tan_thick  = float(cfg.get("grouser_tan_thickness_m", coverage * pitch))
    # Axial width: almost full wheel width by default
    g_axial_width = float(cfg.get("grouser_axial_width_m", 0.9 * width))

    for k in range(gcount):
        theta = 2.0 * math.pi * k / gcount
        plate = place_grouser_plate(
            R_out=R_out,
            g_tan_thick=g_tan_thick,
            g_height=g_height,
            g_axial_width=g_axial_width,
            theta=theta,
        )
        parts.append(plate)

    # Boolean union if requested
    if cfg.get("boolean_union", False):
        try:
            union = trimesh.boolean.union(parts, engine=cfg.get("boolean_engine", None))
            if union is not None:
                return union
        except Exception as e:
            print(f"[w] Boolean union failed ({e}); using concatenation instead.", file=sys.stderr)

    # Fallback: concatenated mesh (fine for DEM collisions)
    return trimesh.util.concatenate(parts)

# ---------------- QA / Repair ----------------

def qa_and_repair(mesh: trimesh.Trimesh, aggressive: bool = False) -> trimesh.Trimesh:
    m = mesh.copy()
    try:
        m.remove_unreferenced_vertices()
        m.remove_degenerate_faces()
        trimesh.repair.fix_normals(m)
        if aggressive:
            trimesh.repair.fill_holes(m)
            trimesh.repair.fix_inversion(m)
            trimesh.repair.fix_winding(m)
    except Exception as e:
        print(f"[w] Repair pass had issues: {e}", file=sys.stderr)
    return m

# ---------------- Mass / Manifest ----------------

def estimate_mass(cfg: dict) -> float:
    """
    Very rough volume + mass estimate:
      - outer shell as thin ring
      - optional hub ring
      - optional spokes as boxes
      - grousers as plate boxes
    """
    rho = float(cfg.get("material_density_kgpm3", 1200.0))
    R  = float(cfg["outer_radius"])
    w  = float(cfg["width"])
    t  = float(cfg.get("t_tread_m", 0.006))

    # Shell
    V_shell = (2.0 * math.pi * R) * w * t

    # Hub
    hubR   = float(cfg.get("hub_radius_m", 0.0))
    hubWal = float(cfg.get("hub_wall_thickness_m", 0.0))
    hubW   = float(cfg.get("hub_width_m", 0.5 * w))
    if hubR > 0.0 and hubWal > 0.0:
        V_hub = (2.0 * math.pi * hubR) * hubW * hubWal
    else:
        V_hub = 0.0

    # Spokes
    spoke_count = int(cfg.get("spoke_count", 0))
    spoke_root_width = float(cfg.get("spoke_root_width_m", 0.5 * hubR if hubR > 0 else 0.0))
    spoke_thickness  = float(cfg.get("spoke_thickness_m", 0.3 * w))
    if hubR > 0.0 and t > 0.0 and spoke_count > 0 and spoke_root_width > 0.0:
        r_inner = hubR
        r_outer = R - t
        radial_length = max(0.0, r_outer - r_inner)
        V_spoke_single = spoke_root_width * radial_length * spoke_thickness
        V_spokes = spoke_count * V_spoke_single
    else:
        V_spokes = 0.0

    # Grousers
    gH    = float(cfg["grouser_height"])
    gcnt  = int(cfg["grouser_number"])
    pitch = 2.0 * math.pi * R / max(gcnt, 1)
    coverage  = float(cfg.get("grouser_coverage_ratio", 0.25))
    g_tan_thick  = float(cfg.get("grouser_tan_thickness_m", coverage * pitch))
    g_axial_width = float(cfg.get("grouser_axial_width_m", 0.9 * w))
    V_g_single = g_tan_thick * gH * g_axial_width
    V_g = gcnt * V_g_single

    V_total = max(0.0, V_shell + V_hub + V_spokes + V_g)
    return rho * V_total

def manifest(cfg: dict, out_mesh_path: Path) -> dict:
    R_out = float(cfg["outer_radius"])
    effR  = R_out + float(cfg["grouser_height"])
    man = {
        "file": out_mesh_path.as_posix(),
        "wheel_name": cfg.get("name", out_mesh_path.stem),
        "tag": cfg.get("tag", ""),
        "outer_radius": R_out,
        "effective_radius_m": effR,
        "width": float(cfg["width"]),
        "grouser_number": int(cfg["grouser_number"]),
        "grouser_height": float(cfg["grouser_height"]),
        "grouser_coverage_ratio": float(cfg.get("grouser_coverage_ratio", 0.25)),
        "grouser_tan_thickness_m": float(cfg.get("grouser_tan_thickness_m", 0.0)),
        "grouser_axial_width_m": float(cfg.get("grouser_axial_width_m", 0.0)),
        "t_tread_m": float(cfg.get("t_tread_m", 0.006)),
        "hub_radius_m": float(cfg.get("hub_radius_m", 0.0)),
        "hub_wall_thickness_m": float(cfg.get("hub_wall_thickness_m", 0.0)),
        "hub_width_m": float(cfg.get("hub_width_m", 0.0)),
        "hub_z_offset_m": float(cfg.get("hub_z_offset_m", 0.0)),
        "spoke_count": int(cfg.get("spoke_count", 0)),
        "spoke_root_width_m": float(cfg.get("spoke_root_width_m", 0.0)),
        "spoke_thickness_m": float(cfg.get("spoke_thickness_m", 0.0)),
        "spoke_z_offset_m": float(cfg.get("spoke_z_offset_m", cfg.get("hub_z_offset_m", 0.0))),
        "material_density_kgpm3": float(cfg.get("material_density_kgpm3", 1200.0)),
        "estimated_mass_kg": estimate_mass(cfg),
        "notes": cfg.get("notes", "")
    }
    return man

# ---------------- IO / Main ----------------

def load_config(path: Path) -> dict:
    """Load JSON config file."""
    text = path.read_text()
    return json.loads(text)

def save_mesh(mesh: trimesh.Trimesh, path: Path, filetype: str):
    filetype = filetype.lower()
    mesh.export(path, file_type=filetype)

def main():
    ap = argparse.ArgumentParser(description="Generate parametric wheel mesh + manifest for Chrono/DEM.")
    ap.add_argument("--config", required=True, help="wheel.json config file")
    ap.add_argument("--outdir", required=True, help="output directory (e.g., meshes/)")
    ap.add_argument("--format", default="obj", choices=["obj", "stl", "ply"], help="mesh format")
    ap.add_argument("--tag", default="", help="extra tag for filenames/manifest")
    ap.add_argument("--union", action="store_true", help="try boolean union")
    ap.add_argument("--aggressive-repair", action="store_true", help="extra repair passes")
    args = ap.parse_args()

    outdir = Path(args.outdir).expanduser().resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    cfg = load_config(Path(args.config).expanduser().resolve())
    if args.tag:
        cfg["tag"] = args.tag
    cfg["boolean_union"] = bool(args.union)

    mesh = build_wheel_mesh(cfg)
    mesh = qa_and_repair(mesh, aggressive=args.aggressive_repair)

    Rmm  = int(round(1000.0 * float(cfg["outer_radius"])))
    Wmm  = int(round(1000.0 * float(cfg["width"])))
    G    = int(cfg["grouser_number"])
    GHmm = int(round(1000.0 * float(cfg["grouser_height"])))
    tag  = cfg.get("tag", "")
    base = f"wheel_R{Rmm}_W{Wmm}_G{G}_h{GHmm}{('_' + tag) if tag else ''}"

    mesh_path = outdir / f"{base}.{args.format}"
    save_mesh(mesh, mesh_path, args.format)

    man = manifest(cfg, mesh_path)
    man_path = outdir / f"{base}.json"
    man_path.write_text(json.dumps(man, indent=2))

    print(f"[i] Saved mesh: {mesh_path}")
    print(f"[i] Saved manifest: {man_path}")
    print(f"[i] Mesh summary: V={len(mesh.vertices)} F={len(mesh.faces)}")
    print(f"[i] Watertight? {mesh.is_watertight} | Normals consistent? {trimesh.repair.is_winding_consistent(mesh)}")

if __name__ == "__main__":
    main()
