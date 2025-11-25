# DRBG Benchmark Project - Makefile

# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
DEBUGFLAGS := -g -O0 -DDEBUG

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin

# Files
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
EXECUTABLE := $(BIN_DIR)/drbg_benchmark

# Include path
INCLUDES := -I$(INC_DIR)

# Default target
.PHONY: all
all: directories $(EXECUTABLE)

# Create directories
.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Link executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@
	@echo "✅ Build complete: $(EXECUTABLE)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
.PHONY: debug
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean all

# Run the benchmark
.PHONY: run
run: all
	@echo "🚀 Running DRBG benchmark..."
	@./$(EXECUTABLE)

# Clean build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f benchmark_results.csv plot_results.py visualization.html
	rm -f drbg_comparison.png drbg_comparison.svg
	@echo "🧹 Cleaned build artifacts"

# Generate visualization (requires Python with matplotlib and pandas)
.PHONY: plot
plot: run
	@echo "📊 Generating plots..."
	@python3 plot_results.py

# Help
.PHONY: help
help:
	@echo "DRBG Benchmark Makefile"
	@echo "========================"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the project (default)"
	@echo "  run      - Build and run the benchmark"
	@echo "  debug    - Build with debug symbols"
	@echo "  plot     - Run benchmark and generate plots"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Output files:"
	@echo "  benchmark_results.csv  - Raw benchmark data"
	@echo "  visualization.html     - Interactive HTML charts"
	@echo "  plot_results.py        - Python plotting script"
