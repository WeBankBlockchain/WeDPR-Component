import time
import unittest

import numpy as np

from ppc_common.ppc_crypto.ihc_cipher import IhcCipher, IhcCiphertext
from ppc_common.ppc_crypto.ihc_codec import IhcCodec
from ppc_common.ppc_crypto.paillier_cipher import PaillierCipher


class PaillierUtilsTest(unittest.TestCase):

    def test_enc_and_dec_parallel(self):
        paillier = PaillierCipher(key_length=1024)
        inputs = np.random.randint(1, 10001, size=10)

        # start_time = time.time()
        # paillier.encrypt_batch(inputs)
        # end_time = time.time()
        # print("enc:", end_time - start_time, "seconds")

        start_time = time.time()
        ciphers = paillier.encrypt_batch_parallel(inputs)
        end_time = time.time()
        print("enc_p:", end_time - start_time, "seconds")

        start_time = time.time()
        outputs = paillier.decrypt_batch_parallel(ciphers)
        end_time = time.time()
        print("dec_p:", end_time - start_time, "seconds")

        self.assertListEqual(list(inputs), list(outputs))

    def test_ihc_enc_and_dec_parallel(self):
        ihc = IhcCipher(key_length=256)
        try_size = 100000
        inputs = np.random.randint(1, 10001, size=try_size)
        expected = np.sum(inputs)

        start_time = time.time()
        ciphers = ihc.encrypt_batch_parallel(inputs)
        end_time = time.time()
        print(
            f"size:{try_size}, enc_p: {end_time - start_time} seconds, "
            f"average times: {(end_time - start_time) / try_size * 1000 * 1000} us")

        start_time = time.time()
        cipher_start = ciphers[0]
        for i in range(1, len(ciphers)):
            cipher_left = (cipher_start.c_left + ciphers[i].c_left)
            cipher_right = (cipher_start.c_right + ciphers[i].c_right)
            # IhcCiphertext(cipher_left, cipher_right, cipher_start.max_mod)
            IhcCiphertext(cipher_left, cipher_right)
        end_time = time.time()
        print(f"size:{try_size}, add_p raw with class: {end_time - start_time} seconds, average times: {(end_time - start_time)/try_size * 1000 * 1000} us")

        start_time = time.time()
        cipher_start = ciphers[0]
        for i in range(1, len(ciphers)):
            cipher_left = (cipher_start.c_left + ciphers[i].c_left)
            cipher_right = (cipher_start.c_right + ciphers[i].c_right)
            # IhcCiphertext(cipher_left, cipher_right)
        end_time = time.time()
        print(f"size:{try_size}, add_p raw: {end_time - start_time} seconds, average times: {(end_time - start_time)/try_size * 1000 * 1000} us")

        start_time = time.time()
        cipher_start = ciphers[0]
        for i in range(1, len(ciphers)):
            cipher_start = cipher_start + ciphers[i]
        end_time = time.time()
        print(
            f"size:{try_size}, add_p: {end_time - start_time} seconds, "
            f"average times: {(end_time - start_time) / try_size * 1000 * 1000} us")

        start_time = time.time()
        outputs = ihc.decrypt_batch_parallel(ciphers)
        end_time = time.time()
        print(
            f"size:{try_size}, dec_p: {end_time - start_time} seconds, "
            f"average times: {(end_time - start_time) / try_size * 1000 * 1000} us")

        decrypted = ihc.decrypt(cipher_start)
        self.assertListEqual(list(inputs), list(outputs))
        assert decrypted == expected

    def test_ihc_code(self):
        ihc = IhcCipher(key_length=256)
        try_size = 100000
        inputs = np.random.randint(1, 10001, size=try_size)
        start_time = time.time()
        ciphers = ihc.encrypt_batch_parallel(inputs)
        end_time = time.time()
        print(
            f"size:{try_size}, enc_p: {end_time - start_time} seconds, "
            f"average times: {(end_time - start_time) / try_size * 1000 * 1000} us")
        for i in range(0, len(ciphers)):
            cipher: IhcCiphertext = ciphers[i]
            encoded, _ = IhcCodec.encode_cipher(cipher)

            decoded = IhcCodec.decode_cipher(None, encoded, None)
            assert cipher == decoded


if __name__ == '__main__':
    unittest.main()
