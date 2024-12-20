/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class ubytes extends java.util.AbstractList<Short> implements java.util.RandomAccess {
    private transient long swigCPtr;
    protected transient boolean swigCMemOwn;

    protected ubytes(long cPtr, boolean cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = cPtr;
    }

    protected static long getCPtr(ubytes obj) {
        return (obj == null) ? 0 : obj.swigCPtr;
    }

    protected static long swigRelease(ubytes obj) {
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
                wedpr_java_transportJNI.delete_ubytes(swigCPtr);
            }
            swigCPtr = 0;
        }
    }

    public ubytes(short[] initialElements) {
        this();
        reserve(initialElements.length);

        for (short element : initialElements) {
            add(element);
        }
    }

    public ubytes(Iterable<Short> initialElements) {
        this();
        for (short element : initialElements) {
            add(element);
        }
    }

    public Short get(int index) {
        return doGet(index);
    }

    public Short set(int index, Short e) {
        return doSet(index, e);
    }

    public boolean add(Short e) {
        modCount++;
        doAdd(e);
        return true;
    }

    public void add(int index, Short e) {
        modCount++;
        doAdd(index, e);
    }

    public Short remove(int index) {
        modCount++;
        return doRemove(index);
    }

    protected void removeRange(int fromIndex, int toIndex) {
        modCount++;
        doRemoveRange(fromIndex, toIndex);
    }

    public int size() {
        return doSize();
    }

    public int capacity() {
        return doCapacity();
    }

    public void reserve(int n) {
        doReserve(n);
    }

    public ubytes() {
        this(wedpr_java_transportJNI.new_ubytes__SWIG_0(), true);
    }

    public ubytes(ubytes other) {
        this(wedpr_java_transportJNI.new_ubytes__SWIG_1(ubytes.getCPtr(other), other), true);
    }

    public boolean isEmpty() {
        return wedpr_java_transportJNI.ubytes_isEmpty(swigCPtr, this);
    }

    public void clear() {
        wedpr_java_transportJNI.ubytes_clear(swigCPtr, this);
    }

    public ubytes(int count, short value) {
        this(wedpr_java_transportJNI.new_ubytes__SWIG_2(count, value), true);
    }

    private int doCapacity() {
        return wedpr_java_transportJNI.ubytes_doCapacity(swigCPtr, this);
    }

    private void doReserve(int n) {
        wedpr_java_transportJNI.ubytes_doReserve(swigCPtr, this, n);
    }

    private int doSize() {
        return wedpr_java_transportJNI.ubytes_doSize(swigCPtr, this);
    }

    private void doAdd(short x) {
        wedpr_java_transportJNI.ubytes_doAdd__SWIG_0(swigCPtr, this, x);
    }

    private void doAdd(int index, short x) {
        wedpr_java_transportJNI.ubytes_doAdd__SWIG_1(swigCPtr, this, index, x);
    }

    private short doRemove(int index) {
        return wedpr_java_transportJNI.ubytes_doRemove(swigCPtr, this, index);
    }

    private short doGet(int index) {
        return wedpr_java_transportJNI.ubytes_doGet(swigCPtr, this, index);
    }

    private short doSet(int index, short val) {
        return wedpr_java_transportJNI.ubytes_doSet(swigCPtr, this, index, val);
    }

    private void doRemoveRange(int fromIndex, int toIndex) {
        wedpr_java_transportJNI.ubytes_doRemoveRange(swigCPtr, this, fromIndex, toIndex);
    }
}
