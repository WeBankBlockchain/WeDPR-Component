from typing import Tuple

from phe import PaillierPublicKey, paillier, EncryptedNumber


class PaillierCodec:
    @staticmethod
    def _int_to_bytes(x):
        return x.to_bytes((x.bit_length() + 7) // 8, 'big')

    @staticmethod
    def _bytes_to_int(x):
        return int.from_bytes(x, 'big')

    @staticmethod
    def encode_enc_key(public_key: PaillierPublicKey) -> bytes:
        return PaillierCodec._int_to_bytes(public_key.n)

    @staticmethod
    def decode_enc_key(public_key_bytes: bytes) -> PaillierPublicKey:
        public_key_n = PaillierCodec._bytes_to_int(public_key_bytes)
        return paillier.PaillierPublicKey(n=public_key_n)

    @staticmethod
    def encode_cipher(cipher: EncryptedNumber, be_secure=True) -> Tuple[bytes, bytes]:
        return PaillierCodec._int_to_bytes(cipher.ciphertext(be_secure=be_secure)), \
            PaillierCodec._int_to_bytes(cipher.exponent)

    @staticmethod
    def decode_cipher(public_key: PaillierPublicKey, ciphertext: bytes, exponent: bytes) -> EncryptedNumber:
        return paillier.EncryptedNumber(public_key,
                                        PaillierCodec._bytes_to_int(
                                            ciphertext),
                                        PaillierCodec._bytes_to_int(exponent))
