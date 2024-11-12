import time
import unittest

import numpy as np

from ppc_common.ppc_crypto.ihc_cipher import IhcCipher, IhcCiphertext
from ppc_common.ppc_crypto.ihc_codec import IhcCodec
from ppc_common.ppc_crypto.paillier_cipher import PaillierCipher


class HomoTest:
    def __init__(self, ut):
        self.paillier = PaillierCipher(key_length=1024)
        self.ihc_cipher = IhcCipher(key_length=1024)
        self.ut = ut

    def test_enc_and_dec_parallel(self,  homo_impl, test_size, start, end):
        inputs = np.random.randint(start, end, size=test_size)

        # start_time = time.time()
        # paillier.encrypt_batch(inputs)
        # end_time = time.time()
        # print("enc:", end_time - start_time, "seconds")

        start_time = time.time()
        ciphers = homo_impl.encrypt_batch_parallel(inputs)
        end_time = time.time()
        print("enc_p:", end_time - start_time, "seconds")

        start_time = time.time()
        outputs = homo_impl.decrypt_batch_parallel(ciphers)
        end_time = time.time()
        print("dec_p:", end_time - start_time, "seconds")

        self.ut.assertListEqual(list(inputs), list(outputs))
        self.test_homo_mul_enc_and_dec(homo_impl, ciphers, inputs, 10)
        # test add and enc dec
        inputs2 = np.random.randint(start, end, size=test_size)
        ciphers2 = homo_impl.encrypt_batch_parallel(inputs2)
        self.test_homo_add_enc_and_desc(
            homo_impl, ciphers, ciphers2, inputs, inputs2)

    def test_homo_mul_enc_and_dec(self, homo_impl, ciphers, inputs, mul_value):
        start_time = time.time()
        mul_ciphers = []
        for cipher in ciphers:
            mul_ciphers.append(cipher * mul_value)
        # decrypt
        outputs = homo_impl.decrypt_batch_parallel(mul_ciphers)
        mul_result = []
        for input in inputs:
            mul_result.append(mul_value * input)
        self.ut.assertListEqual(mul_result, list(outputs))
        end_time = time.time()
        print(
            f"#### test_homo_mul_enc_and_dec passed, time: {end_time - start_time} seconds")

    def test_homo_add_enc_and_desc(self, homo_impl, ciphers1, ciphers2, inputs1, inputs2):
        start_time = time.time()
        add_ciphers = []
        i = 0
        expected_result = []
        for cipher in ciphers1:
            add_ciphers.append(cipher + ciphers2[i])
            expected_result.append(inputs1[i] + inputs2[i])
            i += 1
        outputs = homo_impl.decrypt_batch_parallel(add_ciphers)
        self.ut.assertListEqual(expected_result, list(outputs))
        end_time = time.time()
        print(
            f"#### test_homo_add_enc_and_desc passed, time: {end_time - start_time} seconds, size: {len(inputs1)}")


class PaillierUtilsTest(unittest.TestCase):
    def test_paillier_enc_and_dec(self):
        print("######## test paillier case: ####")
        homo_test = HomoTest(self)
        homo_test.test_enc_and_dec_parallel(homo_test.paillier, 10, -20, -1)
        homo_test.test_enc_and_dec_parallel(
            homo_test.paillier, 10000, -20000, 20000)
        homo_test.test_enc_and_dec_parallel(
            homo_test.paillier, 10000, 0, 20000)
        print("######## test paillier case end ####")

    def test_ihc_enc_and_dec(self):
        print("######## test ihc case: ####")
        homo_test = HomoTest(self)
        homo_test.test_enc_and_dec_parallel(homo_test.ihc_cipher, 10, -20, -1)
        homo_test.test_enc_and_dec_parallel(
            homo_test.ihc_cipher, 10000, -20000, 20000)
        homo_test.test_enc_and_dec_parallel(
            homo_test.ihc_cipher, 10000, 0, 20000)
        print("######## test ihc case end ####")

    def test_ihc_enc_and_dec_parallel(self):
        print("##### test_ihc_enc_and_dec_parallel")
        ihc = IhcCipher(key_length=256)
        try_size = 100000
        inputs = np.random.randint(-10001, 10001, size=try_size)
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
            IhcCiphertext(cipher_left, cipher_right, ihc.number_codec)
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

    def test_ihc_decode(self):
        print("##### test_ihc_decode")
        ihc = IhcCipher(key_length=256)
        try_size = 100000
        inputs = np.random.randint(-10001, 10001, size=try_size)
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
