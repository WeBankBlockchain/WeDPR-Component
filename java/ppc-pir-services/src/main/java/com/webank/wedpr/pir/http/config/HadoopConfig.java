package com.webank.wedpr.pir.http.config;

import java.io.IOException;
import java.net.URI;
import org.apache.hadoop.fs.FileSystem;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

/**
 * @author zachma
 * @date 2024/9/4
 */
@Configuration
public class HadoopConfig {
    @Value("${hdfs.uri}")
    private String hdfsUri;

    @Value("${hdfs.user}")
    private String hdfsUser;

    @Value("${hdfs.local}")
    private String hdfsLocal;

    @Bean
    public FileSystem fileSystem() throws IOException, InterruptedException {
        org.apache.hadoop.conf.Configuration configuration =
                new org.apache.hadoop.conf.Configuration();
        configuration.set("fs.defaultFS", hdfsUri);
        FileSystem fileSystem = FileSystem.get(URI.create(hdfsUri), configuration, hdfsUser);
        return fileSystem;
    }
}
