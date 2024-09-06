set -e

if [[ "$(uname -m)" != "arm64" ]]; then
    echo "Unsupported platform. Grid only supports Apple Silicon Macs"
    exit 1
fi

echo "Downloading Grid..."
curl -L -o /tmp/grid-mac-arm64.zip "https://github.com/m4rc1e/grid/releases/download/v0.0.1/grid-mac-arm64.zip"

echo "Unzipping Grid..."
unzip -o /tmp/grid-mac-arm64.zip -d /usr/local/bin

echo "Cleaning up..."
rm /tmp/grid-mac-arm64.zip

echo "Grid installation complete."