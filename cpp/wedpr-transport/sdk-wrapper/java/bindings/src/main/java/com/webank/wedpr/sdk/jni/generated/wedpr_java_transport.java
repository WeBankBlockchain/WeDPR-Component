/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.webank.wedpr.sdk.jni.generated;

public class wedpr_java_transport {
    public static String printFrontDesc(FrontConfig config) {
        return wedpr_java_transportJNI.printFrontDesc(FrontConfig.getCPtr(config), config);
    }

    public static String printOptionalField(MessageOptionalHeader optionalHeader) {
        return wedpr_java_transportJNI.printOptionalField(
                MessageOptionalHeader.getCPtr(optionalHeader), optionalHeader);
    }

    public static String printMessage(Message _msg) {
        return wedpr_java_transportJNI.printMessage(Message.getCPtr(_msg), _msg);
    }

    public static String printWsMessage(SWIGTYPE_p_bcos__boostssl__MessageFace__Ptr _msg) {
        return wedpr_java_transportJNI.printWsMessage(
                SWIGTYPE_p_bcos__boostssl__MessageFace__Ptr.getCPtr(_msg));
    }
}