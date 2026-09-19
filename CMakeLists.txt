cmake_minimum_required(VERSION 3.20)
project(Tools LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilita PkgConfig para localizar GTK4 no Linux e Windows (MSYS2 / vcpkg)
find_package(PkgConfig REQUIRED)
if(WIN32 AND CMAKE_CROSSCOMPILING)
    pkg_check_modules(GTK4 IMPORTED_TARGET gtk4)
else()
    pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
endif()

# Define diretório global de includes
set(TOOLS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# 1. Módulo Core (Histórico de Clipboard e Atalhos)
# ---------------------------------------------------------------------------
add_library(tools_core STATIC
    src/core/clipboard_manager.cpp
    src/core/shortcut_manager.cpp
    src/core/i18n.cpp
    src/core/keyboard_synthesizer.cpp
    src/core/settings_manager.cpp
)

target_include_directories(tools_core PUBLIC
    ${TOOLS_INCLUDE_DIR}
)

target_link_libraries(tools_core PUBLIC
    PkgConfig::GTK4
)

# ---------------------------------------------------------------------------
# 2. Módulo UI (Janela Flyout Win + V e Estilos)
# ---------------------------------------------------------------------------
add_library(tools_ui STATIC
    src/ui/style.cpp
    src/ui/clipboard_window.cpp
    src/ui/settings_window.cpp
    src/app.cpp
)

target_include_directories(tools_ui PUBLIC
    ${TOOLS_INCLUDE_DIR}
)

target_link_libraries(tools_ui PUBLIC
    tools_core
    PkgConfig::GTK4
)

# ---------------------------------------------------------------------------
# 4. Executáveis da Aplicação
# ---------------------------------------------------------------------------
if(WIN32)
    add_executable(Tools WIN32 src/main.cpp resources/tools.rc)
    add_executable(untitled WIN32 src/main.cpp resources/tools.rc)
    target_compile_definitions(Tools PRIVATE UNICODE _UNICODE)
    target_compile_definitions(untitled PRIVATE UNICODE _UNICODE)
else()
    add_executable(Tools src/main.cpp)
    add_executable(untitled src/main.cpp)
endif()

target_link_libraries(Tools PRIVATE tools_ui)
target_link_libraries(untitled PRIVATE tools_ui)

# ---------------------------------------------------------------------------
# 5. Instalação e Empacotamento CPack
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS Tools
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY resources/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/tools-clipboard
    FILES_MATCHING PATTERN "*.css" PATTERN "*.png" PATTERN "*.ico"
)

set(CPACK_PACKAGE_NAME "tools-clipboard")
set(CPACK_PACKAGE_VENDOR "Luís Andrade Cordeiro")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Gerenciador de Área de Transferência moderno no Linux (Win + V)")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_CONTACT "contato@bnbb.com.br")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://bnbb.com.br")

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luís Andrade Cordeiro <contato@bnbb.com.br>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-4-1 (>= 4.0.0), libc6 (>= 2.34), libstdc++6 (>= 11)")
endif()

include(CPack)



