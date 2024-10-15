/*
 * Copyright 2017-2025  [webank-wedpr]
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */

package com.webank.wedpr.sdk.jni.transport.impl;

import com.webank.wedpr.sdk.jni.generated.Message;
import com.webank.wedpr.sdk.jni.generated.MessageHeader;
import com.webank.wedpr.sdk.jni.transport.IMessage;

public class MessageImpl implements IMessage {
    public class MessageHeaderImpl implements IMessageHeader {
        private final MessageHeader messageHeader;

        protected MessageHeaderImpl(MessageHeader messageHeader) {
            this.messageHeader = messageHeader;
            this.messageHeader.optionalField().disOwnMemory();
            this.messageHeader.disOwnMemory();
        }

        @Override
        public int getVersion() {
            return this.messageHeader.version();
        }

        @Override
        public String getTraceID() {
            return this.messageHeader.traceID();
        }

        @Override
        public String getSrcGwNode() {
            return this.messageHeader.srcGwNode();
        }

        @Override
        public String getDstGwNode() {
            return this.messageHeader.dstGwNode();
        }

        @Override
        public int getPacketType() {
            return this.messageHeader.packetType();
        }

        @Override
        public int getTTL() {
            return this.messageHeader.ttl();
        }

        @Override
        public int getExt() {
            return this.messageHeader.ext();
        }

        @Override
        public boolean isRespPacket() {
            return this.messageHeader.isRespPacket();
        }

        @Override
        public int getRouteType() {
            return this.messageHeader.routeType();
        }

        @Override
        public String getComponentType() {
            return this.messageHeader.optionalField().componentType();
        }

        @Override
        public byte[] getSrcNode() {
            return this.messageHeader.optionalField().srcNodeBuffer();
        }

        @Override
        public byte[] getDstNode() {
            return this.messageHeader.optionalField().dstNodeBuffer();
        }

        @Override
        public String getDstInst() {
            return this.messageHeader.optionalField().dstInst();
        }

        @Override
        public String getSrcInst() {
            return this.messageHeader.optionalField().srcInst();
        }

        @Override
        public String getTopic() {
            return this.messageHeader.optionalField().topic();
        }

        @Override
        public String toString() {
            return "MessageHeaderImpl{"
                    + "traceID="
                    + getTraceID()
                    + ", srcInst="
                    + getSrcInst()
                    + ", dstInst="
                    + getDstInst()
                    + ", topic="
                    + getTopic()
                    + ", response="
                    + isResponsePacket()
                    + '}';
        }
    }

    private final MessageHeaderImpl messageHeader;
    private final Message rawMessage;
    // v1, v2
    public MessageImpl(Message rawMessage) {
        this.rawMessage = rawMessage;
        this.rawMessage.disOwnMemory();
        this.messageHeader = new MessageHeaderImpl(rawMessage.header());
    }

    @Override
    public IMessageHeader getHeader() {
        return this.messageHeader;
    }

    @Override
    public Integer getSeq() {
        return this.rawMessage.frontMessage().seq();
    }

    @Override
    public byte[] getPayload() {
        return this.rawMessage.frontMessage().dataBuffer();
    }

    @Override
    public boolean isResponsePacket() {
        return this.rawMessage.frontMessage().isRespPacket();
    }

    @Override
    public String toString() {
        return "MessageImpl{" + "messageHeader=" + messageHeader + '}';
    }
}
