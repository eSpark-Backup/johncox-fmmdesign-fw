
#!/bin/bash

# Output directory for binaries
OUTPUT_DIR="$1"

# Source binary paths
PROJECT_BIN="${2}"
PROJECT_ELF="${3}"
BOOTLOADER_BIN="${4}"
PARTITION_TABLE_BIN="${5}"

# Ensure the output directory exists
mkdir -p "$OUTPUT_DIR"

# Copy binary files
echo "Copying binaries to $OUTPUT_DIR..."
cp "$PROJECT_BIN" "$OUTPUT_DIR"
echo "Copied $(basename "$PROJECT_BIN") to $OUTPUT_DIR"

cp "$PROJECT_ELF" "$OUTPUT_DIR"
echo "Copied $(basename "$PROJECT_ELF") to $OUTPUT_DIR"

cp "$BOOTLOADER_BIN" "$OUTPUT_DIR"
echo "Copied $(basename "$BOOTLOADER_BIN") to $OUTPUT_DIR"

cp "$PARTITION_TABLE_BIN" "$OUTPUT_DIR"
echo "Copied $(basename "$PARTITION_TABLE_BIN") to $OUTPUT_DIR"

echo "All binaries successfully copied!"

