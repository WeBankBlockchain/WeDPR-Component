/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class GetPeersInfoHandler {
    private transient long swigCPtr;
    private transient boolean swigCMemOwn;

    protected GetPeersInfoHandler(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(GetPeersInfoHandler obj) {
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
                wedpr_java_transportJNI.delete_GetPeersInfoHandler(swigCPtr);
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
        wedpr_java_transportJNI.GetPeersInfoHandler_change_ownership(this, swigCPtr, false);
    }

    public void swigTakeOwnership() {
        swigSetCMemOwn(true);
        wedpr_java_transportJNI.GetPeersInfoHandler_change_ownership(this, swigCPtr, true);
    }

    public GetPeersInfoHandler() {
        this(wedpr_java_transportJNI.new_GetPeersInfoHandler(), true);
        wedpr_java_transportJNI.GetPeersInfoHandler_director_connect(this, swigCPtr, true, true);
    }

    public void onPeersInfo(Error e, String peersInfo) {
        wedpr_java_transportJNI.GetPeersInfoHandler_onPeersInfo(
                swigCPtr, this, Error.getCPtr(e), e, peersInfo);
    }
}
