package com.webank.wedpr.pir.crypto.service.impl;

import com.webank.wedpr.pir.crypto.entity.PirTable;
import com.webank.wedpr.pir.crypto.service.PirOTService;
import com.webank.wedpr.pir.crypto.service.PirTableService;
import com.wedpr.pir.sdk.entity.body.ServerResultBody;
import com.wedpr.pir.sdk.entity.body.ServerResultList;
import com.wedpr.pir.sdk.entity.request.ServerOTRequest;
import com.wedpr.pir.sdk.entity.response.ServerOTResponse;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.helper.AESHelper;
import com.wedpr.pir.sdk.helper.Constant;
import com.wedpr.pir.sdk.helper.ConvertTypeHelper;
import com.wedpr.pir.sdk.helper.CryptoOperatorHelper;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

/**
 * @author zachma
 * @date 2024/8/18
 */
@Service
public class PirOTServiceImpl implements PirOTService {
    private static final Logger logger = LoggerFactory.getLogger(PirOTServiceImpl.class);
    @Autowired PirTableService pirTableService;

    public ServerOTResponse runServerOTParam(ServerOTRequest serverOTRequest)
            throws WedprException {
        logger.info("Server start runServerOTParam.");
        String datasetId = serverOTRequest.getDatasetId();
        String tableId = Constant.datasetId2tableId(datasetId);
        BigInteger x = serverOTRequest.getX();
        BigInteger y = serverOTRequest.getY();
        String[] params = serverOTRequest.getParams();
        List<ServerResultList> serverResultlist = new ArrayList<>();
        for (int i = 0; i < serverOTRequest.getDataBodyList().size(); i++) {
            BigInteger z0 = serverOTRequest.getDataBodyList().get(i).getZ0();
            String filter = serverOTRequest.getDataBodyList().get(i).getFilter();
            List<Object[]> objects = pirTableService.idFilterTable(tableId, filter, params);
            List<PirTable> pirTables = pirTableService.objectsToPirTableList(objects);
            List<ServerResultBody> serverResultBodyTempList = new ArrayList<>();
            for (PirTable pirTable : pirTables) {
                ServerResultBody serverResultBody =
                        cipherPirTableContentIdSearch(pirTable, x, y, z0);
                serverResultBodyTempList.add(serverResultBody);
            }
            serverResultlist.add(new ServerResultList(serverResultBodyTempList));
        }
        logger.info("Server runServerOTParam success");
        return new ServerOTResponse(serverResultlist);
    }

    public ServerOTResponse runServerOTCipher(ServerOTRequest serverOTRequest)
            throws WedprException {
        logger.info("Server start runServerOTCipher.");
        String datasetId = serverOTRequest.getDatasetId();
        BigInteger x = serverOTRequest.getX();
        BigInteger y = serverOTRequest.getY();
        String tableId = Constant.datasetId2tableId(datasetId);
        List<ServerResultList> serverResultlist = new ArrayList<>();
        String[] params = serverOTRequest.getParams();
        for (int i = 0; i < serverOTRequest.getDataBodyList().size(); i++) {
            BigInteger z0 = serverOTRequest.getDataBodyList().get(i).getZ0();
            List<String> idHashList = serverOTRequest.getDataBodyList().get(i).getIdHashList();
            List<ServerResultBody> serverResultBodyTempList = new ArrayList<>();

            for (int j = 0; j < idHashList.size(); j++) {
                List<Object[]> objects =
                        pirTableService.idObfuscationTable(tableId, idHashList.get(j), params);
                List<PirTable> pirTables = pirTableService.objectsToPirTableList(objects);
                for (PirTable pirTable : pirTables) {
                    ServerResultBody serverResultBody =
                            cipherPirTableContentIdObfuscation(pirTable, x, y, z0, j);
                    serverResultBodyTempList.add(serverResultBody);
                }
            }
            serverResultlist.add(new ServerResultList(serverResultBodyTempList));
        }
        logger.info("Server runServerOTCipher success.");
        return new ServerOTResponse(serverResultlist);
    }

    private ServerResultBody cipherPirTableContentIdSearch(
            PirTable pirTable, BigInteger x, BigInteger y, BigInteger z0) throws WedprException {
        String uid = pirTable.getPirKey();
        String message = pirTable.getPirValue();
        byte[] uidBytes = uid.getBytes(StandardCharsets.UTF_8);
        BigInteger uidNum = ConvertTypeHelper.bytesToBigInteger(uidBytes);

        BigInteger blindingR = CryptoOperatorHelper.getRandomInt();
        BigInteger blindingS = CryptoOperatorHelper.getRandomInt();
        BigInteger w =
                CryptoOperatorHelper.OTMul(
                        CryptoOperatorHelper.OTPow(x, blindingS),
                        CryptoOperatorHelper.powMod(blindingR));
        BigInteger z1 = CryptoOperatorHelper.OTMul(z0, CryptoOperatorHelper.powMod(uidNum));
        BigInteger key =
                CryptoOperatorHelper.OTMul(
                        CryptoOperatorHelper.OTPow(z1, blindingS),
                        CryptoOperatorHelper.OTPow(y, blindingR));

        String aesKey = AESHelper.generateRandomKey();
        BigInteger aesNum =
                ConvertTypeHelper.bytesToBigInteger(aesKey.getBytes(StandardCharsets.UTF_8));
        BigInteger messageCipherNum = key.xor(aesNum);
        String c = AESHelper.encrypt(message, aesKey);

        return new ServerResultBody(messageCipherNum, w, c);
    }

    private ServerResultBody cipherPirTableContentIdObfuscation(
            PirTable pirTable, BigInteger x, BigInteger y, BigInteger z0, int index)
            throws WedprException {
        String message = pirTable.getPirValue();
        BigInteger blindingR = CryptoOperatorHelper.getRandomInt();
        BigInteger blindingS = CryptoOperatorHelper.getRandomInt();
        BigInteger w =
                CryptoOperatorHelper.OTMul(
                        CryptoOperatorHelper.OTPow(x, blindingS),
                        CryptoOperatorHelper.powMod(blindingR));
        BigInteger z1 =
                CryptoOperatorHelper.OTMul(
                        z0, CryptoOperatorHelper.powMod(BigInteger.valueOf(index)));
        BigInteger key =
                CryptoOperatorHelper.OTMul(
                        CryptoOperatorHelper.OTPow(z1, blindingS),
                        CryptoOperatorHelper.OTPow(y, blindingR));

        String aesKey = AESHelper.generateRandomKey();
        BigInteger aesNum =
                ConvertTypeHelper.bytesToBigInteger(aesKey.getBytes(StandardCharsets.UTF_8));
        BigInteger messageCipherNum = key.xor(aesNum);
        String c = AESHelper.encrypt(message, aesKey);

        return new ServerResultBody(messageCipherNum, w, c);
    }
}
