/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class INodeInfo {
    private transient long swigCPtr;
    private transient boolean swigCMemOwn;

    protected INodeInfo(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(INodeInfo obj) {
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
                wedpr_java_transportJNI.delete_INodeInfo(swigCPtr);
            }
            swigCPtr = 0;
        }
    }

    public String endPoint() {
        return wedpr_java_transportJNI.INodeInfo_endPoint(swigCPtr, this);
    }

    public SWIGTYPE_p_bcos__bytesConstRef nodeID() {
        return new SWIGTYPE_p_bcos__bytesConstRef(
                wedpr_java_transportJNI.INodeInfo_nodeID(swigCPtr, this), true);
    }

    public void setNodeID(SWIGTYPE_p_bcos__bytesConstRef nodeID) {
        wedpr_java_transportJNI.INodeInfo_setNodeID(
                swigCPtr, this, SWIGTYPE_p_bcos__bytesConstRef.getCPtr(nodeID));
    }

    public void setEndPoint(String endPoint) {
        wedpr_java_transportJNI.INodeInfo_setEndPoint(swigCPtr, this, endPoint);
    }

    public boolean addComponent(String component) {
        return wedpr_java_transportJNI.INodeInfo_addComponent(swigCPtr, this, component);
    }

    public boolean eraseComponent(String component) {
        return wedpr_java_transportJNI.INodeInfo_eraseComponent(swigCPtr, this, component);
    }

    public boolean componentExist(String component) {
        return wedpr_java_transportJNI.INodeInfo_componentExist(swigCPtr, this, component);
    }

    public StringVec copiedComponents() {
        return new StringVec(
                wedpr_java_transportJNI.INodeInfo_copiedComponents(swigCPtr, this), true);
    }

    public String meta() {
        return wedpr_java_transportJNI.INodeInfo_meta(swigCPtr, this);
    }

    public void setMeta(String meta) {
        wedpr_java_transportJNI.INodeInfo_setMeta(swigCPtr, this, meta);
    }
}
