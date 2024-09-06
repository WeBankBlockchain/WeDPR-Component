package com.wedpr.pir.sdk.gateway;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import java.io.IOException;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import okhttp3.Call;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * @author zachma
 * @date 2024/8/20
 */
public class GatewayClient {

    private static final MediaType JSON_MEDIA_TYPE =
            MediaType.get("application/json; charset=utf-8");

    private static String sendGetRequest(String url) throws IOException, WedprException {
        int timeout = ParamEnum.HttpType.HttpTimeout.getValue();
        OkHttpClient httpClient =
                new OkHttpClient()
                        .newBuilder()
                        .connectTimeout(timeout, TimeUnit.SECONDS)
                        .readTimeout(timeout, TimeUnit.SECONDS)
                        .writeTimeout(timeout, TimeUnit.SECONDS)
                        .build();

        Request getRequest = new Request.Builder().url(url).get().build();

        Call call = httpClient.newCall(getRequest);
        Response response = call.execute();
        String responseBody = "";
        if (response.isSuccessful() && Objects.nonNull(response.body())) {
            responseBody = response.body().string();
        } else {
            throw new WedprException(WedprStatusEnum.HTTP_CALL_ERROR);
        }
        return responseBody;
    }

    private static Response sendPostRequest(String url, String jsonBody) throws IOException {
        if (!url.startsWith("http")) {
            url = "http://" + url;
        }
        OkHttpClient httpClient = new OkHttpClient().newBuilder().build();
        RequestBody body = RequestBody.create(jsonBody, JSON_MEDIA_TYPE);
        Request request = new Request.Builder().post(body).url(url).build();
        Call call = httpClient.newCall(request);
        return call.execute();
    }

    public static <T> T sendPostRequestWithRetry(String url, String body, Class<T> responseType)
            throws WedprException {
        int maxRetries = ParamEnum.HttpType.HttpMaxRetries.getValue();
        int retries = 1;
        while (retries <= maxRetries) {
            try {
                Response response = sendPostRequest(url, body);
                if (response.isSuccessful() && Objects.nonNull(response.body())) {
                    ObjectMapper objectMapper = new ObjectMapper();
                    return objectMapper.readValue(response.body().string(), responseType);
                } else {
                    throw new WedprException(WedprStatusEnum.HTTP_CALL_ERROR);
                }

            } catch (Exception e) {
                e.printStackTrace();
                retries++;
                if (retries > maxRetries) {
                    throw new WedprException(WedprStatusEnum.HTTP_CALL_ERROR);
                }
            }
        }
        return null;
    }
}
