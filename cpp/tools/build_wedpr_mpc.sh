#!/bin/bash

dirpath="$(cd "$(dirname "$0")" && pwd)"
binary_name="wedpr-mpc"
binary_path=
output_dir="wedpr-mpc-node"

listen_ip="0.0.0.0"

cdn_link_header="https://osp-1257653870.cos.ap-guangzhou.myqcloud.com/FISCO-BCOS"
OPENSSL_CMD="${HOME}/.fisco/tassl-1.1.1b"

ca_dir=""
cert_conf=""
sm_cert_conf='sm_cert.cnf'
sm2_params="sm_sm2.param"
sm_mode="false"
days=36500
rsa_key_length=2048

default_version="v1.1.0"
compatibility_version=${default_version}
command="deploy"


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

dir_must_exists() {
    if [ ! -d "$1" ]; then
        LOG_FATAL "$1 DIR does not exist, please check!"
    fi
}

get_value() {
    local var_name=${1}
    var_name=var_${var_name//./}
    local res=$(eval echo '$'"${var_name}")
    echo ${res}
}

set_value() {
    local var_name=${1}
    var_name=var_${var_name//./}
    local var_value=${2}
    eval "${var_name}=${var_value}"
}

dir_must_not_exists() {
    if [  -d "$1" ]; then
        LOG_FATAL "$1 DIR already exist, please check!"
    fi
}

file_must_not_exists() {
    if [ -f "$1" ]; then
        LOG_FATAL "$1 file already exist, please check!"
    fi
}

file_must_exists() {
    if [ ! -f "$1" ]; then
        LOG_FATAL "$1 file does not exist, please check!"
    fi
}

check_env() {
    if [ "$(uname)" == "Darwin" ];then
        macOS="macOS"
    fi
    if [ "$(uname -m)" != "x86_64" ];then
        x86_64_arch="false"
    fi
}
check_name() {
    local name="$1"
    local value="$2"
    [[ "$value" =~ ^[a-zA-Z0-9._-]+$ ]] || {
        LOG_FATAL "$name name [$value] invalid, it should match regex: ^[a-zA-Z0-9._-]+\$"
    }
}
check_and_install_tassl(){
    if [ -f "${OPENSSL_CMD}" ];then
        return
    fi
    # https://en.wikipedia.org/wiki/Uname#Examples
    local x86_64_name="x86_64"
    local arm_name="aarch64"
    local tassl_mid_name="linux"
    if [[ -n "${macOS}" ]];then
        x86_64_name="x86_64"
        arm_name="arm64"
        tassl_mid_name="macOS"
    fi

    local tassl_post_fix="x86_64"
    local platform="$(uname -m)"
    if [[ "${platform}" == "${arm_name}" ]];then
        tassl_post_fix="aarch64"
    elif [[ "${platform}" == "${x86_64_name}" ]];then
        tassl_post_fix="x86_64"
    else
        LOG_FATAL "Unsupported platform ${platform} for ${tassl_mid_name}"
        exit 1
    fi
    local tassl_package_name="tassl-1.1.1b-${tassl_mid_name}-${tassl_post_fix}"
    local tassl_tgz_name="${tassl_package_name}.tar.gz"
    local tassl_link_prefix="${cdn_link_header}/FISCO-BCOS/tools/tassl-1.1.1b/${tassl_tgz_name}"
    LOG_INFO "Downloading tassl binary from ${tassl_link_prefix}..."
    wget --no-check-certificate  "${tassl_link_prefix}"
    tar zxvf ${tassl_tgz_name} && rm ${tassl_tgz_name}
    chmod u+x ${tassl_package_name}
    mkdir -p "${HOME}"/.fisco
    mv ${tassl_package_name} "${HOME}"/.fisco/tassl-1.1.1b
}


generate_sm_sm2_param() {
    local output=$1
    cat << EOF > ${output}
-----BEGIN EC PARAMETERS-----
BggqgRzPVQGCLQ==
-----END EC PARAMETERS-----

EOF
}

generate_sm_cert_conf() {
    local output=$1
    cat <<EOF >"${output}"
oid_section		= new_oids

[ new_oids ]
tsa_policy1 = 1.2.3.4.1
tsa_policy2 = 1.2.3.4.5.6
tsa_policy3 = 1.2.3.4.5.7

####################################################################
[ ca ]
default_ca	= CA_default		# The default ca section

####################################################################
[ CA_default ]

dir		= ./demoCA		# Where everything is kept
certs		= $dir/certs		# Where the issued certs are kept
crl_dir		= $dir/crl		# Where the issued crl are kept
database	= $dir/index.txt	# database index file.
#unique_subject	= no			# Set to 'no' to allow creation of
					# several ctificates with same subject.
new_certs_dir	= $dir/newcerts		# default place for new certs.

certificate	= $dir/cacert.pem 	# The CA certificate
serial		= $dir/serial 		# The current serial number
crlnumber	= $dir/crlnumber	# the current crl number
					# must be commented out to leave a V1 CRL
crl		= $dir/crl.pem 		# The current CRL
private_key	= $dir/private/cakey.pem # The private key
RANDFILE	= $dir/private/.rand	# private random number file

x509_extensions	= usr_cert		# The extensions to add to the cert

name_opt 	= ca_default		# Subject Name options
cert_opt 	= ca_default		# Certificate field options

default_days	= 36500			# how long to certify for
default_crl_days= 30			# how long before next CRL
default_md	= default		# use public key default MD
preserve	= no			# keep passed DN ordering

policy		= policy_match

[ policy_match ]
countryName		= match
stateOrProvinceName	= match
organizationName	= match
organizationalUnitName	= optional
commonName		= supplied
emailAddress		= optional

[ policy_anything ]
countryName		= optional
stateOrProvinceName	= optional
localityName		= optional
organizationName	= optional
organizationalUnitName	= optional
commonName		= supplied
emailAddress		= optional

####################################################################
[ req ]
default_bits		= 2048
default_md		= sm3
default_keyfile 	= privkey.pem
distinguished_name	= req_distinguished_name
x509_extensions	= v3_ca	# The extensions to add to the self signed cert

string_mask = utf8only

# req_extensions = v3_req # The extensions to add to a certificate request

[ req_distinguished_name ]
countryName = CN
countryName_default = CN
stateOrProvinceName = State or Province Name (full name)
stateOrProvinceName_default =GuangDong
localityName = Locality Name (eg, city)
localityName_default = ShenZhen
organizationalUnitName = Organizational Unit Name (eg, section)
organizationalUnitName_default = fisco
commonName =  Organizational  commonName (eg, fisco)
commonName_default =  fisco
commonName_max = 64

[ usr_cert ]
basicConstraints=CA:FALSE
nsComment			= "OpenSSL Generated Certificate"

subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer

[ v3_req ]

# Extensions to add to a certificate request

basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature

[ v3enc_req ]

# Extensions to add to a certificate request
basicConstraints = CA:FALSE
keyUsage = keyAgreement, keyEncipherment, dataEncipherment

[ v3_agency_root ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
basicConstraints = CA:true
keyUsage = cRLSign, keyCertSign

[ v3_ca ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
basicConstraints = CA:true
keyUsage = cRLSign, keyCertSign

EOF
}

generate_cert_conf() {
    local output=$1
    cat <<EOF >"${output}"
[ca]
default_ca=default_ca
[default_ca]
default_days = 36500
default_md = sha256

[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req
[req_distinguished_name]
countryName = CN
countryName_default = CN
stateOrProvinceName = State or Province Name (full name)
stateOrProvinceName_default =GuangDong
localityName = Locality Name (eg, city)
localityName_default = ShenZhen
organizationalUnitName = Organizational Unit Name (eg, section)
organizationalUnitName_default = FISCO-BCOS
commonName =  Organizational  commonName (eg, FISCO-BCOS)
commonName_default = FISCO-BCOS
commonName_max = 64

[ v3_req ]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment

[ v4_req ]
basicConstraints = CA:TRUE

EOF
}
print_result() {
    echo "=============================================================="
    LOG_INFO "${binary_name} path      : ${binary_path}"
    LOG_INFO "SM model             : ${sm_mode}"
    LOG_INFO "Output dir           : ${output_dir}"
    LOG_INFO "All completed. Files in ${output_dir}"
}

help() {
 echo $1
    cat <<EOF
Usage:
    -e <wedpr-mpc exec>                 [Optional] wedpr-mpc binary exec
    -o <output dir>                     [Optional] output directory, default ./nodes
    -s <SM model>                       [Optional] SM SSL connection or not, default is false
    -h Help

deploy nodes e.g
    bash $0 -o wedpr-mpc-node -e ./wedpr-mpc
    bash $0 -o wedpr-mpc-node -e ./wedpr-mpc -s
EOF
    exit 0
}

# generate the config.ini
generate_config_ini() {
    local output="${1}"

    local rpc_listen_ip="${2}"
    local rpc_listen_port="${3}"
    local agency_info="${4}"
    local agency_id="${5}"
    local grpc_listen_ip="${6}"
    local grpc_listen_port="${7}"
    local nodeid="${8}"

    cat <<EOF >"${output}"
[agency]
    ; the agency-id of self-party
    id = ${agency_id}
    ; the agency info

[mpc]
    dataset_hdfs_path = /user/ppc/
    job_path = /data/app/ppc/mpc-job/
    mpc_root_path = /ppc/scripts/ppc-mpc/
    mpc_root_path_no_gateway = /ppc/scripts/ppc-mpc-no-gateway/
    read_per_batch_lines = 100000

[crypto]
    sm_crypto = ${sm_mode}

[rpc]
    listen_ip=${rpc_listen_ip}
    listen_port=${rpc_listen_port}
    token = ppcs_psi_apikey
    thread_count=4
    ; ssl or sm ssl
    sm_ssl=${sm_mode}
    ; ssl connection switch, if disable the ssl connection, default: false
    ;disable_ssl = true

[hdfs_storage]
    ; the hdfs configuration
    user = root
    name_node = 127.0.0.1
    name_node_port = 9900
    token =
    ; enable replace-datanode-on-failure or not
    replace-datanode-on-failure = false
    ; the connection-timeout, in ms, default is 1000ms
    connection-timeout = 2000
    ; enable auth or not, default is false
    ; enable_krb5_auth = false
    ; the hdfs kerberos auth principal, used when enable_krb5_auth
    ; auth_principal = root@NODE.DC1.CONSUL
    ; the hdfs kerberos auth password, used when enable_krb5_auth
    ; auth_password =
    ; the ccache path, used when enable_krb5_auth
    ; ccache_path = /tmp/krb5cc_ppc_node
    ; the krb5.conf path
    ; krb5_conf_path  = conf/krb5.conf

[transport]
   ; the endpoint information
   listen_ip = ${grpc_listen_ip}
   listen_port = ${grpc_listen_port}
   host_ip = 
   ; the threadPoolSize
   thread_count = 4
   ; the gatewayService endpoint information
   gateway_target =  
   ; the components
   components =
   nodeid=${nodeid}

[cert]
    ; directory the certificates located in
    cert_path=./conf

[log]
    enable=true
    ; print the log to std::cout or not, default print to the log files
    enable_console_output = false
    log_path=./log
    ; info debug trace
    level=info
    ; MB
    max_log_file_size=200
EOF
}

generate_krb5_file_template()
{
  local filepath=$1
  mkdir -p $(dirname $filepath)   
   cat << EOF > "${filepath}"
[libdefaults]
 default_realm = NODE.DC1.CONSUL
 dns_lookup_realm = false
 dns_lookup_kdc = false
 ticket_lifetime = 24h
 renew_lifetime = 7d
 forwardable = true

[realms]
 NODE.DC1.CONSUL = {
  kdc = 
  admin_server =
 }

[domain_realm]
 .node.dc1.consul = NODE.DC1.CONSUL
 node.dc1.consul = NODE.DC1.CONSUL
EOF
}

generate_script_template()
{
    local filepath=$1
    mkdir -p $(dirname $filepath)
    cat << EOF > "${filepath}"
#!/bin/bash
SHELL_FOLDER=\$(cd \$(dirname \$0);pwd)

LOG_ERROR() {
    content=\${1}
    echo -e "\033[31m[ERROR] \${content}\033[0m"
}

LOG_INFO() {
    content=\${1}
    echo -e "\033[32m[INFO] \${content}\033[0m"
}

EOF
    chmod +x ${filepath}
}

# TODO: support docker-mode
generate_node_scripts() {
    local output=${1}
    local ps_cmd="\$(ps aux|grep \${ppc_mpc}|grep -v grep|awk '{print \$2}')"
    local start_cmd="nohup \${ppc_mpc} -c config.ini >>nohup.out 2>&1 &"
    local stop_cmd="kill \${node_pid}"
    local pid="pid"
    local log_cmd="tail -n20  nohup.out"
    local check_success="\$(${log_cmd} | grep running)"
    generate_script_template "$output/start.sh"
    cat <<EOF >> "${output}/start.sh"
ppc_mpc=\${SHELL_FOLDER}/${binary_name}
cd \${SHELL_FOLDER}
node=\$(basename \${SHELL_FOLDER})
node_pid=${ps_cmd}
if [ ! -z \${node_pid} ];then
    echo " \${node} is running, ${pid} is \$node_pid."
    exit 0
else
    ${start_cmd}
    sleep 1.5
fi
try_times=4
i=0
while [ \$i -lt \${try_times} ]
do
    node_pid=${ps_cmd}
    success_flag=${check_success}
    if [[ ! -z \${node_pid} && ! -z "\${success_flag}" ]];then
        echo -e "\033[32m \${node} start successfully pid=\${node_pid}\033[0m"
        exit 0
    fi
    sleep 0.5
    ((i=i+1))
done
echo -e "\033[31m  Exceed waiting time. Please try again to start \${node} \033[0m"
${log_cmd}
EOF
    chmod u+x "${output}/start.sh"
    generate_script_template "$output/stop.sh"
    cat <<EOF >> "${output}/stop.sh"
ppc_mpc=\${SHELL_FOLDER}/${binary_name}
node=\$(basename \${SHELL_FOLDER})
node_pid=${ps_cmd}
try_times=10
i=0
if [ -z \${node_pid} ];then
    echo " \${node} isn't running."
    exit 0
fi
[ ! -z \${node_pid} ] && ${stop_cmd} > /dev/null
while [ \$i -lt \${try_times} ]
do
    sleep 1
    node_pid=${ps_cmd}
    if [ -z \${node_pid} ];then
        echo -e "\033[32m stop \${node} success.\033[0m"
        exit 0
    fi
    ((i=i+1))
done
echo "  Exceed maximum number of retries. Please try again to stop \${node}"
exit 1
EOF
    chmod u+x "${output}/stop.sh"
}

generate_all_node_scripts() {
    local output=${1}
    mkdir -p ${output}

    cat <<EOF >"${output}/start_all.sh"
#!/bin/bash
dirpath="\$(cd "\$(dirname "\$0")" && pwd)"
cd "\${dirpath}"

dirs=(\$(ls -l \${dirpath} | awk '/^d/ {print \$NF}'))
for dir in \${dirs[*]}
do
    if [[ -f "\${dirpath}/\${dir}/config.ini" && -f "\${dirpath}/\${dir}/start.sh" ]];then
        echo "try to start \${dir}"
        bash \${dirpath}/\${dir}/start.sh &
    fi
done
wait
EOF
    chmod u+x "${output}/start_all.sh"

    cat <<EOF >"${output}/stop_all.sh"
#!/bin/bash
dirpath="\$(cd "\$(dirname "\$0")" && pwd)"
cd "\${dirpath}"

dirs=(\$(ls -l \${dirpath} | awk '/^d/ {print \$NF}'))
for dir in \${dirs[*]}
do
    if [[ -f "\${dirpath}/\${dir}/config.ini" && -f "\${dirpath}/\${dir}/stop.sh" ]];then
        echo "try to stop \${dir}"
        bash \${dirpath}/\${dir}/stop.sh
    fi
done
wait
EOF
    chmod u+x "${output}/stop_all.sh"
}

gen_non_sm_ca_cert() {
    if [ ! -f "${cert_conf}" ]; then
        generate_cert_conf "${cert_conf}"
    fi
    local ca_cert_dir="${1}"
    file_must_not_exists "${ca_cert_dir}"/ca.key
    file_must_not_exists "${ca_cert_dir}"/ca.crt
    file_must_exists "${cert_conf}"

    mkdir -p "$ca_cert_dir"
    dir_must_exists "$ca_cert_dir"

    ${OPENSSL_CMD} genrsa -out "${ca_cert_dir}"/ca.key "${rsa_key_length}"
    ${OPENSSL_CMD} req -new -x509 -days "${days}" -subj "/CN=FISCO-BCOS/O=FISCO-BCOS/OU=chain" -key "${ca_cert_dir}"/ca.key -config "${cert_conf}" -out "${ca_cert_dir}"/ca.crt  2>/dev/null
    if [ ! -f "${ca_cert_dir}/cert.cnf" ];then
        mv "${cert_conf}" "${ca_cert_dir}"
    fi
    LOG_INFO "Generate ca cert successfully!"
}

gen_sm_ca_cert() {
    local ca_cert_dir="${1}"
    name=$(basename "$ca_cert_dir")
    check_name chain "$name"

    if [ ! -f "${sm_cert_conf}" ]; then
        generate_sm_cert_conf ${sm_cert_conf}
    fi

    generate_sm_sm2_param "${sm2_params}"

    mkdir -p "$ca_cert_dir"
    dir_must_exists "$ca_cert_dir"

    "$OPENSSL_CMD" genpkey -paramfile "${sm2_params}" -out "$ca_cert_dir/sm_ca.key" 2>/dev/null
    "$OPENSSL_CMD" req -config sm_cert.cnf -x509 -days "${days}" -subj "/CN=wedpr/O=wedpr/OU=ca" -key "$ca_cert_dir/sm_ca.key" -extensions v3_ca -out "$ca_cert_dir/sm_ca.crt" 2>/dev/null
    if [ ! -f "${ca_cert_dir}/${sm_cert_conf}" ];then
        cp "${sm_cert_conf}" "${ca_cert_dir}"
    fi
    if [ ! -f "${ca_cert_dir}/${sm2_params}" ];then
        cp "${sm2_params}" "${ca_cert_dir}"
    fi
}


gen_rsa_node_cert() {
    local capath="${1}"
    local ndpath="${2}"
    local type="${3}"

    file_must_exists "$capath/ca.key"
    file_must_exists "$capath/ca.crt"
    # check_name node "$node"

    file_must_not_exists "$ndpath"/"${type}".key
    file_must_not_exists "$ndpath"/"${type}".crt

    mkdir -p "${ndpath}"
    dir_must_exists "${ndpath}"

    ${OPENSSL_CMD} genrsa -out "${ndpath}"/"${type}".key "${rsa_key_length}" 2>/dev/null
    ${OPENSSL_CMD} req -new -sha256 -subj "/CN=FISCO-BCOS/O=fisco-bcos/OU=agency" -key "$ndpath"/"${type}".key -config "$capath"/cert.cnf -out "$ndpath"/"${type}".csr
    ${OPENSSL_CMD} x509 -req -days "${days}" -sha256 -CA "${capath}"/ca.crt -CAkey "$capath"/ca.key -CAcreateserial \
        -in "$ndpath"/"${type}".csr -out "$ndpath"/"${type}".crt -extensions v4_req -extfile "$capath"/cert.cnf 2>/dev/null

    ${OPENSSL_CMD} pkcs8 -topk8 -in "$ndpath"/"${type}".key -out "$ndpath"/pkcs8_node.key -nocrypt
    cp "$capath"/ca.crt "$capath"/cert.cnf "$ndpath"/

    rm -f "$ndpath"/"$type".csr
    rm -f "$ndpath"/"$type".key

    mv "$ndpath"/pkcs8_node.key "$ndpath"/"$type".key

    LOG_INFO "Generate ${ndpath} cert successful!"
}

# we use sm_param to generate the private key
generate_private_key() {
    local output_path="${1}"
    if [ ! -d "${output_path}" ]; then
        mkdir -p ${output_path}
    fi
    if [ ! -f ${sm2_params} ]; then
        generate_sm_sm2_param ${sm2_params}
    fi
    ${OPENSSL_CMD} genpkey -paramfile ${sm2_params} -out ${output_path}/node.pem 2>/dev/null
    $OPENSSL_CMD ec -in "$output_path/node.pem" -text -noout 2> /dev/null | sed -n '3,5p' | sed 's/://g' | tr "\n" " " | sed 's/ //g'  | cat > "$output_path/node.privateKey"
    ${OPENSSL_CMD} ec -text -noout -in "${output_path}/node.pem" 2>/dev/null | sed -n '7,11p' | tr -d ": \n" | awk '{print substr($0,3);}' | cat >"$output_path"/node.nodeid
    private_key=$(cat $output_path/node.privateKey)
    echo ${private_key}
}

gen_sm_node_cert_with_ext() {
    local capath="$1"
    local certpath="$2"
    local type="$3"
    local extensions="$4"

    file_must_exists "$capath/sm_ca.key"
    file_must_exists "$capath/sm_ca.crt"

    file_must_not_exists "$ndpath/sm_${type}.crt"
    file_must_not_exists "$ndpath/sm_${type}.key"

    "$OPENSSL_CMD" genpkey -paramfile "$capath/${sm2_params}" -out "$certpath/sm_${type}.key" 2> /dev/null
    "$OPENSSL_CMD" req -new -subj "/CN=FISCO-BCOS/O=fisco-bcos/OU=${type}" -key "$certpath/sm_${type}.key" -config "$capath/sm_cert.cnf" -out "$certpath/sm_${type}.csr" 2> /dev/null

    "$OPENSSL_CMD" x509 -sm3 -req -CA "$capath/sm_ca.crt" -CAkey "$capath/sm_ca.key" -days "${days}" -CAcreateserial -in "$certpath/sm_${type}.csr" -out "$certpath/sm_${type}.crt" -extfile "$capath/sm_cert.cnf" -extensions "$extensions" 2> /dev/null

    rm -f "$certpath/sm_${type}.csr"
}

gen_sm_node_cert() {
    local capath="${1}"
    local ndpath="${2}"
    local type="${3}"

    file_must_exists "$capath/sm_ca.key"
    file_must_exists "$capath/sm_ca.crt"

    mkdir -p "$ndpath"
    dir_must_exists "$ndpath"
    local node=$(basename "$ndpath")
    check_name node "$node"

    gen_sm_node_cert_with_ext "$capath" "$ndpath" "${type}" v3_req
    gen_sm_node_cert_with_ext "$capath" "$ndpath" "en${type}" v3enc_req
    #nodeid is pubkey
    $OPENSSL_CMD ec -in "$ndpath/sm_${type}.key" -text -noout 2> /dev/null | sed -n '7,11p' | sed 's/://g' | tr "\n" " " | sed 's/ //g' | awk '{print substr($0,3);}'  | cat > "${ndpath}/sm_${type}.nodeid"
    cp "$capath/sm_ca.crt" "$ndpath"
}

generate_ca_cert() {
    local sm_mode="$1"
    local ca_cert_path="$2"
    LOG_INFO "generate ca cert, sm: ${sm_mode}, ca_cert_path: ${ca_cert_path}"
    mkdir -p "${ca_cert_path}"
    if [[ "${sm_mode}" == "false" ]]; then
        gen_non_sm_ca_cert "${ca_cert_path}"
    else
        gen_sm_ca_cert "${ca_cert_path}"
    fi
    LOG_INFO "generate ca cert successfully, sm: ${sm_mode}, ca_cert_path: ${ca_cert_path}"
}

generate_node_cert() {
    local sm_mode="$1"
    local ca_cert_path="${2}"
    local node_cert_path="${3}"

    mkdir -p ${node_cert_path}
    if [[ "${sm_mode}" == "false" ]]; then
        gen_rsa_node_cert "${ca_cert_path}" "${node_cert_path}" "ssl" 2>&1
    else
        gen_sm_node_cert "${ca_cert_path}" "${node_cert_path}" "ssl" 2>&1
    fi
}


deploy_nodes()
{
    echo "output_dir:${output_dir}"
    dir_must_not_exists "${output_dir}"
    mkdir -p "$output_dir"
    dir_must_exists "${output_dir}"

    # check the binary
    if [[ ! -f "$binary_path" ]]; then
        LOG_FATAL "wedpr-component binary exec ${binary_path} not exist, Must copy binary file ${binary_name} to ${binary_path}"
    fi

    local agency_index=0
    local agency_info=""
    # generate the ca-cert
    ca_dir="${output_dir}"/ca
    cert_conf="${output_dir}/cert.cnf"
    generate_ca_cert "${sm_mode}" "${ca_dir}"
    # start_all.sh and stop_all.sh
    # generate_all_node_scripts "${output_dir}"
    cp "${binary_path}" "${output_dir}"
    # generate cert for the node
    ca_cert_dir="${output_dir}"/ca
    mkdir -p ${ca_cert_dir}
    # cp -r ${ca_dir}/* ${ca_cert_dir}

    # generate the node config
    mkdir -p "${output_dir}"
    # generate the node-cert
    generate_node_cert "${sm_mode}" "${ca_dir}" "${output_dir}/conf"
    # generate the node-script
    generate_node_scripts "${output_dir}"
    # generate the config.ini
    local rpc_port=5894
    local agency_id="agency${count}"
    local grpc_port=18100
    # the nodeid
    private_key=$(generate_private_key "${output_dir}/conf")
    node_id=$(cat "${output_dir}/conf/node.nodeid")
    generate_config_ini "${output_dir}/config.ini"  "${listen_ip}" "${rpc_port}" "${agency_info}" ${agency_id} "${listen_ip}" "${grpc_port}" "${node_id}"
    generate_krb5_file_template "{output_dir}/conf/krb5.conf"
    print_result
}

parse_params() {
    while getopts "o:e:sh" option; do
        case $option in
        o)
            output_dir="$OPTARG"
            ;;
        e)
            binary_path="$OPTARG"
            file_must_exists "${binary_path}"
            ;;
        s) sm_mode="true" ;;
        h) help ;;
        *) help ;;
        esac
    done
}

main() {
    check_env
    check_and_install_tassl
    parse_params "$@"
    deploy_nodes
}
main "$@"