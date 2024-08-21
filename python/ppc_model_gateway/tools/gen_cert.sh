#!/bin/bash

print_usage() {
    echo "Usage: $0 [--ca] [--node <number>] [-h|--help]"
    echo "  --ca                    Generate CA key and certificate"
    echo "  --node <number>         Generate <number> sets of certificates and place them in nodeX directories"
    echo "  -h, --help              Display this help message"
}

generate_ca() {
    echo "Generating CA key and certificate..."
    # 生成CA私钥
    openssl genrsa -out ca.key 2048
    # 生成自签名的CA证书
    openssl req -x509 -new -nodes -key ca.key -sha256 -days 36500 -out ca.crt -subj "/CN=PPCS CA"
}

generate_node_certificates() {
    local number=$1
    echo "Generating $number sets of node certificates..."
    for ((i=1; i<=$number; i++)); do
        # 生成节点私钥
        openssl genrsa -out node.key 2048
        # 生成证书签署请求(CSR)
        openssl req -new -key node.key -out node.csr -subj "/CN=PPCS MODEL GATEWAY"
        # 使用CA证书签署CSR以生成节点证书
        openssl x509 -req -in node.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out node.crt -days 36500 -sha256
        # 清理CSR文件
        rm node.csr
        # 创建节点目录并将证书移入
        mkdir -p "node$i"
        mv node.key node.crt "node$i/"
        echo "Generated certificate set $i and placed in node$i directory"
    done
}

# 检查参数
if [[ $# -eq 0 ]]; then
    echo "Error: No arguments provided."
    print_usage
    exit 1
fi

while [[ "$1" != "" ]]; do
    case $1 in
        --ca)
            generate_ca
            ;;
        --node)
            shift
            if [[ $1 =~ ^[0-9]+$ ]]; then
                generate_node_certificates $1
            else
                echo "Error: --node argument expects a number."
                print_usage
                exit 1
            fi
            ;;
        -h | --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Error: Invalid argument: $1"
            print_usage
            exit 1
            ;;
    esac
    shift
done
