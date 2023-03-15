project(mom)

set(ROOT .)
set(LIB_DIR ./lib)
set(SRC_DIR ./src)
set(INC_DIR ./inc)
set(TEST_DIR ./test)


set(LIB ${SRC_DIR}/mom_call_app.c ${SRC_DIR}/mom_list.c ${SRC_DIR}/mom_map.c ${SRC_DIR}/mom_common.c ${SRC_DIR}/mom_resource.c ${SRC_DIR}/mom_collection.c ${SRC_DIR}/mom_collection_core.c ${SRC_DIR}/mom_result.c ${SRC_DIR}/mom_shared_data.c ${SRC_DIR}/mom_shared_map.c ${SRC_DIR}/mom_shared_queue.c)
set(LIB_NAME mom)
set(TEST ${TEST_DIR}/main.c)
set(OUTPUT test_mom)
set(EXE_FILE program)

# 최소 cmake 요구 버젼
cmake_minimum_required(VERSION 3.24)
# 메시지 설정
message(${CMAKE_PROJECT_NAME})
# 컴파일 옵션 = g++ -W -Wall -o -c source
add_compile_options(-W -Wall)

# 공통 해더 파일 설정
include_directories(${INC_DIR})

add_library(${LIB_NAME} SHARED ${LIB})

# 빌드 설정
add_executable(${OUTPUT} ${TEST})

# 라이브러리 설정, 라이브러리 이름 = libtest.so

# 빌드시에 참조 라이브러리
target_link_libraries(${OUTPUT} PUBLIC ${LIB_NAME})

# make install 설정, TARGETS 1, 2 ,3 = RUNTIME DESTINATION, LIBRARY DESTINATION, ARCHIVE DESTINATION
install(TARGETS  ${OUTPUT} ${LIB_NAME}
        RUNTIME DESTINATION /Users/dk.choi/tst/bin
        LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH})

#set(APPS "mom.app")
#
## Directories to look for dependencies
#set(DIRS ${CMAKE_LIBRARY_PATH})
#
#install(CODE "include(BundleUtilities)
#    fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")
#
#set(CPACK_GENERATOR Bundle)
#set(CPACK_BUNDLE_NAME "_NAME_")
#set(VERSION "0.1.1")
#set(CPACK_PACKAGE_RELEASE 2)
#set(CPACK_PACKAGE_VERSION ${VERSION})VERSION
#set(CPACK_GENERATOR Bundle)
#set(CPACK_BUNDLE_NAME "_NAME_")
#set(CPACK_PACKAGE_CONTACT "developer")
#set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
#set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

#include(CPack)