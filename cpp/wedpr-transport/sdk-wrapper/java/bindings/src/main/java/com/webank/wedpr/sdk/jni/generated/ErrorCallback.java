/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class ErrorCallback {
    private transient long swigCPtr;
    private transient boolean swigCMemOwn;

    protected ErrorCallback(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(ErrorCallback obj) {
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
                wedpr_java_transportJNI.delete_ErrorCallback(swigCPtr);
            }
            swigCPtr = 0;
        }
    }

    protected void swigDirectorDisconnect() {
        swigSetCMemOwn(false);
        delete();
    }

    public void swigReleaseOwnership() {
        swigSetCMemOwn(false);
        wedpr_java_transportJNI.ErrorCallback_change_ownership(this, swigCPtr, false);
    }

    public void swigTakeOwnership() {
        swigSetCMemOwn(true);
        wedpr_java_transportJNI.ErrorCallback_change_ownership(this, swigCPtr, true);
    }

    public ErrorCallback() {
        this(wedpr_java_transportJNI.new_ErrorCallback(), true);
        wedpr_java_transportJNI.ErrorCallback_director_connect(this, swigCPtr, true, true);
    }

    public void onError(Error error) {
        wedpr_java_transportJNI.ErrorCallback_onError(swigCPtr, this, Error.getCPtr(error), error);
    }
}
