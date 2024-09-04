package com.webank.wedpr.sdk.jni.homo;

import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.JniLibLoader;

public class NativeFloatingIhc {
    public enum mode {
        IHC_128,
        IHC_256
    }

    static {
        JniLibLoader.loadJniLibrary();
    }

    public static native byte[] generateKey(int mode) throws JniException;

    public static native byte[] encrypt(int mode, byte[] key, byte[] significant, int exponent)
            throws JniException;

    public static native FloatingPointNumber decrypt(int mode, byte[] key, byte[] cipher)
            throws JniException;

    public static native byte[] add(int mode, byte[] cipher1, byte[] cipher2) throws JniException;

    public static native byte[] sub(int mode, byte[] cipher1, byte[] cipher2) throws JniException;

    public static native byte[] scalaMul(int mode, byte[] significant, int exponent, byte[] cipher)
            throws JniException;
}
