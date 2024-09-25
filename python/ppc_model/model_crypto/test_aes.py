from ppc_model.model_crypto.crypto_aes import generate_aes_key, save_key_to_file
from ppc_model.model_crypto.crypto_aes import encrypt_data, decrypt_data
from ppc_model.model_crypto.crypto_aes import base64_to_cipher, cipher_to_base64


key = generate_aes_key()
save_key_to_file(key, 'aes_key.bin')
print("AES密钥已生成并保存到aes_key.bin文件中。")

plaintext = "需要加密的内容".encode('utf-8')
ciphertext = encrypt_data(key, plaintext)
print(f"加密后的内容: {ciphertext}")

decrypted_text = decrypt_data(key, ciphertext)
print(f"解密后的内容: {decrypted_text.decode('utf-8')}")

# 保存密文到文件
# ciphertext = encrypt_data(key, plaintext)
encoded_ciphertext = cipher_to_base64(ciphertext)
print(f"encoded_ciphertext: {encoded_ciphertext}")

# 使用AES密钥解密
decoded_ciphertext = base64_to_cipher(encoded_ciphertext)
print(f"encoded_ciphertext: {decoded_ciphertext}")
decrypted_text = decrypt_data(key, decoded_ciphertext)
print(f"解密后的内容字符串: {decrypted_text.decode('utf-8')}")
