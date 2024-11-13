#!/bin/bash
os=""
LOG_WARN() {
    local content=${1}
    echo -e "\033[31m[ERROR] ${content}\033[0m"
}

LOG_INFO() {
    local content=${1}
    echo -e "\033[32m[INFO] ${content}\033[0m"
}

LOG_FATAL() {
    local content=${1}
    echo -e "\033[31m[FATAL] ${content}\033[0m"
    exit 1
}

install_gsasl_depend()
{
    os_type="${1}"
    LOG_INFO "check gsasl..."
    gsasl=$(whereis libgsasl|grep "libgsasl.a")
    if [[ ! -z "${gsasl}" ]];then
        LOG_INFO "the libgasal.a has already exists!"
        return
    fi

    LOG_INFO "download and install libgsasl..."
    wget --no-check-certificate https://ftp.gnu.org/gnu/gsasl/libgsasl-1.8.0.tar.gz && tar -xvf libgsasl-1.8.0.tar.gz 
    
    # centos
    if [[ "${os_type}" == "centos" ]];then
        cd libgsasl-1.8.0 && ./configure --with-pic && make -j4 && make install
    fi
    # macos
    if [[ "${os_type}" == "macos" ]];then
        cd libgsasl-1.8.0 && ./configure --with-pic && make -j4 && make install
    fi
    # ubuntu
    if [[ "${os_type}" == "ubuntu" ]];then
        cd libgsasl-1.8.0 && ./configure --with-pic && make -j4 && make install
    fi
    cd .. && rm -rf libgsasl-1.8.0 && rm -rf libgsasl-1.8.0.tar.gz
    df -lh
    LOG_INFO "download and install libgsasl success..."
}

install_nasm_depend()
{
    LOG_INFO "check nasm..."
    nasm=$(nasm --version | awk -F' ' '{print $3}')

    echo "### current nasm: ${nasm}"
    # require the nasm >= 2.15
    if [[ "${nasm}" == "2.15" ]] || [[ "${nasm}" > "2.15" ]];then
        LOG_INFO "the nasm >= 2.15 has already exists!"
        return
    fi
    LOG_INFO "download and install nasm 2.15..."
    wget --no-check-certificate https://www.nasm.us/pub/nasm/releasebuilds/2.15/nasm-2.15.tar.gz && tar -xvf nasm-2.15.tar.gz
    cd nasm-2.15 && ./configure && make -j4 && make install
    cd .. && rm -rf nasm-2.15.tar.gz && rm -rf nasm-2.15
    LOG_INFO "download and install nasm success..."
}

install_binutils_depend()
{
    LOG_INFO "download and install binutils 2.35..."
    wget --no-check-certificate http://ftp.gnu.org/gnu/binutils/binutils-2.35.tar.gz && tar -zxf binutils-2.35.tar.gz
    cd binutils-2.35 && ./configure && make -j4 && make install
}

install_centos_depends()
{
    LOG_INFO "install depends for centos ..."
    # install the basic package
    sudo yum install -y lcov bison flex epel-release centos-release-scl flex bison patch devtoolset-11 rh-perl530-perl cmake3 zlib-devel ccache lcov python-devel python3-devel krb5-devel autoconf
    LOG_INFO "install basic-package-depends for centos success..."
    
    LOG_INFO "install gsasl ... "
    install_gsasl_depend "centos"
    LOG_INFO "install gsasl success!"

    LOG_INFO "install nasm ... "
    install_nasm_depend
    LOG_INFO "install nasm success!"

    LOG_INFO "install depends for centos success ..."
}

install_ubuntu_depends()
{
    LOG_INFO "install depends for ubuntu ..."
    # install the basic package
    sudo apt install -y lcov bison flex g++ libssl-dev openssl cmake git build-essential autoconf texinfo flex patch bison libgmp-dev zlib1g-dev automake pkg-config libtool wget libkrb5-dev autoconf
    LOG_INFO "install basic-package-depends for ubuntu success..."
   
    LOG_INFO "install gsasl ... "
    install_gsasl_depend "ubuntu"
    LOG_INFO "install gsasl success!"

    LOG_INFO "install nasm ... "
    install_nasm_depend
    LOG_INFO "install nasm success!"

    LOG_INFO "install binutils ... "
    install_binutils_depend
    LOG_INFO "install binutils success!"

    LOG_INFO "install depends for ubuntu success ..."
}

install_iconv_depend()
{
    LOG_INFO "download and install libiconv..."
    wget --no-check-certificate https://mirrors.tuna.tsinghua.edu.cn/gnu/libiconv/libiconv-1.16.tar.gz && tar -xvf libiconv-1.16.tar.gz
    
    cd libiconv-1.16 && ./configure --with-pic && make -j4 && make install
    cd .. && rm -rf libiconv-1.16*
    df -lh
    LOG_INFO "download and install libiconv success..."
}

install_macos_depends()
{
    LOG_INFO "install depends for macos ..."
    brew install autoconf nasm lcov iconv
    install_gsasl_depend "macos"
    install_iconv_depend
    LOG_INFO "install depends for macos success ..."
}

help() {
 echo $1
    cat <<EOF
Usage:
    -o <os>                        [Optional] the os to install depends, support 'centos', 'ubuntu' and 'macos' now, default is empty
    -h Help

e.g:
    bash $0 -o centos
    bash $0 -o ubuntu
    bash $0 -o macos
EOF
    exit 0
}

parse_params() {
    while getopts "o:h" option; do
        case $option in
        o) os="${OPTARG}"
            ;;
        h) help ;;
        *) help ;;
        esac
    done
}
main() {
    parse_params "$@"
    if [[ -z \${os} ]];then
        help
    fi
    if [ "${os}" == "centos" ];then
        install_centos_depends
    fi
    if [ "${os}" == "ubuntu" ];then
        install_ubuntu_depends
    fi
    if [ "${os}" == "macos" ];then
        install_macos_depends
    fi
}
main "$@"
