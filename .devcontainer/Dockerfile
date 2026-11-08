FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl-dev libarchive-dev libcurl4-openssl-dev python3-venv ninja-build \
    && rm -rf /var/lib/apt/lists/*
# No private repository or credential is copied into this image. Install an
# authorized CurlWrapper checkout into /workspaces/dependencies/installed.
