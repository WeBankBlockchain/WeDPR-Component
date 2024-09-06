package com.webank.wedpr.pir;

import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.domain.EntityScan;

/**
 * @author zachma
 * @date 2024/8/18
 */
@SpringBootApplication
@EnableAutoConfiguration
@MapperScan("com.webank.wedpr.pir.http.mapper")
@EntityScan(basePackages = {"cn.webank.wedpr.http", "cn.webank.wedpr.pir"})
public class WedprPirApplication {
    public static void main(String[] args) {
        SpringApplication.run(WedprPirApplication.class, args);
        System.out.println("Start WedprPirApplication Server successfully!");
    }
}
