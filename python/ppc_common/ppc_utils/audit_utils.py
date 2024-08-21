# -*- coding: utf-8 -*-
import getopt
import sys

from ppc_common.ppc_utils import utils
from ppc_common.ppc_utils.utils import CryptoType

AUDIT_KEYS = ["agency_name", "input_dataset_hash",
              "psi_input_hash", "psi_output_hash",
              "mpc_result_hash"]


def parse_parameter(argv):
    file_path = 0
    data_hash_value = 0
    crypto_type = None
    try:
        opts, args = getopt.getopt(
            argv, "hf:v:c:", ["file_path=", "data_hash_value="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    if len(opts) == 0:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            usage()
            sys.exit(0)
        elif opt in ("-f", "--file_path"):
            file_path = arg
        elif opt in ("-v", "--data_hash_value"):
            data_hash_value = arg
        elif opt in ("-c", "--crypto_type"):
            crypto_type = arg
        else:
            usage()
            sys.exit(2)
    return file_path, data_hash_value, crypto_type


def usage():
    print('audit.py -f <file_path> -v <data_hash_value> -c crypto_type')
    print('usage:')
    print('     -f <file_path>  Notice:The file will be hashed to audit.')
    print('     -v <data_hash_value>  Notice:The dataset hash value will be compared to audit.')
    print(
        '     -c <crypto_type>  Notice:The crypto type[ECDSA or GM] will be used.')


if __name__ == '__main__':
    file_path, data_hash_value, crypto_type = parse_parameter(sys.argv[1:])
    file_data_hash = utils.make_hash_from_file_path(
        file_path, CryptoType[crypto_type])
    print(f'The file hash is:{file_data_hash}')
    print(f'Audit result is:{file_data_hash == data_hash_value}')
