package com.wedpr.pir.sdk.helper;

import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * @author zachma
 * @date 2024/8/19
 */
public class CryptoOperatorHelper {
    private static final BigInteger DEFAULT_G =
            new BigInteger(
                    "9020881489161854992071763483314773468341853433975756385639545080944698236944020124874820917267762049756743282301106459062535797137327360192691469027152272");
    private static final BigInteger DEFAULT_N =
            new BigInteger(
                    "102724610959913950919762303151320427896415051258714708724768326174083057407299433043362228762657118029566890747043004760241559786931866234640457856691885212534669604964926915306738569799518792945024759514373214412797317972739022405456550476153212687312211184540248262330559143446510677062823907392904449451177");
    private static final BigInteger DEFAULT_FI =
            new BigInteger(
                    "102724610959913950919762303151320427896415051258714708724768326174083057407299433043362228762657118029566890747043004760241559786931866234640457856691885192126363163670343672910761259882348623401714459980712242233796355982147797162316532450768783823909695360736554767341443201861573989081253763975895939627220");

    private static final SecureRandom RANDOM = new SecureRandom();

    /** 生成随机数 * */
    public static BigInteger getRandomInt() {
        BigInteger num = new BigInteger(DEFAULT_N.bitLength(), RANDOM);
        while (num.compareTo(DEFAULT_N) >= 0) {
            num = new BigInteger(DEFAULT_N.bitLength(), RANDOM);
        }
        return num;
    }

    /** b*G mod N */
    public static BigInteger powMod(BigInteger b) {
        return DEFAULT_G.modPow(b, DEFAULT_N);
    }

    /** a^b mod N */
    public static BigInteger OTPow(BigInteger a, BigInteger b) {
        return a.modPow(b, DEFAULT_N);
    }

    /** a*b mod FI */
    public static BigInteger mulMod(BigInteger a, BigInteger b) {
        return a.multiply(b).mod(DEFAULT_FI);
    }

    /** a*b mod N */
    public static BigInteger OTMul(BigInteger a, BigInteger b) {
        return a.multiply(b).mod(DEFAULT_N);
    }

    /** 批量获取searchID的Hash值 */
    public static List<String> getIdHashVec(int obfuscationOrder, Integer idIndex, String searchId)
            throws WedprException {

        List<String> clientDataBodyList = new ArrayList<>();
        String searchIdTemp;
        for (int i = 0; i < obfuscationOrder + 1; i++) {
            if (idIndex == i) {
                searchIdTemp = searchId;
            } else {
                String uuid = UUID.randomUUID().toString();
                searchIdTemp = makeHash(uuid.getBytes());
            }
            clientDataBodyList.add(searchIdTemp);
        }
        return clientDataBodyList;
    }

    private static String makeHash(byte[] data) throws WedprException {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hashBytes = digest.digest(data);

            StringBuilder hexString = new StringBuilder();
            for (byte b : hashBytes) {
                String hex = Integer.toHexString(0xFF & b);
                if (hex.length() == 1) {
                    hexString.append('0');
                }
                hexString.append(hex);
            }
            return hexString.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new WedprException(WedprStatusEnum.WRONG_HASH_ERROR);
        }
    }
}
