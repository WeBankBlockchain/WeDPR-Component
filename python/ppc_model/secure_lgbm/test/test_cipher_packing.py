import time
import unittest
import numpy as np

from ppc_common.ppc_crypto.paillier_cipher import PaillierCipher
from ppc_common.ppc_crypto.paillier_codec import PaillierCodec
from ppc_common.ppc_protos.generated.ppc_model_pb2 import CipherList, ModelCipher


paillier = PaillierCipher()


class TestCipherPacking:

    def test_cipher_list(self):

        data_list = np.random.randint(1, 10001, size=1000)

        start_time = time.time()
        ciphers = paillier.encrypt_batch_parallel(data_list)
        # ciphers = paillier.encrypt_batch(data_list)
        print("enc:", time.time() - start_time, "seconds")

        start_time = time.time()
        enc_data_pb = CipherList()
        enc_data_pb.public_key = PaillierCodec.encode_enc_key(
            paillier.public_key)
        for cipher in ciphers:
            paillier_cipher = ModelCipher()
            paillier_cipher.ciphertext, paillier_cipher.exponent = PaillierCodec.encode_cipher(
                cipher)
            enc_data_pb.cipher_list.append(paillier_cipher)
        print("pack ciphers:", time.time() - start_time, "seconds")

        ciphers2 = []
        for i in range(100):
            ciphers2.append(np.array(ciphers[10*i:10*(i+1)]).sum())

        start_time = time.time()
        enc_data_pb2 = CipherList()
        enc_data_pb2.public_key = PaillierCodec.encode_enc_key(
            paillier.public_key)
        for cipher in ciphers2:
            paillier_cipher2 = ModelCipher()
            paillier_cipher2.ciphertext, paillier_cipher2.exponent = PaillierCodec.encode_cipher(
                cipher, be_secure=False)
            enc_data_pb2.cipher_list.append(paillier_cipher2)
        print("pack ciphers:", time.time() - start_time, "seconds")

        ciphers3 = []
        for i in range(100):
            ciphers3.append(np.array(ciphers[10*i:10*(i+1)]).sum())

        start_time = time.time()
        enc_data_pb3 = CipherList()
        enc_data_pb3.public_key = PaillierCodec.encode_enc_key(
            paillier.public_key)
        for cipher in ciphers3:
            paillier_cipher3 = ModelCipher()
            paillier_cipher3.ciphertext, paillier_cipher3.exponent = PaillierCodec.encode_cipher(
                cipher)
            enc_data_pb3.cipher_list.append(paillier_cipher3)
        print("pack ciphers:", time.time() - start_time, "seconds")


if __name__ == '__main__':
    unittest.main()
