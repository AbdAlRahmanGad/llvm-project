set(LLVM_LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
        Core
        Option
        Support
        TargetParser
)

add_clang_tool(cpp-modules-from-driver
        CPPModulesFromDriver.cpp

        DEPENDS
        GENERATE_DRIVER
)

set(CPP_MODULES_FROM_DRIVER_LIB_DEPS
        clangAST
        clangBasic
        clangDependencyScanning
        clangDriver
        clangFrontend
        clangLex
        clangSerialization
        clangTooling
)

clang_target_link_libraries(cpp-modules-from-driver
        PRIVATE
        ${CPP_MODULES_FROM_DRIVER_LIB_DEPS}
)
