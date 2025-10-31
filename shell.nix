{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # PostgreSQL with development headers
    postgresql
    libpq
    
    # Crypt library
    libxcrypt
    
    # Build tools
    cmake
    clang
    pkg-config
    
    # Make sure we have the standard C library headers
    glibc.dev
    
    # Additional tools for linting
    clang-tools
  ];

  shellHook = ''
    export PKG_CONFIG_PATH="${pkgs.postgresql}/lib/pkgconfig:$PKG_CONFIG_PATH"
    export CMAKE_PREFIX_PATH="${pkgs.postgresql}:${pkgs.libxcrypt}:$CMAKE_PREFIX_PATH"
    
    # Set compiler flags for clang-tidy with all necessary includes
    export CFLAGS="--sysroot=${pkgs.glibc} -I${pkgs.glibc.dev}/include -I${pkgs.postgresql}/include -I${pkgs.libpq}/include -I${pkgs.libxcrypt}/include"
    
    echo "Development environment loaded with PostgreSQL and crypt support"
    echo "PostgreSQL include path: ${pkgs.postgresql}/include"
    echo "libpq include path: ${pkgs.libpq}/include"
    echo "System headers path: ${pkgs.glibc.dev}/include"
  '';
}