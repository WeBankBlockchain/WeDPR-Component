from abc import ABC


class PheCipher(ABC):
    def __init__(self, key_length: int) -> None:
        self.key_length = key_length
        pass

    def encrypt(self, number: int):
        pass

    def decrypt(self, cipher) -> int:
        pass

    def encrypt_batch(self, numbers: list) -> list:
        pass

    def decrypt_batch(self, ciphers: list) -> list:
        pass

    def encrypt_batch_parallel(self, numbers: list) -> list:
        pass

    def decrypt_batch_parallel(self, ciphers: list) -> list:
        pass
