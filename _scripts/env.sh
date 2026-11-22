#!/bin/bash
# Environment function(s) support

# check cc65 compiler

EXEC_PACKAGES_FOLDER=../_install_packages/bin
INSTALL_PACKAGES_FOLDER=../_install_packages/packages

CC65_VERSION=2.19
CC65_INSTALL_PACKAGE_VERSION=cc65-${CC65_VERSION}
LUA_INSTALL_PACKAGE_VERSION=lua-5.3.6

CC65_HOME=${EXEC_PACKAGES_FOLDER}/${CC65_INSTALL_PACKAGE_VERSION}
LUA_HOME=${EXEC_PACKAGES_FOLDER}/${LUA_INSTALL_PACKAGE_VERSION}

export PATH=${CC65_HOME}/bin:${CC65_HOME}/lib:${CC65_HOME}/include:${LUA_HOME}/src:${PATH}

mkdir -p ${EXEC_PACKAGES_FOLDER}
mkdir -p ${INSTALL_PACKAGES_FOLDER}

function check_cc65_env() {
    # The build invokes ca65 and ld65 directly; checking only the cc65
    # compiler driver can pass even when the required assembler/linker are absent.
    local ca65_version=""
    local ld65_version=""
    local cc65_ready=1

    if ! command -v ca65 >/dev/null 2>&1 || ! command -v ld65 >/dev/null 2>&1; then
        cc65_ready=0
    else
        ca65_version=$(ca65 --version 2>&1)
        ld65_version=$(ld65 --version 2>&1)
        [[ "$ca65_version" == *"V\${CC65_VERSION}"* && "$ld65_version" == *"V\${CC65_VERSION}"* ]] || cc65_ready=0
    fi

    # If the required assembler/linker are absent or not the pinned version,
    # install the project's known cc65 toolchain.
    if [ "$cc65_ready" -ne 1 ]; then
        echoerror "Please make sure that your computer has installed cc65! => https://github.com/cc65/cc65"
        read -p "Do you need to install the cc65 compiler?(yes/no): " result
        if [[ $result = [Yy][Ee][Ss] ]] || [[ $result = [Yy] ]]; then
            currentPwd=`pwd`
            echoerror "Your computer has not install [cc65] compiler, wait a moment and prepare install environment..."
            curl -k -o ${INSTALL_PACKAGES_FOLDER}/${CC65_INSTALL_PACKAGE_VERSION}.tar.gz "https://codeload.github.com/cc65/cc65/tar.gz/refs/tags/V${CC65_VERSION}"
            cd ${INSTALL_PACKAGES_FOLDER} && tar xvzf ${CC65_INSTALL_PACKAGE_VERSION}.tar.gz && cd ${CC65_INSTALL_PACKAGE_VERSION}
            Return

            make
            Return

            echoinfo "You have successfully installed [cc65]!"
            cd "${currentPwd}" ;
            mv ${INSTALL_PACKAGES_FOLDER}/${CC65_INSTALL_PACKAGE_VERSION} ${EXEC_PACKAGES_FOLDER}
            hash -r
            ca65 --version
            Return
            ld65 --version
            Return
        else
            echoerror "Please make sure that [cc65] is installed on your computer, otherwise the project cannot be compiled!"
            exit 254
        fi
    fi
}

# check lua vm
function check_lua_env() {
    # The preparation script targets Lua 5.3.x; accepting any "lua" binary
    # can silently select an incompatible system version.
    local lua_version=""
    local lua_ready=1
    if ! command -v lua >/dev/null 2>&1; then
        lua_ready=0
    else
        lua_version=$(lua -v 2>&1)
        [[ "$lua_version" =~ Lua[[:space:]]+5\.3([.]|[[:space:]]|$) ]] || lua_ready=0
    fi

    # If Lua 5.3.x is unavailable, install the pinned 5.3.6 toolchain.
    if [ "$lua_ready" -ne 1 ]; then
        echoerror "Please make sure that your computer has installed lua 5.3.x! => https://www.lua.org/versions.html"
        read -p "Do you need to install the lua VM?(yes/no): " result
        if [[ $result = [Yy][Ee][Ss] ]] || [[ $result = [Yy] ]]; then
            currentPwd=`pwd`
            echoerror "Your computer has not install [lua] compiler, wait a moment and prepare install environment..."
            curl -k -o ${INSTALL_PACKAGES_FOLDER}/${LUA_INSTALL_PACKAGE_VERSION}.tar.gz "https://www.lua.org/ftp/${LUA_INSTALL_PACKAGE_VERSION}.tar.gz"
            cd ${INSTALL_PACKAGES_FOLDER} && tar xvzf ${LUA_INSTALL_PACKAGE_VERSION}.tar.gz && cd ${LUA_INSTALL_PACKAGE_VERSION}
            Return

            case ${OS_VERSION} in
                Darwin)
                    echoinfo "This OS is macos!"

                    make macosx test
                    Return
                ;;
                Linux)
                    echoinfo "This OS is linux!"

                    # install dependency of <readline/readline.h>
                    Installer libreadline-dev readline-devel
                    make linux test
                    Return
                ;;
                #FreeBSD)
                    #;;
            esac

            echoinfo "You have successfully installed [lua]!"
            cd "${currentPwd}" ;
            mv ${INSTALL_PACKAGES_FOLDER}/${LUA_INSTALL_PACKAGE_VERSION} ${EXEC_PACKAGES_FOLDER}
            hash -r
            lua -v
            Return
        else
            echoerror "Please make sure that [lua] is installed on your computer, otherwise the project cannot be compiled!"
            exit 254
        fi
    fi
}
