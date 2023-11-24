SET( TCLAP_SOURCES
        ../ArgBase.h
        ../ArgSwitch.h
        ../ArgSwitchMulti.h
        ../ArgUnlabeledValues.h
        ../ArgValue.h
        ../ArgValueMulti.h
        ../CmdLine.h
        ../CmdLineBase.h
        ../ConstraintBase.h
        ../ConstraintValues.h
        ../ContainerBase.h
        ../Exception.h
        ../GroupBase.h
        ../Groups.h
        ../Output_fmt.h
        ../OutputBase.h )

file(GLOB TCLAP_TEST_SOURCES *.cpp)
foreach (TEST_SOURCE ${TCLAP_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WLE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${TCLAP_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE C:/src/smf_tools/ext)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)
    target_link_libraries(${TEST_NAME} PRIVATE spdlog::spdlog -static)
    target_link_options(${TEST_NAME} PUBLIC "LINKER:--allow-multiple-definition")

    add_test( NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}> )
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${TEST_NAME}" WILL_FAIL TRUE )
endforeach ()

# == Help Tests ==
add_test( NAME tclap COMMAND $<TARGET_FILE:tclap-testing> )
set_tests_properties( tclap PROPERTIES LABELS "tclap" WILL_FAIL TRUE )

add_test( NAME tclap_version COMMAND $<TARGET_FILE:tclap-testing> --version )
set_tests_properties( tclap_version PROPERTIES LABELS "tclap version" )

add_test( NAME tclap_h COMMAND $<TARGET_FILE:tclap-testing> -h )
set_tests_properties( tclap_h PROPERTIES LABELS "tclap help" )

add_test( NAME tclap_help COMMAND $<TARGET_FILE:tclap-testing> --help )
set_tests_properties( tclap_help PROPERTIES LABELS "tclap help" )

add_test( NAME tclap_unknown COMMAND $<TARGET_FILE:tclap-testing> --beans )
set_tests_properties( tclap_unknown PROPERTIES LABELS "tclap unknown" WILL_FAIL TRUE )

add_test( NAME tclap_nonoption COMMAND $<TARGET_FILE:tclap-testing> random.txt )
set_tests_properties( tclap_nonoption PROPERTIES LABELS "tclap nonoption" WILL_FAIL TRUE)

# Switch
#   Combinations: -abc | -bac | -a -b -c | -ab -c | -a -bc | -a --b-name -c | --a-name -b -c etc.
# SwitchMulti
#   Combinations: -aaa | -a -a -a | -a -aa | -aa -a | --a-name -a-name | -a -aa -a-name
# Value
#   Combinations: -a <missing> | -a <correct> | -a <invalid> | --a-name <missing> | --a-name <correct> --a-name <invalid>
# ValueMulti
# UnlabeledValues
#   Combinations: <blank> | <single> | <multiple> | "quoted text with spaces" | 'single quoted with spaces' | "mixed 'quoted with' spaces"
# Constraints
#   OneOf - List of items it must match to be considered valid
#       Combinations: <blank> | <matching> | <non matching>
# Visitors

# If I can come up with some form of combination process, I can generate the test output.
# Like the fact that an argument can have either its short form or its long form.
# An argument can be specified multiple times
# Short Form repetition
# Argument or not, or multiple arguments.

# Correct Cases
# -a | -aaa | -a value | -a value value value | -a value -a value
# --a-name | --a-name value | --a-name value value value | --a-name value --a-name value

# where values can be typical integer or floating point, etc. the simple types.
# Or as a string:
# "value" | "this is a single value" | 'value' | 'this is a single value' | "single 'inside' double" | 'double "inside" single'

# Degenerate Cases
# -aaa value | -aaa -a value | -aaa --a-name value


SET( ARG_VALUES "\"double\"" "\"double spaces\"" "'single'" "'single spaces'" "\"single 'in' double\"" "'double \"in\" single'" )
SET( ARG_NAMES "Double" "DoubleSpaces" "Single" "SingleSpaces" "SingleInDouble" "DoubleInSingle" )

foreach(ARG_VALUE ARG_NAME IN ZIP_LISTS ARG_VALUES ARG_NAMES)
    SET( ARG_VARIANTS
            "-a"
            "-aaa"
            "-a -a -a"
            "--a-name"
            "--a-name --a-name"
            "-a -aaa --a-name"
            "-a ${ARG_VALUE}"
            "-a ${ARG_VALUE} ${ARG_VALUE}"
            "-a ${ARG_VALUE} -a ${ARG_VALUE}"
            "--a-name ${ARG_VALUE}"
            "--a-name ${ARG_VALUE} --a-name ${ARG_VALUE}"
            "-a ${ARG_VALUE} --a-name ${ARG_VALUE}"
            "-a ${ARG_VALUE} ${ARG_VALUE} --a-name ${ARG_VALUE} ${ARG_VALUE}")
    set( VAR a )
    foreach( ARG_VARIANT ${ARG_VARIANTS})
        list( LENGTH VAR INDEX )
        add_test( NAME "test01_${INDEX}_${ARG_NAME}" COMMAND $<TARGET_FILE:tclap_01> ${ARG_VARIANT}  )
        list( APPEND VAR a)
    endforeach ()
endforeach()