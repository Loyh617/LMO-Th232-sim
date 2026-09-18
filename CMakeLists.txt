#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.16...3.27)
project(LMOTh232Sim)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# See the documentation for a guide on how to enable/disable specific components
#
find_package(Geant4 REQUIRED ui_all vis_all)
# find_package(G4CMP REQUIRED)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, use our local headers, and link it to the Geant4 libraries
#
add_executable(LMOTh232Sim LMOTh232Sim.cc ${sources} ${headers})
target_include_directories(LMOTh232Sim PRIVATE
    include
    # ${G4CMP_INCLUDE_DIRS}
)

target_link_libraries(LMOTh232Sim PRIVATE
    ${Geant4_LIBRARIES}
    # ${G4CMP_LIBRARIES}
)

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build LMOTh232Sim. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(LMOTh232Sim_SCRIPTS
  init_vis.mac
  run.mac
  vis.mac
  )

foreach(_script ${LMOTh232Sim_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()
