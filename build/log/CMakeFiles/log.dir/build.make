# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zzq/unix/webServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zzq/unix/webServer/build

# Include any dependencies generated for this target.
include log/CMakeFiles/log.dir/depend.make

# Include the progress variables for this target.
include log/CMakeFiles/log.dir/progress.make

# Include the compile flags for this target's objects.
include log/CMakeFiles/log.dir/flags.make

log/CMakeFiles/log.dir/log.cpp.o: log/CMakeFiles/log.dir/flags.make
log/CMakeFiles/log.dir/log.cpp.o: ../log/log.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zzq/unix/webServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object log/CMakeFiles/log.dir/log.cpp.o"
	cd /home/zzq/unix/webServer/build/log && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/log.dir/log.cpp.o -c /home/zzq/unix/webServer/log/log.cpp

log/CMakeFiles/log.dir/log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/log.dir/log.cpp.i"
	cd /home/zzq/unix/webServer/build/log && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zzq/unix/webServer/log/log.cpp > CMakeFiles/log.dir/log.cpp.i

log/CMakeFiles/log.dir/log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/log.dir/log.cpp.s"
	cd /home/zzq/unix/webServer/build/log && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zzq/unix/webServer/log/log.cpp -o CMakeFiles/log.dir/log.cpp.s

log/CMakeFiles/log.dir/log.cpp.o.requires:

.PHONY : log/CMakeFiles/log.dir/log.cpp.o.requires

log/CMakeFiles/log.dir/log.cpp.o.provides: log/CMakeFiles/log.dir/log.cpp.o.requires
	$(MAKE) -f log/CMakeFiles/log.dir/build.make log/CMakeFiles/log.dir/log.cpp.o.provides.build
.PHONY : log/CMakeFiles/log.dir/log.cpp.o.provides

log/CMakeFiles/log.dir/log.cpp.o.provides.build: log/CMakeFiles/log.dir/log.cpp.o


# Object files for target log
log_OBJECTS = \
"CMakeFiles/log.dir/log.cpp.o"

# External object files for target log
log_EXTERNAL_OBJECTS =

lib/liblog.a: log/CMakeFiles/log.dir/log.cpp.o
lib/liblog.a: log/CMakeFiles/log.dir/build.make
lib/liblog.a: log/CMakeFiles/log.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zzq/unix/webServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../lib/liblog.a"
	cd /home/zzq/unix/webServer/build/log && $(CMAKE_COMMAND) -P CMakeFiles/log.dir/cmake_clean_target.cmake
	cd /home/zzq/unix/webServer/build/log && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/log.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
log/CMakeFiles/log.dir/build: lib/liblog.a

.PHONY : log/CMakeFiles/log.dir/build

log/CMakeFiles/log.dir/requires: log/CMakeFiles/log.dir/log.cpp.o.requires

.PHONY : log/CMakeFiles/log.dir/requires

log/CMakeFiles/log.dir/clean:
	cd /home/zzq/unix/webServer/build/log && $(CMAKE_COMMAND) -P CMakeFiles/log.dir/cmake_clean.cmake
.PHONY : log/CMakeFiles/log.dir/clean

log/CMakeFiles/log.dir/depend:
	cd /home/zzq/unix/webServer/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zzq/unix/webServer /home/zzq/unix/webServer/log /home/zzq/unix/webServer/build /home/zzq/unix/webServer/build/log /home/zzq/unix/webServer/build/log/CMakeFiles/log.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : log/CMakeFiles/log.dir/depend
