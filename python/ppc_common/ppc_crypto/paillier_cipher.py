import math
import os
from concurrent.futures import ProcessPoolExecutor

from phe import paillier, EncryptedNumber

from ppc_common.ppc_crypto.phe_cipher import PheCipher


class PaillierCipher(PheCipher):

    def __init__(self, key_length: int = 2048) -> None:
        super().__init__(key_length)
        self.public_key, self.private_key = paillier.generate_paillier_keypair(
            n_length=self.key_length)

    def encrypt(self, number) -> EncryptedNumber:
        return self.public_key.encrypt(int(number))

    def decrypt(self, cipher: EncryptedNumber) -> int:
        return self.private_key.decrypt(cipher)

    def encrypt_batch(self, numbers) -> list:
        return [self.encrypt(num) for num in numbers]

    def decrypt_batch(self, ciphers) -> list:
        return [self.decrypt(cipher) for cipher in ciphers]

    def encrypt_batch_parallel(self, numbers) -> list:
        num_cores = os.cpu_count()
        batch_size = math.ceil(len(numbers) / num_cores)
        batches = [numbers[i:i + batch_size]
                   for i in range(0, len(numbers), batch_size)]
        with ProcessPoolExecutor(max_workers=num_cores) as executor:
            futures = [executor.submit(self.encrypt_batch, batch)
                       for batch in batches]
            result = [future.result() for future in futures]
        return [item for sublist in result for item in sublist]

    def decrypt_batch_parallel(self, ciphers) -> list:
        num_cores = os.cpu_count()
        batch_size = math.ceil(len(ciphers) / num_cores)
        batches = [ciphers[i:i + batch_size]
                   for i in range(0, len(ciphers), batch_size)]
        with ProcessPoolExecutor(max_workers=num_cores) as executor:
            futures = [executor.submit(self.decrypt_batch, batch)
                       for batch in batches]
            result = [future.result() for future in futures]
        return [item for sublist in result for item in sublist]
