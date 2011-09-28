SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

file(GLOB TOP_SRC *.cc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")

if(${STATIC_LINK})
  # probably not the cleanest way to do (-static won't exist for all
  # compilers). Using find_library and putting .a at the front of
  # CMAKE_FIND_LIBRARY_SUFFIXES should also do the trick if ever
  # needed.
  set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

if(${PRINT_DEBUG})
  add_definitions(-DPRINT_DEBUG)
endif()

if(${PRINT_DEBUG_SMT})
  add_definitions(-DPRINT_DEBUG_SMT)
endif()

include_directories(
  ${YICES_PATH}/include 
  ${Z3_PATH}/include
  ${GMP_PREFIX}/include
  ${MPFR_PREFIX}/include
  ${APRON_PREFIX}/include 
  ${CUDD_PATH}/include
)

# TODO: If we don't add /usr/lib at the front, then the pagai executable
# tries to link against libgmp.so.3 which doesn't exist. It seems to
# be linked to the version of libutils chosen by the linker.
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib ${CMAKE_EXE_LINKER_FLAGS}")
link_directories(
  ${GMP_PREFIX}/lib
  ${MPFR_PREFIX}/lib 
  ${APRON_PREFIX}/lib
  ${PPL_PREFIX}/lib
  ${CAMLIDL_PREFIX}/lib/ocaml/apron
  ${YICES_PATH}/lib 
  ${Z3_PATH}/lib 
)


add_executable(pagai ${TOP_SRC})
target_link_libraries(pagai ${LLVM_LIBS})
# Apron and dependancies
target_link_libraries(pagai polkaMPQ octD boxMPFR ap_ppl ap_pkgrid apron ppl)
# SMT solvers
target_link_libraries(pagai yices z3)
target_link_libraries(pagai mpfr gmpxx gmp)
# CUDD (BDD library)
# TODO: There is a confusion between /lib/libutil and CUDD's libutil,
# hence we need to add CUDD one by its full name (If we don't do this,
# it doesn't link on Julien's machine). While we're there, make all
# CUDD library paths absolute and unambiguous.
add_cudd_library(obj)
add_cudd_library(cudd)
add_cudd_library(util)
add_cudd_library(mtr)
add_cudd_library(st)
add_cudd_library(epd)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DOXYFILE.in ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE @ONLY)
ADD_CUSTOM_TARGET(doc
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE
  )