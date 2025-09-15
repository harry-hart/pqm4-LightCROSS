cmake_minimum_required(VERSION 3.7)

project(CROSS C)
set(CMAKE_C_STANDARD 11)

set(CC gcc)
# set(CC clang)
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -pedantic -Wuninitialized -march=native -O0 -g3")
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${SANITIZE} ${TEST_FLAGS}")
message("Compilation flags:" ${CMAKE_C_FLAGS})

#generate by default KATs with standalone keccak library
set(STANDALONE_KECCAK 1)

foreach(impl ref light)
  foreach(category RANGE 1 5 2)
      set(RSDP_VARIANTS sdp sdpg)
      foreach(RSDP_VARIANT ${RSDP_VARIANTS})
          set(PARAM_TARGETS small balance fast)
          foreach(optimiz_target ${PARAM_TARGETS})
              set(COMMON_DIR ../../common)
              set(PQCLEAN_COMMON_DIR ../../pqclean/common)
              set(BASE_DIR ../crossv2.0-sha3-r-${RSDP_VARIANT}-${category}-${optimiz_target}/${impl})
#if(REFERENCE EQUAL 1)
#message("Compiling portable reference code")
              set(SPEC_HEADERS
                      ${BASE_DIR}/sha3.h
                      ${BASE_DIR}/csprng_hash.h
                      ${BASE_DIR}/restr_arith.h
                      ${BASE_DIR}/fp_arith.h )
              set(SPEC_SOURCES
                      ${BASE_DIR}/merkle.c
                      ${BASE_DIR}/seedtree.c
                      ${BASE_DIR}/CROSS.c
              )
#else()
#  message("Compiling optimized LighCROSS code")
#set(BASE_DIR ${OPTIMIZED_CODE_DIR})
#set(SPEC_HEADERS
#        ${BASE_DIR}/include/align.h
#        ${BASE_DIR}/include/fips202x4.h
#        ${BASE_DIR}/include/KeccakP-1600-times4-SnP.h
#        ${BASE_DIR}/include/SIMD256-config.h
#        ${BASE_DIR}/include/sha3.h
#        ${BASE_DIR}/include/csprng_hash.h
#        ${BASE_DIR}/include/architecture_detect.h
#        ${BASE_DIR}/include/restr_arith.h
#        ${BASE_DIR}/include/fp_arith.h )
#set(SPEC_SOURCES
#        ${BASE_DIR}/lib/fips202x4.c
#        ${BASE_DIR}/lib/KeccakP-1600-times4-SIMD256.c
#        ${BASE_DIR}/lib/merkle.c
#        ${BASE_DIR}/lib/seedtree.c
#        ${BASE_DIR}/lib/CROSS.c
#)
#endif()

              set(HEADERS
                  ${SPEC_HEADERS}
                  ${BASE_DIR}/api.h
                  ${BASE_DIR}/CROSS.h
                  ${BASE_DIR}/pack_unpack.h
                  ${COMMON_DIR}/fips202.h
                  ${COMMON_DIR}/keccakf1600.h
                  ${BASE_DIR}/parameters.h
                  ${BASE_DIR}/seedtree.h
                  ${BASE_DIR}/merkle_tree.h
              )

              if(STANDALONE_KECCAK)
                message("Employing standalone SHA-3")
                set(KECCAK_EXTERNAL_LIB "")
                set(KECCAK_EXTERNAL_ENABLE "")
                list(APPEND FALLBACK_SOURCES ${COMMON_DIR}/keccakf1600.c)
                list(APPEND FALLBACK_SOURCES ${COMMON_DIR}/fips202.c)
              else()
                message("Employing libkeccak")
                set(KECCAK_EXTERNAL_LIB keccak)
                set(KECCAK_EXTERNAL_ENABLE "-DSHA_3_LIBKECCAK")
              endif()


              set(SOURCES
                  ${SPEC_SOURCES}
                  ${FALLBACK_SOURCES}
                  ${BASE_DIR}/csprng_hash.c
                  ${BASE_DIR}/pack_unpack.c
                  ${COMMON_DIR}/keccakf1600.c
                  ${COMMON_DIR}/fips202.c
                  ${BASE_DIR}/sign.c
                  ${PQCLEAN_COMMON_DIR}/randombytes.c
              )

              if(optimiz_target STREQUAL SPEED)
                  set(OMIT_SEED_TREE "-DNO_TREES=1")
              else()
                  set(OMIT_SEED_TREE "")
              endif()
              # settings for benchmarking binary
              set(TARGET_BINARY_NAME CROSS_KATgen_cat_${category}_${RSDP_VARIANT}_${optimiz_target}_${impl})
              add_executable(${TARGET_BINARY_NAME} ${HEADERS} ${SOURCES}
                                  ./src/hal-x86.c ./src/rng.c ./src/PQCgenKAT_sign.c)
              target_include_directories(${TARGET_BINARY_NAME} PRIVATE
                                          ${BASE_DIR}
                                          ${COMMON_DIR}
                                          ./src)
              target_link_libraries(${TARGET_BINARY_NAME} m ssl crypto ${SANITIZE} ${KECCAK_EXTERNAL_LIB})
              set_target_properties(${TARGET_BINARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./bin)
              set_property(TARGET ${TARGET_BINARY_NAME} APPEND PROPERTY
                  COMPILE_FLAGS "${KECCAK_EXTERNAL_ENABLE} ")        
          endforeach(optimiz_target)
      endforeach(RSDP_VARIANT)
  endforeach(category)
endforeach(impl)
