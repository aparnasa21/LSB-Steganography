#!/bin/sh
# Round-trip test: hide a file in an image, extract it, compare.
set -e
BIN=./lsb_steg
IMG=samples/beautiful.bmp
SECRET=samples/secret.txt
STEGO=tests/stego.tmp
OUT=tests/decoded.tmp

$BIN -e "$IMG" "$SECRET" "$STEGO" > /dev/null
$BIN -d "$STEGO" "$OUT" > /dev/null

if cmp -s "$SECRET" "$OUT"; then
    echo "PASS: decoded file matches the original secret"
else
    echo "FAIL: decoded file differs from the original" >&2
    exit 1
fi
rm -f "$STEGO" "$OUT"
