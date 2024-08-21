package com.webank.wedpr.hive.udf.impl.ihc;

import com.webank.wedpr.hive.udf.config.ConfigLoader;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
import java.util.ArrayList;
import javax.xml.bind.DatatypeConverter;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDFArgumentTypeException;
import org.apache.hadoop.hive.ql.metadata.HiveException;
import org.apache.hadoop.hive.ql.parse.SemanticException;
import org.apache.hadoop.hive.ql.udf.generic.AbstractGenericUDAFResolver;
import org.apache.hadoop.hive.ql.udf.generic.GenericUDAFEvaluator;
import org.apache.hadoop.hive.ql.udf.generic.GenericUDAFParameterInfo;
import org.apache.hadoop.hive.serde2.objectinspector.*;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.BinaryObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.PrimitiveObjectInspectorFactory;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.PrimitiveObjectInspectorUtils;
import org.apache.hadoop.hive.serde2.typeinfo.PrimitiveTypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.TypeInfo;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.Text;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author caryliao
 * @date 2023/11/9
 */
@Description(
        name = "ihc_sum",
        value = "Returns the sum of a set of ihc ciphertexts",
        extended =
                "Usage:ihc_sum(columnName) \n"
                        + "Example:select ihc_sum(encrypted_salary, 64) from t_user\n")
public class IhcSumUDAF extends AbstractGenericUDAFResolver {
    static final Logger logger = LoggerFactory.getLogger(IhcSumUDAF.class);

    @Override
    public GenericUDAFEvaluator getEvaluator(GenericUDAFParameterInfo info)
            throws SemanticException {
        return getEvaluator(info.getParameters());
    }

    @Override
    public GenericUDAFEvaluator getEvaluator(TypeInfo[] parameters) throws SemanticException {
        int paramsCount = 1;
        if (parameters.length != paramsCount) {
            throw new UDFArgumentTypeException(
                    parameters.length - 1, "Exactly one arguments are expected.");
        }

        if (((PrimitiveTypeInfo) parameters[0]).getPrimitiveCategory()
                != PrimitiveObjectInspector.PrimitiveCategory.STRING) {
            throw new UDFArgumentTypeException(0, "Only (string, int) type arguments are accepted");
        }
        return new IhcSumUDAFEvaluator();
    }

    /** IhcSumUDAFEvaluator. */
    public static class IhcSumUDAFEvaluator extends GenericUDAFEvaluator {

        // Iterate 输入
        private PrimitiveObjectInspector inputOICipherText;
        // Merge 输入
        private StructObjectInspector structOI;

        private BinaryObjectInspector sumFieldOI;

        private StructField sumField;

        // TerminatePartial 输出
        private Object[] partialResult;

        // Terminate 输出
        private Text result;

        // the keyConfig
        private KeyConfig keyConfig;

        // 中间结果
        static class IhcSumBuffer implements AggregationBuffer {
            byte[] sum;
        }

        @Override
        public ObjectInspector init(Mode mode, ObjectInspector[] parameters) throws HiveException {
            super.init(mode, parameters);
            // load keys
            keyConfig = ConfigLoader.config.getKeyConfig();
            // 初始化输入参数类型
            if (mode == Mode.PARTIAL1 || mode == Mode.COMPLETE) {
                inputOICipherText = (PrimitiveObjectInspector) parameters[0];
            } else {
                // Struct
                structOI = (StructObjectInspector) parameters[0];
                sumField = structOI.getStructFieldRef("sum");
                sumFieldOI = (BinaryObjectInspector) sumField.getFieldObjectInspector();
            }
            // 初始化输出类型
            if (mode == Mode.PARTIAL1 || mode == Mode.PARTIAL2) {
                // 输出类型
                if (partialResult == null) {
                    partialResult = new Object[1];
                    partialResult[0] = new BytesWritable();
                }
                // 输出OI
                // 字段类型
                ArrayList<ObjectInspector> objectInspectors = new ArrayList<ObjectInspector>();
                objectInspectors.add(PrimitiveObjectInspectorFactory.writableBinaryObjectInspector);
                // 字段名称
                ArrayList<String> fieldNames = new ArrayList<String>();
                fieldNames.add("sum");
                return ObjectInspectorFactory.getStandardStructObjectInspector(
                        fieldNames, objectInspectors);

            } else {
                result = new Text();
                return PrimitiveObjectInspectorFactory.writableStringObjectInspector;
            }
        }

        @Override
        public AggregationBuffer getNewAggregationBuffer() throws HiveException {
            IhcSumBuffer buffer = new IhcSumBuffer();
            reset(buffer);
            return buffer;
        }

        @Override
        public void reset(AggregationBuffer agg) throws HiveException {
            IhcSumBuffer buffer = (IhcSumBuffer) agg;
            buffer.sum = null;
        }

        @Override
        public void iterate(AggregationBuffer agg, Object[] parameters) throws HiveException {
            try {
                if (parameters[0] != null) {
                    IhcSumBuffer buffer = (IhcSumBuffer) agg;
                    String cipherText =
                            PrimitiveObjectInspectorUtils.getString(
                                    parameters[0], inputOICipherText);
                    byte[] cipherTextBytes = DatatypeConverter.parseHexBinary(cipherText);
                    if (buffer.sum == null) {
                        buffer.sum = cipherTextBytes;
                    } else {
                        buffer.sum =
                                NativeFloatingIhc.add(
                                        keyConfig.getIhcMode(), buffer.sum, cipherTextBytes);
                    }
                }
            } catch (JniException e) {
                logger.error("ihc迭代过程失败", e);
                throw new HiveException(e);
            } catch (Exception e) {
                logger.error("ihc迭代过程失败", e);
                throw new HiveException(e);
            }
        }

        // 输出部分聚合结果
        @Override
        public Object terminatePartial(AggregationBuffer agg) throws HiveException {
            IhcSumBuffer buffer = (IhcSumBuffer) agg;
            if (buffer.sum == null) {
                return null;
            }
            ((BytesWritable) partialResult[0]).set(new BytesWritable(buffer.sum));
            return partialResult;
        }

        // 合并
        @Override
        public void merge(AggregationBuffer agg, Object partialResult) throws HiveException {
            IhcSumBuffer buffer = (IhcSumBuffer) agg;
            if (partialResult == null) {
                return;
            }
            // 通过StandardStructObjectInspector实例，分解出 partialResult 数组元素值
            Object partialSum = structOI.getStructFieldData(partialResult, sumField);
            try {
                // 通过基本数据类型的OI 实例解析 Object 的值
                byte[] sum = sumFieldOI.getPrimitiveJavaObject(partialSum);
                if (buffer.sum == null) {
                    buffer.sum = sum;
                } else {
                    buffer.sum = NativeFloatingIhc.add(keyConfig.getIhcMode(), buffer.sum, sum);
                }
            } catch (JniException e) {
                logger.error("ihc聚合过程失败", e);
                throw new HiveException(e);
            }
        }

        // 输出最终聚合结果
        @Override
        public Object terminate(AggregationBuffer agg) throws HiveException {
            IhcSumBuffer buffer = (IhcSumBuffer) agg;
            if (buffer.sum == null) {
                return null;
            }
            if (result == null) {
                result = new Text();
            }
            result.set(DatatypeConverter.printHexBinary(buffer.sum));
            return result;
        }
    }
}
