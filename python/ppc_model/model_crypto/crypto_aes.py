import os
import base64

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding


# 生成256位（32字节）的AES密钥
def generate_aes_key():
    return os.urandom(32)  # 32 bytes == 256 bits


# 将密钥保存到文件
def save_key_to_file(key, filename):
    with open(filename, 'wb') as file:
        file.write(key)


# 从文件中加载密钥
def load_key_from_file(filename):
    with open(filename, 'rb') as file:
        key = file.read()
    return key

# AES加密函数


def encrypt_data(key, plaintext):
    # 使用随机生成的初始向量 (IV)
    iv = os.urandom(16)  # AES块大小为128位（16字节）

    # 创建AES加密器
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv),
                    backend=default_backend())
    encryptor = cipher.encryptor()

    # 对数据进行填充（AES要求输入的块大小为128位）
    padder = padding.PKCS7(128).padder()
    padded_data = padder.update(plaintext) + padder.finalize()

    # 加密数据
    ciphertext = encryptor.update(padded_data) + encryptor.finalize()

    # 返回IV和密文
    return iv + ciphertext


# AES解密函数
def decrypt_data(key, ciphertext):
    # 提取IV和密文
    iv = ciphertext[:16]  # 前16字节是IV
    actual_ciphertext = ciphertext[16:]

    # 创建AES解密器
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv),
                    backend=default_backend())
    decryptor = cipher.decryptor()

    # 解密数据
    decrypted_padded_data = decryptor.update(
        actual_ciphertext) + decryptor.finalize()

    # 去除填充
    unpadder = padding.PKCS7(128).unpadder()
    plaintext = unpadder.update(decrypted_padded_data) + unpadder.finalize()

    return plaintext


def cipher_to_base64(ciphertext):
    # 将bytes类型转换为Base64字符串
    encoded_ciphertext = base64.b64encode(ciphertext).decode('utf-8')
    return encoded_ciphertext


def base64_to_cipher(data):
    decoded_ciphertext = base64.b64decode(data)
    return decoded_ciphertext
