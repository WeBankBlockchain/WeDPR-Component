import binascii

import base64
import random
from Crypto.Cipher import AES
from Crypto.Cipher import PKCS1_v1_5 as Cipher_PKCS1_v1_5
from Crypto.PublicKey import RSA

from ppc_common.config import CONFIG_DATA

try:
    import gmpy2

    IS_GMP = True
except ImportError:
    IS_GMP = False

_USE_MOD_GMP_SIZE = (1 << (8 * 2))
_USE_MULMOD_GMP_SIZE = (1 << 1000)

RSA_PUBLIC_HEADER = "-----BEGIN PUBLIC KEY-----"
RSA_PUBLIC_END = "-----END PUBLIC KEY-----"

RSA_PRIVATE_HEADER = "-----BEGIN RSA PRIVATE KEY-----"
RSA_PRIVATE_END = "-----END RSA PRIVATE KEY-----"


def powmod(a, b, c):
    if a == 1:
        return 1
    if not IS_GMP or max(a, b, c) < _USE_MOD_GMP_SIZE:
        return pow(a, b, c)
    else:
        return int(gmpy2.powmod(a, b, c))


def mulmod(a, b, c):
    if not IS_GMP or max(a, b, c) < _USE_MULMOD_GMP_SIZE:
        return a * b % c
    else:
        a, b, c = gmpy2.mpz(a), gmpy2.mpz(b), gmpy2.mpz(c)
        return int(gmpy2.mod(gmpy2.mul(a, b), c))


DEFAULT_KEYSIZE = 1024
DEFAULT_G = 9020881489161854992071763483314773468341853433975756385639545080944698236944020124874820917267762049756743282301106459062535797137327360192691469027152272
DEFAULT_N = 102724610959913950919762303151320427896415051258714708724768326174083057407299433043362228762657118029566890747043004760241559786931866234640457856691885212534669604964926915306738569799518792945024759514373214412797317972739022405456550476153212687312211184540248262330559143446510677062823907392904449451177
DEFAULT_FI = 102724610959913950919762303151320427896415051258714708724768326174083057407299433043362228762657118029566890747043004760241559786931866234640457856691885192126363163670343672910761259882348623401714459980712242233796355982147797162316532450768783823909695360736554767341443201861573989081253763975895939627220

# OUTPUT_BIT_LENGTH = 128
OUTPUT_BIT_LENGTH = CONFIG_DATA['MPC_BIT_LENGTH']
DEFAULT_MPC_N = pow(2, OUTPUT_BIT_LENGTH)


# DEFAULT_MPC_N = DEFAULT_N

def ot_base_pown(value):
    return powmod(DEFAULT_G, value, DEFAULT_N)


def ot_pown(base, value):
    return powmod(base, value, DEFAULT_N)


def ot_mul_fi(a_val, b_val):
    return mulmod(a_val, b_val, DEFAULT_FI)


def ot_mul_n(a_val, b_val):
    return mulmod(a_val, b_val, DEFAULT_N)


# def ot_add(a_val, b_val):
#     return (a_val + b_val)%DEFAULT_N


def ot_str_to_int(input_str):
    return int.from_bytes(input_str.encode('utf-8'), 'big'), len(input_str)


def ot_int_to_str(input_int, len_input_str):
    # len_int = len(str(input_int))//2
    result = input_int.to_bytes(len_input_str, 'big').decode('utf-8')
    # TODO: check all 0x00 valid
    # if result[0].encode('ascii') == b'\x00':
    #     result = result[1:]
    return result


def get_random_int():
    return random.SystemRandom().randrange(1, DEFAULT_N)


def make_rsa_decrypt(encrypted_hex, private_key):
    if RSA_PRIVATE_HEADER not in private_key:
        private_key = RSA_PRIVATE_HEADER + "\n" + private_key
    if RSA_PRIVATE_END not in private_key:
        private_key = private_key + "\n" + RSA_PRIVATE_END
    rsa_private_key = RSA.importKey(private_key)
    encrypted_text = binascii.unhexlify(encrypted_hex)
    cipher = Cipher_PKCS1_v1_5.new(rsa_private_key)
    decrypted_text = cipher.decrypt(encrypted_text, None)
    return decrypted_text.decode('utf-8')


class AESCipher:

    def __init__(self, key):
        # self.key = bytes(key, 'utf-8')
        self.key = key

    def encrypt(self, raw):
        raw = pad(raw)
        iv = "encryptionIntVec".encode('utf-8')
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        return base64.b64encode(cipher.encrypt(raw))

    def decrypt(self, enc):
        iv = "encryptionIntVec".encode('utf-8')
        enc = base64.b64decode(enc)
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        return unpad(cipher.decrypt(enc)).decode('utf8')


BS = 16
def pad(s): return bytes(s + (BS - len(s) %
                              BS) * chr(BS - len(s) % BS), 'utf-8')


def unpad(s): return s[0:-ord(s[-1:])]
