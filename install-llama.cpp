#!/usr/bin/env bash
# install-llama-cpp.sh — build llama.cpp from source and install system-wide
# Tested on Fedora. Re-run anytime to update.
#
# Usage:
#   ./install-llama-cpp.sh              # build latest master
#   ./install-llama-cpp.sh --pr 22673   # build a specific PR branch (e.g. MTP support)
#   ./install-llama-cpp.sh --cuda       # enable NVIDIA CUDA backend
#   ./install-llama-cpp.sh --hip        # enable AMD ROCm/HIP backend
#   Flags can be combined: ./install-llama-cpp.sh --pr 22673 --cuda

set -euo pipefail

SRC_DIR="${HOME}/.local/src/llama.cpp"
PREFIX="/usr/local"
PR_NUMBER=""
GPU_FLAGS=()

# --- argument parsing ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --pr)    PR_NUMBER="$2"; shift 2 ;;
        --cuda)  GPU_FLAGS+=("-DGGML_CUDA=ON"); shift ;;
        --hip)   GPU_FLAGS+=("-DGGML_HIP=ON"); shift ;;
        -h|--help)
            sed -n '2,11p' "$0"; exit 0 ;;
        *) echo "Unknown flag: $1" >&2; exit 1 ;;
    esac
done

info()  { printf '\033[1;34m::\033[0m %s\n' "$*"; }
error() { printf '\033[1;31m!!\033[0m %s\n' "$*" >&2; exit 1; }

# --- sanity checks ---
command -v dnf >/dev/null || error "dnf not found — this script is for Fedora."
[[ $EUID -eq 0 ]] && error "Don't run this as root. It'll sudo when needed."

# --- 1. dependencies ---
info "Installing build dependencies..."
sudo dnf install -y git cmake gcc-c++ libcurl-devel

# --- 2. clone or update ---
if [[ -d "${SRC_DIR}/.git" ]]; then
    info "Updating existing checkout at ${SRC_DIR}..."
    git -C "${SRC_DIR}" fetch origin
else
    info "Cloning llama.cpp into ${SRC_DIR}..."
    mkdir -p "$(dirname "${SRC_DIR}")"
    git clone https://github.com/ggml-org/llama.cpp "${SRC_DIR}"
fi

# --- 3. checkout the right branch ---
cd "${SRC_DIR}"
# discard any local changes from previous builds
git reset --hard HEAD

if [[ -n "${PR_NUMBER}" ]]; then
    BRANCH="pr-${PR_NUMBER}"
    info "Checking out PR #${PR_NUMBER} as branch '${BRANCH}'..."
    git fetch origin "pull/${PR_NUMBER}/head:${BRANCH}" --force
    git checkout "${BRANCH}"
else
    info "Checking out latest master..."
    git checkout master
    git reset --hard origin/master
fi

CURRENT_REF=$(git rev-parse --short HEAD)
info "Building from commit ${CURRENT_REF}"

# --- 4. build ---
# nuke the build dir, switching branches can leave a stale CMake cache
rm -rf "${SRC_DIR}/build"

info "Configuring build${GPU_FLAGS:+ with ${GPU_FLAGS[*]}}..."
cmake -S "${SRC_DIR}" -B "${SRC_DIR}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DLLAMA_CURL=ON \
    -DBUILD_SHARED_LIBS=ON \
    "${GPU_FLAGS[@]}"

info "Compiling (this takes a while)..."
cmake --build "${SRC_DIR}/build" --config Release -j"$(nproc)"

# --- 5. install ---
info "Installing to ${PREFIX}..."
sudo cmake --install "${SRC_DIR}/build"

# On Fedora, /usr/local/lib64 isn't in the default linker search path.
# Drop a config file so binaries can find libllama-common.so etc.
if [[ ! -f /etc/ld.so.conf.d/local-lib64.conf ]]; then
    info "Adding /usr/local/lib64 to linker search path..."
    echo "/usr/local/lib64" | sudo tee /etc/ld.so.conf.d/local-lib64.conf >/dev/null
fi
sudo ldconfig

# --- 6. verify ---
info "Done. Installed binaries:"
ls "${PREFIX}/bin/" | grep '^llama-' | sed 's/^/  /'

printf '\n'
info "Try it: llama-cli --version"
