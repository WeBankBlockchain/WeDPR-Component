# from abc import ABC
# import namedTuple
from dataclasses import dataclass

import struct
from ppc_common.ppc_crypto.phe_cipher import PheCipher
import secrets


@dataclass
class IhcCiphertext():
    __slots__ = ['c_left', 'c_right']
    
    def __init__(self, c_left: int, c_right: int) -> None:
        self.c_left = c_left
        self.c_right = c_right
        
    def __add__(self, other):
        cipher_left = self.c_left + other.c_left
        cipher_right = self.c_right + other.c_right
        return IhcCiphertext(cipher_left, cipher_right)
    
    def __eq__(self, other):
        return self.c_left == other.c_left and self.c_right == other.c_right
    
    def encode(self) -> bytes:
        # 计算每个整数的字节长度
        len_c_left = (self.c_left.bit_length() + 7) // 8
        len_c_right = (self.c_right.bit_length() + 7) // 8

        # 将整数转换为字节序列，使用大端字节序和带符号整数
        c_left_bytes = self.c_left.to_bytes(len_c_left, byteorder='big')
        c_right_bytes = self.c_right.to_bytes(len_c_right, byteorder='big')

        # 编码整数的长度
        len_bytes = struct.pack('>II', len_c_left, len_c_right)

        # 返回所有数据
        return len_bytes + c_left_bytes + c_right_bytes
    
    @classmethod
    def decode(cls, encoded_data: bytes):
        # 解码整数的长度
        len_c_left, len_c_right = struct.unpack('>II', encoded_data[:8])

        # 根据长度解码整数
        c_left = int.from_bytes(encoded_data[8:8 + len_c_left], byteorder='big')
        c_right = int.from_bytes(encoded_data[8 + len_c_left:8 + len_c_left + len_c_right], byteorder='big')
        return cls(c_left, c_right)
    
class IhcCipher(PheCipher):
    def __init__(self, key_length: int = 256, iter_round: int = 16) -> None:
        super().__init__(key_length)
        key = secrets.randbits(key_length)
        self.public_key = key
        self.private_key = key
        self.iter_round = iter_round
        self.key_length = key_length
        
        self.max_mod = 1 << key_length
        
    def encrypt(self, number: int) -> IhcCiphertext:
        random_u = secrets.randbits(self.key_length)
        x_this = number
        x_last = random_u
        for i in range(0, self.iter_round):
            x_tmp = (self.private_key * x_this - x_last) % self.max_mod
            x_last = x_this
            x_this = x_tmp
        # cipher = IhcCiphertext(x_this, x_last, self.max_mod)
        cipher = IhcCiphertext(x_this, x_last)
        return cipher
    
    def decrypt(self, cipher: IhcCiphertext) -> int:
        x_this = cipher.c_right
        x_last = cipher.c_left
        for i in range(0, self.iter_round-1):
            x_tmp = (self.private_key * x_this - x_last) % self.max_mod
            x_last = x_this
            x_this = x_tmp
        return x_this
    
    def encrypt_batch(self, numbers) -> list:
        return [self.encrypt(num) for num in numbers]

    def decrypt_batch(self, ciphers) -> list:
        return [self.decrypt(cipher) for cipher in ciphers]

    def encrypt_batch_parallel(self, numbers: list) -> list:
        return self.encrypt_batch(numbers)

    def decrypt_batch_parallel(self, ciphers: list) -> list:
        return self.decrypt_batch(ciphers)
