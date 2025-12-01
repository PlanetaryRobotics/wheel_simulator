#!/usr/bin/env bash
set -euo pipefail

echo "[i] Setting up environment for gen_wheel.py (Linux)..."

# ----- Locate repo root (directory containing this script) -----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ----- Choose Python -----
PYTHON_BIN="${PYTHON_BIN:-python3}"

if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    echo "[e] Could not find python3 (or \$PYTHON_BIN) on PATH."
    echo "    Please load a Python module or install Python 3, then re-run this script."
    exit 1
fi

echo "[i] Using Python: $PYTHON_BIN"

# ----- Create / reuse virtual environment -----
VENV_DIR="$SCRIPT_DIR/.venv"

if [ ! -d "$VENV_DIR" ]; then
    echo "[i] Creating virtual environment in $VENV_DIR ..."
    "$PYTHON_BIN" -m venv "$VENV_DIR"
else
    echo "[i] Reusing existing virtual environment at $VENV_DIR"
fi

# Activate venv
# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"

echo "[i] Python in venv: $(command -v python)"
echo "[i] Upgrading pip..."
python -m pip install --upgrade pip

# ----- Check for required Python packages -----
REQUIRED_PKGS=(trimesh shapely)

echo "[i] Checking for required Python packages: ${REQUIRED_PKGS[*]}"

MISSING_PKGS=()

for pkg in "${REQUIRED_PKGS[@]}"; do
    if python - <<PY >/dev/null 2>&1
import importlib; importlib.import_module("${pkg}")
PY
    then
        echo "[i] Package '${pkg}' is already installed."
    else
        echo "[i] Package '${pkg}' is missing."
        MISSING_PKGS+=("$pkg")
    fi
done

# ----- Install missing packages -----
if [ "${#MISSING_PKGS[@]}" -gt 0 ]; then
    echo "[i] Installing missing packages: ${MISSING_PKGS[*]}"
    python -m pip install "${MISSING_PKGS[@]}"
else
    echo "[i] All required packages are already installed."
fi

echo
echo "[i] Environment setup complete."
echo "[i] To use it, run:"
echo "    source \"$VENV_DIR/bin/activate\""
echo "    python gen_wheel.py"
