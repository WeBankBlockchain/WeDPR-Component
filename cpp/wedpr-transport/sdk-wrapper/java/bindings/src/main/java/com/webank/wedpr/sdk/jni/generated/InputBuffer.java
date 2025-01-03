/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class InputBuffer {
    private transient long swigCPtr;
    protected transient boolean swigCMemOwn;

    protected InputBuffer(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(InputBuffer obj) {
        return (obj == null) ? 0 : obj.swigCPtr;
    }

    protected static long swigRelease(InputBuffer obj) {
        long ptr = 0;
        if (obj != null) {
            if (!obj.swigCMemOwn)
                throw new RuntimeException("Cannot release ownership as memory is not owned");
            ptr = obj.swigCPtr;
            obj.swigCMemOwn = false;
            obj.delete();
        }
        return ptr;
    }

    @SuppressWarnings({"deprecation", "removal"})
    protected void finalize() {
        delete();
    }

    public synchronized void delete() {
        if (swigCPtr != 0) {
            if (swigCMemOwn) {
                swigCMemOwn = false;
                wedpr_java_transportJNI.delete_InputBuffer(swigCPtr);
            }
            swigCPtr = 0;
        }
    }

    public void setData(SWIGTYPE_p_unsigned_char value) {
        wedpr_java_transportJNI.InputBuffer_data_set(
                swigCPtr, this, SWIGTYPE_p_unsigned_char.getCPtr(value));
    }

    public SWIGTYPE_p_unsigned_char getData() {
        long cPtr = wedpr_java_transportJNI.InputBuffer_data_get(swigCPtr, this);
        return (cPtr == 0) ? null : new SWIGTYPE_p_unsigned_char(cPtr, false);
    }

    public void setLen(java.math.BigInteger value) {
        wedpr_java_transportJNI.InputBuffer_len_set(swigCPtr, this, value);
    }

    public java.math.BigInteger getLen() {
        return wedpr_java_transportJNI.InputBuffer_len_get(swigCPtr, this);
    }

    public InputBuffer() {
        this(wedpr_java_transportJNI.new_InputBuffer(), true);
    }
}
