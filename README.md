# LSB Image Steganography in C

A command-line tool that hides a secret file inside a 24-bit BMP image using
**least-significant-bit (LSB) steganography**, and extracts it again. The
stego image looks identical to the original to the human eye, but carries the
hidden file in its pixel data.

## How it works

Every pixel byte in a BMP is a colour value from 0 to 255. Changing only the
lowest bit of a byte changes the colour by at most 1/255, which is invisible.
So each byte of the secret is split into 8 bits, and those 8 bits are stored
in the lowest bit of 8 consecutive image bytes.

```
secret byte 'A' = 0 1 0 0 0 0 0 1

image bytes   1011 0110   0100 1101   ...   (8 bytes, one per secret bit)
after encode  1011 011[0] 0100 110[1] ...   <- only the last bit changes
```

The hidden data is laid out in this order right after the 54-byte BMP header
(which is copied unchanged):

| Field | Size in file | Purpose |
|---|---|---|
| Magic string `#*` | 2 bytes | Lets the decoder check the image really holds hidden data |
| File extension | 4 bytes | e.g. `.txt`, so the decoder can name the output correctly |
| Secret file size | 4 bytes | Tells the decoder how many bytes to read |
| Secret file data | N bytes | The hidden payload |

Each of these bytes takes 8 image bytes. The rest of the image is copied as-is.

## Build

Requires `gcc` and `make`.

```
make
```

This produces the `lsb_steg` executable.

## Usage

```
./lsb_steg -e <input.bmp> <secret_file> [output.bmp]    Hide a file in an image
./lsb_steg -d <stego.bmp> [output_file]                 Extract the hidden file
```

If you don't give an output name, encoding writes `steged_img.bmp`, and
decoding writes `decoded_output<extension>` (for example
`decoded_output.txt`).

### Example

```
$ ./lsb_steg -e samples/beautiful.bmp samples/secret.txt
INFO: Output File not mentioned. Creating steged_img.bmp as default
INFO: ## Encoding Procedure Started ##
INFO: Opened all required files
width = 1024
height = 768
INFO: Capacity OK
...
INFO: ## Encoding Done Successfully ##
Encoding completed successfully

$ ./lsb_steg -d steged_img.bmp
INFO: ## Decoding Procedure Started ##
INFO: Magic string matched - valid stego image
INFO: Decoded file extension: .txt
INFO: Decoded secret file size: 25 bytes
INFO: ## Decoding Done Successfully ##
INFO: Decoded data written to decoded_output.txt

$ cat decoded_output.txt
My password is SECRET ;)
```

Decoding an image that has nothing hidden in it fails cleanly:

```
$ ./lsb_steg -d samples/beautiful.bmp
ERROR: Magic string mismatch - not a valid stego image, or no hidden data present
Decoding failed
```

## Test

```
make test
```

Hides `samples/secret.txt` in `samples/beautiful.bmp`, extracts it, and checks
that the result is byte-for-byte identical to the original.

## Project structure

```
.
├── src/
│   ├── main.c        Argument handling and top-level flow
│   ├── encode.c      Capacity check, LSB encoding, writing the stego image
│   └── decode.c      Magic-string check, LSB decoding, writing the secret file
├── inc/
│   ├── types.h       Status and operation-type enums
│   ├── common.h      Magic string definition
│   ├── encode.h      EncodeInfo structure and encode prototypes
│   └── decode.h      DecodeInfo structure and decode prototypes
├── samples/          Sample 1024x768 BMP and a secret text file
├── tests/            Round-trip test script
└── Makefile
```

## Capacity

The image must have enough pixel bytes to hold 8 bits for every hidden byte
plus the 10 bytes of header fields above. The program checks this before
encoding and refuses to continue if the secret is too large. For the included
1024x768 image that is roughly 294 KB of secret data.

## Limitations

- Expects an **uncompressed 24-bit BMP** (54-byte header). Other formats are
  not detected or rejected.
- The file extension is stored in 4 bytes including the dot, so longer
  extensions are truncated (`.json` becomes `.jso`), and the secret file must
  have an extension.
- The hidden data is **not encrypted**. Anyone who knows the layout can read
  it. Encrypting the secret before hiding it would be a natural next step.
- LSB hiding does not survive lossy re-compression (for example converting
  the image to JPEG).

## Concepts practised

C file I/O in binary mode, bitwise operations (masking and shifting),
structures for passing state between functions, multi-file projects with
headers, and Makefiles.
