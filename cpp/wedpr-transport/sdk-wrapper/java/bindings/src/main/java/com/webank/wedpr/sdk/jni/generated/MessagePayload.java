/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class MessagePayload {
    private transient long swigCPtr;
    private transient boolean swigCMemOwn;

    protected MessagePayload(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(MessagePayload obj) {
        return (obj == null) ? 0 : obj.swigCPtr;
    }

    protected void swigSetCMemOwn(boolean own) {
        swigCMemOwn = own;
    }

    @SuppressWarnings({"deprecation", "removal"})
    protected void finalize() {
        delete();
    }

    public synchronized void delete() {
        if (swigCPtr != 0) {
            if (swigCMemOwn) {
                swigCMemOwn = false;
                wedpr_java_transportJNI.delete_MessagePayload(swigCPtr);
            }
            swigCPtr = 0;
        }
    }

    public long encode(ubytes buffer) {
        return wedpr_java_transportJNI.MessagePayload_encode(
                swigCPtr, this, ubytes.getCPtr(buffer), buffer);
    }

    public long decode(SWIGTYPE_p_bcos__bytesConstRef data) {
        return wedpr_java_transportJNI.MessagePayload_decode(
                swigCPtr, this, SWIGTYPE_p_bcos__bytesConstRef.getCPtr(data));
    }

    public short version() {
        return wedpr_java_transportJNI.MessagePayload_version(swigCPtr, this);
    }

    public void setVersion(short version) {
        wedpr_java_transportJNI.MessagePayload_setVersion(swigCPtr, this, version);
    }

    public byte[] dataBuffer() {
        return wedpr_java_transportJNI.MessagePayload_dataBuffer(swigCPtr, this);
    }

    public int seq() {
        return wedpr_java_transportJNI.MessagePayload_seq(swigCPtr, this);
    }

    public void setSeq(int seq) {
        wedpr_java_transportJNI.MessagePayload_setSeq(swigCPtr, this, seq);
    }

    public long length() {
        return wedpr_java_transportJNI.MessagePayload_length(swigCPtr, this);
    }

    public String traceID() {
        return wedpr_java_transportJNI.MessagePayload_traceID(swigCPtr, this);
    }

    public void setTraceID(String traceID) {
        wedpr_java_transportJNI.MessagePayload_setTraceID(swigCPtr, this, traceID);
    }

    public int ext() {
        return wedpr_java_transportJNI.MessagePayload_ext(swigCPtr, this);
    }

    public void setExt(int ext) {
        wedpr_java_transportJNI.MessagePayload_setExt(swigCPtr, this, ext);
    }

    public void setRespPacket() {
        wedpr_java_transportJNI.MessagePayload_setRespPacket(swigCPtr, this);
    }

    public boolean isRespPacket() {
        return wedpr_java_transportJNI.MessagePayload_isRespPacket(swigCPtr, this);
    }

    public void disOwnMemory() {
        swigSetCMemOwn(false);
    }
}
