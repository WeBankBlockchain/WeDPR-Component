package com.webank.wedpr.hive.udf.exceptions;

public class KeyException extends RuntimeException {
    public KeyException(String message) {
        super(message);
    }

    public KeyException(String message, Throwable cause) {
        super(message, cause);
    }
}
