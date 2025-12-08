#!/usr/bin/env bash
set -euo pipefail

PYTHON="${PYTHON:-python3.7}"
VENV_DIR=".venv"

echo "Using: $PYTHON"
$PYTHON -V

echo "Creating venv: $VENV_DIR"
$PYTHON -m venv "$VENV_DIR"

# activate
# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"

echo "Upgrading pip (Python 3.7 => pip<24.1)"
python -m pip install -U "pip<24.1"

echo "Installing matplotlib (3.5.3 for Python 3.7)"
python -m pip install "matplotlib==3.5.3"

WHEEL="$(ls -1 *.whl | head -n 1)"
echo "Installing wheel: $WHEEL"
python -m pip install "$WHEEL"

echo "Done. Activate later:"
echo "  source $VENV_DIR/bin/activate"
echo "Test:"
echo "  python -c \"import rhythmic_delivery as rd; import matplotlib; print('ok')\""
