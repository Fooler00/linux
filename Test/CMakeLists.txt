cmake_minimum_required(VERSION 3.13)

set(TEST_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

include(GoogleTest)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(backup_test_smoke
    smoke/test_smoke.cpp
)
target_link_libraries(backup_test_smoke PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(backup_test_smoke
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:backup_test_smoke>"
    PROPERTIES
        LABELS "smoke"
        TIMEOUT 30
)

add_library(backup_test_helpers INTERFACE)
target_include_directories(backup_test_helpers INTERFACE "${TEST_ROOT}/helpers")
target_compile_definitions(backup_test_helpers INTERFACE
    TEST_OUTPUT_ROOT="${TEST_ROOT}/output/cpp"
)

add_executable(backup_unit_tests
    unit/test_utils.cpp
    unit/test_filter.cpp
    unit/test_incremental_manifest.cpp
    unit/test_user_auth_task.cpp
)
target_link_libraries(backup_unit_tests PRIVATE
    backup_core
    backup_test_helpers
    GTest::gtest_main
)
gtest_discover_tests(backup_unit_tests
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:backup_unit_tests>"
    PROPERTIES
        LABELS "unit"
        TIMEOUT 60
)

add_executable(backup_component_tests
    component/test_archive_crypto.cpp
    component/test_cloud_storage.cpp
)
target_link_libraries(backup_component_tests PRIVATE
    backup_core
    backup_cloud
    backup_test_helpers
    GTest::gtest_main
)
gtest_discover_tests(backup_component_tests
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:backup_component_tests>"
    PROPERTIES
        LABELS "component"
        TIMEOUT 180
        RESOURCE_LOCK filesystem
)

add_executable(backup_integration_tests
    integration/test_backup_restore.cpp
    integration/test_metadata_manifest.cpp
    integration/test_defect_reproduction.cpp
)
target_link_libraries(backup_integration_tests PRIVATE
    backup_core
    backup_cloud
    backup_test_helpers
    GTest::gtest_main
)
gtest_discover_tests(backup_integration_tests
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:backup_integration_tests>"
    PROPERTIES
        LABELS "integration"
        TIMEOUT 240
        RESOURCE_LOCK filesystem
)

add_test(NAME CLI.Flow COMMAND bash "${TEST_ROOT}/cli/test_cli.sh")
set_tests_properties(CLI.Flow PROPERTIES
    LABELS "cli;flow"
    TIMEOUT 240
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK filesystem
)

add_test(NAME API.Core COMMAND bash "${TEST_ROOT}/api/test_api.sh")
set_tests_properties(API.Core PROPERTIES
    LABELS "api;flow"
    TIMEOUT 300
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK backup_server
)

add_test(NAME API.WatchScheduleCloud COMMAND bash "${TEST_ROOT}/api/test_watch_schedule_cloud.sh")
set_tests_properties(API.WatchScheduleCloud PROPERTIES
    LABELS "api;flow;watch;schedule;cloud"
    TIMEOUT 420
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK backup_server
)

add_test(NAME Performance.Basic COMMAND bash "${TEST_ROOT}/performance/test_performance.sh")
set_tests_properties(Performance.Basic PROPERTIES
    LABELS "performance"
    TIMEOUT 420
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK filesystem
)

add_test(NAME Security.Basic COMMAND bash "${TEST_ROOT}/security/test_security.sh")
set_tests_properties(Security.Basic PROPERTIES
    LABELS "security"
    TIMEOUT 300
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK backup_server
)

add_test(NAME Frontend.E2EEnv COMMAND bash "${TEST_ROOT}/frontend/check_e2e_env.sh")
set_tests_properties(Frontend.E2EEnv PROPERTIES
    LABELS "frontend;e2e"
    TIMEOUT 60
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
)

add_test(NAME Frontend.E2ESmoke COMMAND bash "${TEST_ROOT}/frontend/run_e2e_smoke.sh")
set_tests_properties(Frontend.E2ESmoke PROPERTIES
    LABELS "frontend;e2e"
    TIMEOUT 420
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK e2e_8080
)

add_test(NAME Frontend.E2EUISmoke COMMAND bash "${TEST_ROOT}/frontend/run_e2e_ui_smoke.sh")
set_tests_properties(Frontend.E2EUISmoke PROPERTIES
    LABELS "frontend;e2e;ui-smoke"
    TIMEOUT 900
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK e2e_8080
)

add_test(NAME Frontend.E2EFullBusiness COMMAND bash "${TEST_ROOT}/frontend/run_e2e_full_business.sh")
set_tests_properties(Frontend.E2EFullBusiness PROPERTIES
    LABELS "frontend;e2e;full-business"
    TIMEOUT 60
    SKIP_RETURN_CODE 77
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESOURCE_LOCK e2e_8080
    DISABLED TRUE
)
