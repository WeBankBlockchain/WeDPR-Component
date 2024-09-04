package com.webank.wedpr.hive.udf.demo;

import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingPointPaillier;
import com.webank.wedpr.sdk.jni.homo.NativePaillier;
import java.io.File;
import java.io.FileWriter;
import java.math.BigDecimal;
import java.util.Random;
import javax.xml.bind.DatatypeConverter;

public class GeneratePaillierMain {

    public static void main(String[] args) throws Exception {
        String hexPublicKey =
                "00000800010100E74CF4A6DD95032412678A2702C6B1C158F0DC94F72191311DC8FA3A2608AAC2F287D3BE7D2583EE7F042B1DBC42C8FE149669D4A36A77A49956D9DFC83DCBD38AB03F7324CB5071EEED8C7FAE108BA4C79639CAFEB4FF96A87EFAAB4580F8CDFCD9FC55BAD63AF2B27BF84B568348228ED7CE8A16A1220CB323301D7F4475A7BAB747477D9056CF9D1FDDE6EBDF4D94814B4A530FB84CC34E36AD0E8DCA220F7E354B4C471B699B155A3463C8A74DE5E824897F519A4BBF01E11FD3F62A349B0604003FB6BC00C1C25173AF4FA610B760114CC55BB92A4173B90A52DE2009C5B18F86725A27B6DA19ED60BD208C5F1875E151CD8DA8D689543C50347A23ED5D0201FF0D6877B3378C46D2839EEF71C78042F99218C2C3E6A3EEB741BAD372E2B5B7FFA222BEBA5C3C18A1B3E4AC890F5AFBAE460607A558743738D11E53EDFA00A54D209A0F52410576ACDD2F28D448B3E48F054A066BEEFA61207E29BB0BDC1EA1EA1B674A94D63D72CC57C55F8250E3EB01F1A4AD42FA14F9EAF92D880CF279DFF50A2AD6FD062B026886FF23BBE310EA1E324BF83B6860A864BD9E28B5DBE5D01173E090827262A65FA7EB0729860808DE9DEB9B2D17B0EABCB3699635EB5B4676AA7E7C533F361FDBE578DA114C6771EEE97894291796A03F1519C77ADBF8697741E48E1DC8E0B15667B617631155E88D9D6781B0FC2D26BD9A121620B2A65ACD099FE494BD887021759E7A16AAA570C881964DF67152E7D578A1751E09B11461A1ACF5FDAEFEAC5410BC5970BE924E56551F7763FD293669F24FAE322931DACDB27A6A865E0F5875A5C26082A329CEDEA47A5F2776DFD2144389AB5EC5358CFAB44B481851ED03292AB79B3E59EF0B0CB8DCE7CEE102E5E0A7597150B297FFEFFF80ED684C1CC5A0DE369EC2B636F1F9B41F4372B4AF662C71F89F0AD456595423CE4B3A38E5998CBB5EA096C9AFAABD9B48DE8BC5819F95C47E7576CAC92CADC4E00175252D6BFD1C5D585442AD5C4285A6878DE9A65D425AF3B61359D9B39C1427923CEB5B82B1CFBF4941DA0454746CD79E30912998A6407F34C29713C210";

        long startTime1 = System.currentTimeMillis();
        long publicKeyPointer =
                NativePaillier.loadPublicKey(DatatypeConverter.parseHexBinary(hexPublicKey));
        long endTime = System.currentTimeMillis();
        System.out.println(
                "TestPaillierDemo loadPublicKey cost: " + (endTime - startTime1) + " ms");

        final NumberCodec codec = new NumberCodecImpl();
        // 创建一个文件
        File file = new File("paillier.csv");
        // 创建一个文件写入器
        FileWriter writer = new FileWriter(file);
        // 创建一个随机数生成器
        Random random = new Random();
        int count = Integer.parseInt(args[0]);
        // 循环生成100万个随机数
        for (int i = 1; i <= count; i++) {
            // 生成一个随机数
            double number = random.nextDouble() * 1000.99999 + 1.99999;
            //            System.out.println("TestPaillierDemo double number: " + number);
            BigDecimal originalValue = new BigDecimal(number);
            FloatingPointNumber plainValueFp = codec.encode(originalValue);
            byte[] cipher =
                    NativeFloatingPointPaillier.encrypt(
                            plainValueFp.getSignificantBytes(),
                            plainValueFp.getExponent(),
                            publicKeyPointer);
            String hexCipher = DatatypeConverter.printHexBinary(cipher);
            //            System.out.println("TestPaillierDemo BigDecimal encryptedValue: " +
            // hexCipher);
            // 将自增id和密文字符串写入文件
            writer.write(i + "," + hexCipher + "\n");
        }
        // 关闭文件写入器
        writer.close();
        long startTime2 = System.nanoTime();
        NativePaillier.freePublicKey(publicKeyPointer);
        long endTime2 = System.nanoTime();
        System.out.println(
                "TestPaillierDemo freePublicKey cost: " + (endTime2 - startTime2) + " ns");
    }
}
