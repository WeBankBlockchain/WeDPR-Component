package com.wedpr.pir.sdk.exception;

import lombok.AllArgsConstructor;
import lombok.Getter;

/**
 * @author zachma
 * @date 2024/8/18
 */
@AllArgsConstructor
@Getter
public enum WedprStatusEnum {
    SUCCESS("0", "成功"),
    FAILURE("1", "系统内部错误"),
    INTERNAL_SERVER_ERROR("3", "系统内部错误"),
    SYSTEM_CONFIG_ERROR("4", "系统配置参数错误"),
    HANDLE_FILE_ERROR("5", "处理文件错误"),
    TASK_EXECUTE_ERROR("6", "任务执行失败"),

    // 1000 未定义错误
    SYSTEM_EXCEPTION("1000", "系统抛出异常，请检查日志记录"),
    CLIENT_PARAM_EXCEPTION("1001", "发起方参数错误"),

    // 2000 参数解析错误
    INVALID_INPUT_VALUE("2000", "输入参数异常"),
    JSON_ERROR("2001", "输入参数JSON解析失败"),
    INVALID_SYSTEM_KEY("2002", "系统密钥解析失败"),
    INVALID_CUSTOMER_TYPE("2003", "未定义的客户类型"),
    INVALID_HASH_VALUE("2004", "输入的哈希值不合法"),
    INVALID_INPUT_LENGTH("2005", "输入参数列表长度不合法"),
    INVALID_INPUT_PREFIX("2006", "输入参数前缀不匹配"),
    INVALID_TIMESTAMP_VALUE("2007", "输入时间戳不合法"),
    JSON_GET_ERROR("2008", "JSON解析返回值失败"),
    INVALID_TRANS_TYPE("2009", "不合法的交易类型"),
    INVALID_FILE_PATH("2010", "不合法交集文件路径"),
    INVALID_ROLE_VALUE("2011", "pir.Role设置错误"),
    CSV_ERROR("2012", "csv解析是错误"),

    // 3000 db错误
    DB_ERROR("3000", "DB查询失败"),
    // DB_TIMEOUT("3000", "DB访问超时"),
    DB_INSERT_FAILURE("3001", "DB插入失败"),
    DB_DELETE_FAILURE("3002", "DB删除失败"),
    UNENCRYPTED_DATA("3003", "查询的数据未被加密"),
    DB_MISSED_USER("3004", "无法写入未命中用户"),

    // 4000 被请求服务端异常
    SERVICE_UNAVAILABLE("4000", "对方PIR服务访问失败"),
    INVALID_ENCRYPTION_KEY("4001", "服务端加密密钥异常"),
    SEARCHED_DB_UNAVAILABLE("4002", "服务端db无法访问"),
    HTTP_CALL_ERROR("4003", "HTTP调用异常"),
    CALL_PSI_ERROR("4004", "调用PIR服务异常"),
    UPLOAD_FPS_ERROR("4005", "上传文件至Fps失败"),
    DOWNLOAD_FPS_ERROR("4006", "下载Fps文件失败"),
    CLIENT_LOCAL_FILE("4007", "本地初始化./client/local.txt没有设置"),
    SERVER_LOCAL_FILE("4008", "本地初始化./server/local.txt没有设置"),

    // 5000 密码学运算错误
    AES_ENCRYPT_ERROR("5000", "AES加密明文信息错误"),
    AES_DECRYPT_ERROR("5001", "AES解密密文信息错误"),
    MATCH_RESULT_ERROR("5002", "找不到匹配结果查询值"),
    ENCODE_BIG_NUMBER_ERROR("5003", "转换输入哈希到BIG NUMBER类型失败"),
    DECODE_ECC_POINT_ERROR("5004", "解码密文到椭圆曲线上点失败"),
    MATCH_CIPHER_ERROR("5005", "计算匹配结果失败"),
    CLIENT_CIPHER_ERROR("5006", "计算客户端密文失败"),
    CIPHER_LENGTH_ERROR("5007", "根据前缀索引到的密文太多"),
    WRONG_HASH_ERROR("5008", "输入哈希算法错误"),

    CRYPTO_SPE_ERROR("5099", "密码运算通用错误");

    private String code;
    private String message;
}
