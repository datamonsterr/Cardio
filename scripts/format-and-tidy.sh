#!/bin/bash

# Comprehensive formatting and tidying script for the Cardio project
# This script can be run independently or called from the Makefile

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND_DIR="${PROJECT_ROOT}/backend"

# Include directories for clang-tidy
INCLUDE_DIRS=(
    "-I${BACKEND_DIR}/include"
    "-I${BACKEND_DIR}/lib/card/include"
    "-I${BACKEND_DIR}/lib/db/include"
    "-I${BACKEND_DIR}/lib/logger/include"
    "-I${BACKEND_DIR}/lib/pokergame/include"
    "-I${BACKEND_DIR}/lib/utils"
    "-I${BACKEND_DIR}/lib/mpack/include"
)

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to find all source files
find_source_files() {
    local file_type="$1"
    find "${BACKEND_DIR}" -type f -name "*.${file_type}" \
        ! -path "*/build/*" \
        ! -path "*/mpack/*" \
        2>/dev/null || true
}

# Function to check if tools are available
check_tools() {
    print_status "Checking for required tools..."
    
    local missing_tools=()
    
    if ! command -v clang-format >/dev/null 2>&1; then
        missing_tools+=("clang-format")
    fi
    
    if ! command -v clang-tidy >/dev/null 2>&1; then
        missing_tools+=("clang-tidy")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_status "Install them with: sudo apt install clang-format clang-tidy"
        exit 1
    fi
    
    print_success "All required tools are available"
}

# Function to format files
format_files() {
    print_status "Formatting C/C++ files with clang-format..."
    
    local c_files=($(find_source_files "c"))
    local h_files=($(find_source_files "h"))
    local all_files=("${c_files[@]}" "${h_files[@]}")
    
    if [ ${#all_files[@]} -eq 0 ]; then
        print_warning "No source files found to format"
        return
    fi
    
    local formatted=0
    for file in "${all_files[@]}"; do
        if clang-format -i "$file"; then
            ((formatted++))
        else
            print_error "Failed to format $file"
        fi
    done
    
    print_success "Formatted $formatted/${#all_files[@]} files"
}

# Function to check formatting
check_formatting() {
    print_status "Checking formatting of C/C++ files..."
    
    local c_files=($(find_source_files "c"))
    local h_files=($(find_source_files "h"))
    local all_files=("${c_files[@]}" "${h_files[@]}")
    
    if [ ${#all_files[@]} -eq 0 ]; then
        print_warning "No source files found to check"
        return 0
    fi
    
    local needs_formatting=()
    for file in "${all_files[@]}"; do
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            needs_formatting+=("$file")
        fi
    done
    
    if [ ${#needs_formatting[@]} -eq 0 ]; then
        print_success "All ${#all_files[@]} files are properly formatted"
        return 0
    else
        print_error "${#needs_formatting[@]} files need formatting:"
        printf '  %s\n' "${needs_formatting[@]}"
        return 1
    fi
}

# Function to lint files
lint_files() {
    print_status "Linting C files with clang-tidy..."
    
    local c_files=($(find_source_files "c"))
    
    if [ ${#c_files[@]} -eq 0 ]; then
        print_warning "No C files found to lint"
        return
    fi
    
    local linted=0
    local errors=0
    for file in "${c_files[@]}"; do
        print_status "Linting $(basename "$file")..."
        if clang-tidy "$file" -- "${INCLUDE_DIRS[@]}"; then
            ((linted++))
        else
            ((errors++))
            print_warning "Linting issues found in $file"
        fi
    done
    
    print_success "Linted $linted/${#c_files[@]} files"
    if [ $errors -gt 0 ]; then
        print_warning "$errors files had linting issues"
    fi
}

# Function to lint and fix files
lint_and_fix_files() {
    print_status "Linting and fixing C files with clang-tidy..."
    
    local c_files=($(find_source_files "c"))
    
    if [ ${#c_files[@]} -eq 0 ]; then
        print_warning "No C files found to lint and fix"
        return
    fi
    
    local fixed=0
    for file in "${c_files[@]}"; do
        print_status "Linting and fixing $(basename "$file")..."
        if clang-tidy --fix-errors "$file" -- "${INCLUDE_DIRS[@]}"; then
            ((fixed++))
        else
            print_warning "Could not fix all issues in $file"
        fi
    done
    
    print_success "Processed $fixed/${#c_files[@]} files"
}

# Function to clean backup files
clean_backups() {
    print_status "Cleaning formatting backup files..."
    
    local cleaned=0
    for pattern in "*.orig" "*~" "*.bak"; do
        while IFS= read -r -d '' file; do
            rm "$file"
            ((cleaned++))
        done < <(find "${BACKEND_DIR}" -name "$pattern" -type f -print0 2>/dev/null)
    done
    
    if [ $cleaned -gt 0 ]; then
        print_success "Cleaned $cleaned backup files"
    else
        print_status "No backup files to clean"
    fi
}

# Main function
main() {
    echo "🎨 Cardio Project - Code Formatting and Tidying Script"
    echo "====================================================="
    
    case "${1:-tidy}" in
        "format")
            check_tools
            format_files
            ;;
        "format-check")
            check_tools
            check_formatting
            ;;
        "lint")
            check_tools
            lint_files
            ;;
        "lint-fix")
            check_tools
            lint_and_fix_files
            ;;
        "tidy")
            check_tools
            format_files
            lint_files
            clean_backups
            print_success "Full code tidying complete!"
            ;;
        "clean")
            clean_backups
            ;;
        "help"|"-h"|"--help")
            echo "Usage: $0 [command]"
            echo ""
            echo "Commands:"
            echo "  format      - Format all C files with clang-format"
            echo "  format-check- Check formatting without changes"
            echo "  lint        - Lint all C files with clang-tidy"
            echo "  lint-fix    - Lint and auto-fix issues where possible"
            echo "  tidy        - Run format, lint, and cleanup (default)"
            echo "  clean       - Clean backup files"
            echo "  help        - Show this help message"
            ;;
        *)
            print_error "Unknown command: $1"
            print_status "Use '$0 help' for usage information"
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"