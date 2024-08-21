from ppc_common.ppc_crypto.ihc_cipher import IhcCipher
from ppc_common.ppc_crypto.ihc_codec import IhcCodec
from ppc_common.ppc_crypto.paillier_cipher import PaillierCipher
from ppc_common.ppc_crypto.paillier_codec import PaillierCodec


class PheCipherFactory(object):

    @staticmethod
    def build_phe(homo_algorithm=0, key_length=2048):
        if homo_algorithm == 0:
            return IhcCipher()
        if homo_algorithm == 1:
            return PaillierCipher(key_length)
        else:
            raise ValueError("Unsupported homo algorithm")

    @staticmethod
    def build_codec(homo_algorithm=0):
        if homo_algorithm == 0:
            return IhcCodec()
        if homo_algorithm == 1:
            return PaillierCodec()
        else:
            raise ValueError("Unsupported homo algorithm")
