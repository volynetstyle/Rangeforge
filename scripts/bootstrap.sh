#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
PIXI_DIR="$ROOT/.pixi/bin"
PIXI="$PIXI_DIR/pixi"
VERSION="v0.81.0"

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)
    ASSET="pixi-x86_64-unknown-linux-musl"
    SHA256="248510f5f75460a2ae4ec62d761e15c0191dc35db3b1da6f82d9f17c6017051d"
    ;;
  Darwin-arm64|Darwin-aarch64)
    ASSET="pixi-aarch64-apple-darwin"
    SHA256="f389557fc5de929cc33a042f6ae5abbaeb087ffa1affd55aa6ab8da3222d1122"
    ;;
  *)
    echo "Unsupported host: $(uname -s)-$(uname -m)" >&2
    exit 1
    ;;
esac

if [ ! -x "$PIXI" ]; then
  mkdir -p "$PIXI_DIR"
  temporary_pixi="$PIXI.download"
  trap 'rm -f "$temporary_pixi"' EXIT HUP INT TERM
  curl --fail --location --retry 3 --output "$temporary_pixi" \
    "https://github.com/prefix-dev/pixi/releases/download/$VERSION/$ASSET"
  actual_sha256="$(shasum -a 256 "$temporary_pixi" 2>/dev/null | cut -d ' ' -f 1 || sha256sum "$temporary_pixi" | cut -d ' ' -f 1)"
  if [ "$actual_sha256" != "$SHA256" ]; then
    echo "Pixi checksum mismatch: expected $SHA256, got $actual_sha256" >&2
    exit 1
  fi
  mv "$temporary_pixi" "$PIXI"
  chmod +x "$PIXI"
  trap - EXIT HUP INT TERM
fi

export PIXI_HOME="$ROOT/.pixi/home"
export PIXI_CACHE_DIR="$ROOT/.pixi/cache"
"$PIXI" install --locked
echo "Rangeforge is ready. Run: ./.pixi/bin/pixi run check"
