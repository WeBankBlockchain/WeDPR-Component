from typing import Tuple

from ppc_common.ppc_crypto.ihc_cipher import IhcCiphertext


class IhcCodec:
    @staticmethod
    def _int_to_bytes(x):
        return x.to_bytes((x.bit_length() + 7) // 8, 'big')

    @staticmethod
    def _bytes_to_int(x):
        return int.from_bytes(x, 'big')

    @staticmethod
    def encode_enc_key(public_key) -> bytes:
        return bytes()

    @staticmethod
    def decode_enc_key(public_key_bytes) -> bytes:
        return bytes()

    @staticmethod
    def encode_cipher(cipher: IhcCiphertext, be_secure=True) -> Tuple[bytes, bytes]:
        return cipher.encode(), bytes()

    @staticmethod
    def decode_cipher(enc_key, ciphertext: bytes, exponent) -> IhcCiphertext:
        return IhcCiphertext.decode(ciphertext)
