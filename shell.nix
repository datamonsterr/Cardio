{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Compiler and build tools
    clang
    cmake

    # Development dependencies
    postgresql # provides libpq
    libxcrypt # provides libcrypt for password hashing

    # Code quality tools
    clang-tools # provides clang-tidy and clang-format

    # Additional build tools that might be needed
    pkg-config
    gnumake

    # For development
    gdb
    valgrind
  ];

  # Set environment variables
  shellHook = ''
    echo "Cardio project development environment"
    echo "====================================="
    echo "Available commands:"
    echo "  make build          - Build all libraries and main project"
    echo "  make test-libs      - Run library unit tests"
    echo "  make test           - Run all unit tests (requires database)"
    echo "  make clean          - Clean build artifacts"
    echo "  make format         - Format C code"
    echo "  make lint           - Lint C code"
    echo ""
    echo "Database setup:"
    echo "  docker compose up -d  # Start PostgreSQL database"
    echo ""
    export CC=clang
    export CXX=clang++
  '';
}